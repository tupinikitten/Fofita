#include "shader_manager.h"
#include "slang_backend.h"
#include "video_math.h"
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define ARQUIVO_LIBRASHADER "librashader.dll"
#elif defined(__APPLE__)
#define ARQUIVO_LIBRASHADER "librashader.dylib"
#else
#define ARQUIVO_LIBRASHADER "librashader.so"
#endif
#define PRESET_TESTE "shaders/fofita_teste/fofita_teste.slangp"
#define ARQUIVO_LOG "fofita_shader.log"
#define LIMITE_DIMENSAO 4096

/* Independent context: never issue GL commands inside SDL_Renderer. */
typedef GLuint (APIENTRY *CreateShaderFn)(GLenum);
typedef void (APIENTRY *ShaderSourceFn)(GLuint,GLsizei,const char *const*,const GLint*);
typedef void (APIENTRY *ShaderFn)(GLuint);
typedef void (APIENTRY *GetShaderFn)(GLuint,GLenum,GLint*);
typedef GLuint (APIENTRY *CreateProgramFn)(void);
typedef void (APIENTRY *AttachFn)(GLuint,GLuint);
typedef void (APIENTRY *GetProgramFn)(GLuint,GLenum,GLint*);
typedef void (APIENTRY *GenFn)(GLsizei,GLuint*);
typedef void (APIENTRY *BindFn)(GLenum,GLuint);
typedef void (APIENTRY *AttachTexFn)(GLenum,GLenum,GLenum,GLuint,GLint);
typedef GLenum (APIENTRY *CheckFn)(GLenum);
typedef void (APIENTRY *DeleteFn)(GLsizei,const GLuint*);
typedef void (APIENTRY *ActiveTexFn)(GLenum);

int video_shader = SHADER_NENHUM;

static SDL_Window *win;
static SDL_GLContext ctx;
static GLuint program, input, output, fbo;
static unsigned char *rgba, *result;
static size_t capacity;
static int modo_ativo;              /* 0: nenhum recurso inicializado */
static int modo_falhou;             /* modo que falhou (persiste apos fechar) */
static char ultimo_erro[1024];
static int in_w, in_h, out_w, out_h;
static size_t contador_quadros;
static double soma_ms;
static int medidas;
static char *caminho_lib_teste, *caminho_preset_teste;

static ShaderFn use_program, delete_shader, delete_program;
static GenFn gen_fbo;
static DeleteFn delete_fbo;
static BindFn bind_fbo;
static AttachTexFn attach_tex;
static CheckFn check_fbo;
static ActiveTexFn active_tex;

/* Diagnostico em arquivo: ajuda a reportar falhas sem depender do console. */
static void registrar(const char *texto) {
    static int iniciado;
    SDL_Log("%s", texto);
    FILE *f = fopen(ARQUIVO_LOG, iniciado ? "a" : "w");
    if (!f) return;
    iniciado = 1;
    fprintf(f, "%s\n", texto);
    fclose(f);
}

static void definir_erro(const char *texto) {
    SDL_strlcpy(ultimo_erro, texto, sizeof(ultimo_erro));
}

int shader_falhou(void) { return video_shader && modo_falhou == video_shader; }
const char *shader_ultimo_erro(void) { return ultimo_erro; }
void shader_limpar_falha(void) { modo_falhou = 0; ultimo_erro[0] = '\0'; }

void shader_slang_definir_caminhos(const char *biblioteca, const char *preset) {
    SDL_free(caminho_lib_teste);
    SDL_free(caminho_preset_teste);
    caminho_lib_teste = biblioteca ? SDL_strdup(biblioteca) : NULL;
    caminho_preset_teste = preset ? SDL_strdup(preset) : NULL;
}

/* Caminho absoluto ao lado do executavel: evita carregar bibliotecas de
   pastas arbitrarias do PATH ou do diretorio de trabalho. */
static char *caminho_do_app(const char *relativo) {
    char *base = SDL_GetBasePath();
    if (!base) return NULL;
    size_t n = strlen(base) + strlen(relativo) + 1;
    char *r = SDL_malloc(n);
    if (r) SDL_snprintf(r, n, "%s%s", base, relativo);
    SDL_free(base);
    return r;
}

/* Cria janela oculta + contexto, restaurando os atributos GL anteriores. */
static int criar_contexto(int maior, int menor, int perfil) {
    int antes_maior = 0, antes_menor = 0, antes_perfil = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &antes_maior);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &antes_menor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &antes_perfil);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, maior);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, menor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, perfil);
    win = SDL_CreateWindow("Fofita shader", 0, 0, 1, 1,
                           SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (win) ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        char texto[512];
        SDL_snprintf(texto, sizeof(texto), "OpenGL %d.%d indisponivel: %s",
                     maior, menor, SDL_GetError());
        definir_erro(texto);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, antes_maior);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, antes_menor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, antes_perfil);
    return ctx != NULL;
}

static int carregar_fbo(void) {
    gen_fbo = (GenFn)SDL_GL_GetProcAddress("glGenFramebuffers");
    delete_fbo = (DeleteFn)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    bind_fbo = (BindFn)SDL_GL_GetProcAddress("glBindFramebuffer");
    attach_tex = (AttachTexFn)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    check_fbo = (CheckFn)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
    if (!gen_fbo || !delete_fbo || !bind_fbo || !attach_tex || !check_fbo) {
        definir_erro("funcoes de framebuffer OpenGL ausentes");
        return 0;
    }
    glGenTextures(1, &input);
    glGenTextures(1, &output);
    gen_fbo(1, &fbo);
    if (!input || !output || !fbo) {
        definir_erro("falha ao criar texturas/framebuffer");
        return 0;
    }
    return 1;
}

#define LOAD(type,name,glname) type name=(type)SDL_GL_GetProcAddress(glname); \
    if(!name) { definir_erro("funcao OpenGL ausente: " glname); return 0; }
static int iniciar_passthrough(void) {
    if (!criar_contexto(2, 1, 0)) return 0;
    LOAD(CreateShaderFn,create_shader,"glCreateShader");
    LOAD(ShaderSourceFn,source,"glShaderSource");
    LOAD(ShaderFn,compile,"glCompileShader");
    LOAD(GetShaderFn,get_shader,"glGetShaderiv");
    LOAD(CreateProgramFn,create_program,"glCreateProgram");
    LOAD(AttachFn,attach,"glAttachShader");
    LOAD(ShaderFn,link,"glLinkProgram");
    LOAD(GetProgramFn,get_program,"glGetProgramiv");
    use_program=(ShaderFn)SDL_GL_GetProcAddress("glUseProgram");
    delete_shader=(ShaderFn)SDL_GL_GetProcAddress("glDeleteShader");
    delete_program=(ShaderFn)SDL_GL_GetProcAddress("glDeleteProgram");
    if(!use_program||!delete_shader||!delete_program) {
        definir_erro("funcoes de shader OpenGL ausentes");
        return 0;
    }
    const char *vs="#version 120\nvoid main(){gl_Position=gl_Vertex;gl_TexCoord[0]=gl_MultiTexCoord0;}";
    const char *fs="#version 120\nuniform sampler2D Source;void main(){gl_FragColor=texture2D(Source,gl_TexCoord[0].xy);}";
    GLuint v=create_shader(GL_VERTEX_SHADER), f=create_shader(GL_FRAGMENT_SHADER);
    GLint vok=0,fok=0,ok=0;
    if(v&&f){
        source(v,1,&vs,NULL); compile(v); get_shader(v,GL_COMPILE_STATUS,&vok);
        source(f,1,&fs,NULL); compile(f); get_shader(f,GL_COMPILE_STATUS,&fok);
        if(vok&&fok){program=create_program();if(program){attach(program,v);attach(program,f);link(program);get_program(program,GL_LINK_STATUS,&ok);}}
    }
    if(v) delete_shader(v);
    if(f) delete_shader(f);
    if(!ok) { definir_erro("falha ao compilar o passthrough"); return 0; }
    if (!carregar_fbo()) return 0;
    registrar("Shader GPU passthrough ativo (experimental)");
    return 1;
}

static int iniciar_slang(void) {
    /* A biblioteca vem antes: se estiver ausente, nem criamos contexto. */
    char *lib = caminho_lib_teste ? SDL_strdup(caminho_lib_teste)
                                  : caminho_do_app(ARQUIVO_LIBRASHADER);
    char erro[900] = "";
    int ok = lib && slang_biblioteca_carregar(lib, erro, sizeof(erro));
    SDL_free(lib);
    if (!ok) { definir_erro(erro[0] ? erro : "sem memoria"); return 0; }

    if (!criar_contexto(3, 3, SDL_GL_CONTEXT_PROFILE_CORE)) return 0;
    if (SDL_GL_MakeCurrent(win, ctx) != 0) { definir_erro(SDL_GetError()); return 0; }
    active_tex = (ActiveTexFn)SDL_GL_GetProcAddress("glActiveTexture");
    if (!active_tex) { definir_erro("glActiveTexture ausente"); return 0; }
    if (!carregar_fbo()) return 0;

    const char *versao = (const char *)glGetString(GL_VERSION);
    const char *gpu = (const char *)glGetString(GL_RENDERER);
    char texto[1200];
    SDL_snprintf(texto, sizeof(texto), "Slang: OpenGL %s | %s",
                 versao ? versao : "?", gpu ? gpu : "?");
    registrar(texto);

    char *preset = caminho_preset_teste ? SDL_strdup(caminho_preset_teste)
                                        : caminho_do_app(PRESET_TESTE);
    Uint64 inicio = SDL_GetPerformanceCounter();
    ok = preset && slang_cadeia_criar(preset, erro, sizeof(erro));
    SDL_free(preset);
    if (!ok) { definir_erro(erro[0] ? erro : "sem memoria"); return 0; }
    SDL_snprintf(texto, sizeof(texto), "Slang: preset carregado em %.0f ms",
                 (SDL_GetPerformanceCounter() - inicio) * 1000.0 /
                 SDL_GetPerformanceFrequency());
    registrar(texto);
    return 1;
}

/* Libera tudo, preservando o contexto que estava atual antes da chamada. */
void shader_fechar(void){
    SDL_Window *oldwin=SDL_GL_GetCurrentWindow();
    SDL_GLContext old=SDL_GL_GetCurrentContext();
    int era_nosso = ctx && old == ctx;
    if(ctx&&SDL_GL_MakeCurrent(win,ctx)==0){
        slang_cadeia_destruir(); /* exige o contexto que criou a cadeia */
        if(program&&delete_program) delete_program(program);
        if(fbo&&delete_fbo) delete_fbo(1,&fbo);
        if(input) glDeleteTextures(1,&input);
        if(output) glDeleteTextures(1,&output);
        if(!era_nosso) SDL_GL_MakeCurrent(oldwin,old);
    }
    if(ctx) SDL_GL_DeleteContext(ctx);
    if(win) SDL_DestroyWindow(win);
    ctx=NULL;win=NULL;program=input=output=fbo=0;
    in_w=in_h=out_w=out_h=0;
    free(rgba);free(result);rgba=result=NULL;capacity=0;
    modo_ativo=0;contador_quadros=0;soma_ms=0;medidas=0;
}

static int preparar(int modo) {
    if (modo_ativo == modo) return SDL_GL_MakeCurrent(win, ctx) == 0;
    if (modo_ativo) shader_fechar();
    int ok = modo == SHADER_SLANG_TESTE ? iniciar_slang() : iniciar_passthrough();
    if (!ok) return 0;
    modo_ativo = modo;
    modo_falhou = 0;
    ultimo_erro[0] = '\0';
    return SDL_GL_MakeCurrent(win, ctx) == 0;
}

/* Desativa o modo ate o proximo jogo ou nova escolha no menu. */
static void falhar(int modo, SDL_Window *oldwin, SDL_GLContext old) {
    if (!ultimo_erro[0]) definir_erro(SDL_GetError());
    char texto[1200];
    SDL_snprintf(texto, sizeof(texto), "Shader indisponivel; fallback SDL. Motivo: %s",
                 ultimo_erro);
    registrar(texto);
    shader_fechar();
    modo_falhou = modo;
    SDL_GL_MakeCurrent(oldwin, old);
}

static int reservar(size_t n) {
    if (n <= capacity) return 1;
    unsigned char *a=malloc(n),*b=malloc(n);
    if(!a||!b){free(a);free(b);definir_erro("sem memoria");return 0;}
    free(rgba);free(result);rgba=a;result=b;capacity=n;
    return 1;
}

static void enviar_entrada(int w, int h) {
    glBindTexture(GL_TEXTURE_2D,input);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    if(w!=in_w||h!=in_h){
        /* Apenas o nivel 0: sem isso a textura ficaria incompleta. */
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
        in_w=w;in_h=h;
    } else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
}

static void preparar_saida(int w, int h) {
    if(w==out_w&&h==out_h) return;
    glBindTexture(GL_TEXTURE_2D,output);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    out_w=w;out_h=h;
}

static int anexar_saida(void) {
    bind_fbo(GL_FRAMEBUFFER,fbo);
    attach_tex(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
    if(check_fbo(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        definir_erro("framebuffer incompleto");
        return 0;
    }
    return 1;
}

static int dimensoes_validas(const void *data, int w, int h, int pitch) {
    return data&&w>0&&h>0&&w<=LIMITE_DIMENSAO&&h<=LIMITE_DIMENSAO&&pitch>0;
}

const void *shader_processar(const void *data,int w,int h,int pitch,Uint32 format,int *out_pitch){
    if(video_shader!=SHADER_PASSTHROUGH || modo_falhou==SHADER_PASSTHROUGH) return NULL;
    if(!dimensoes_validas(data,w,h,pitch)||!out_pitch) return NULL;
    SDL_Window *oldwin=SDL_GL_GetCurrentWindow();
    SDL_GLContext old=SDL_GL_GetCurrentContext();
    ultimo_erro[0]='\0';
    if(!preparar(SHADER_PASSTHROUGH)) goto fail;
    if(!reservar((size_t)w*h*4)) goto fail;
    if(video_frame_para_rgba(data,w,h,pitch,format,rgba)!=0) goto fail;
    enviar_entrada(w,h);
    preparar_saida(w,h);
    if(!anexar_saida()) goto fail;
    glViewport(0,0,w,h);glBindTexture(GL_TEXTURE_2D,input);use_program(program);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(-1,-1);
    glTexCoord2f(1,0);glVertex2f(1,-1);
    glTexCoord2f(1,1);glVertex2f(1,1);
    glTexCoord2f(0,1);glVertex2f(-1,1);
    glEnd();
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    if(glGetError()!=GL_NO_ERROR) { definir_erro("erro OpenGL no passthrough"); goto fail; }
    if(SDL_ConvertPixels(w,h,SDL_PIXELFORMAT_RGBA32,rgba,w*4,format,result,w*SDL_BYTESPERPIXEL(format))!=0) goto fail;
    if(SDL_GL_MakeCurrent(oldwin,old)!=0) goto fail;
    *out_pitch=w*SDL_BYTESPERPIXEL(format);return result;
fail:
    falhar(SHADER_PASSTHROUGH,oldwin,old);
    return NULL;
}

const void *shader_slang_processar(const void *data,int w,int h,int pitch,
                                   Uint32 format,int saida_w,int saida_h){
    if(video_shader!=SHADER_SLANG_TESTE || modo_falhou==SHADER_SLANG_TESTE) return NULL;
    if(!dimensoes_validas(data,w,h,pitch) || saida_w<=0 || saida_h<=0 ||
       saida_w>LIMITE_DIMENSAO || saida_h>LIMITE_DIMENSAO) return NULL;
    SDL_Window *oldwin=SDL_GL_GetCurrentWindow();
    SDL_GLContext old=SDL_GL_GetCurrentContext();
    ultimo_erro[0]='\0';
    if(!preparar(SHADER_SLANG_TESTE)) goto fail;
    Uint64 inicio=SDL_GetPerformanceCounter();
    size_t entrada=(size_t)w*h*4, saida=(size_t)saida_w*saida_h*4;
    if(!reservar(entrada>saida?entrada:saida)) goto fail;
    if(video_frame_para_rgba(data,w,h,pitch,format,rgba)!=0) goto fail;
    /* Formatos sem alfa (XRGB) precisam chegar opacos ao shader. */
    for(size_t i=3;i<entrada;i+=4) rgba[i]=255;
    /* librashader altera bindings; restaurar o que usamos no upload. */
    active_tex(GL_TEXTURE0);
    enviar_entrada(w,h);
    preparar_saida(saida_w,saida_h);
    if(glGetError()!=GL_NO_ERROR) { definir_erro("erro OpenGL ao preparar texturas"); goto fail; }

    char erro[900]="";
    if(!slang_cadeia_quadro(input,w,h,output,saida_w,saida_h,contador_quadros,
                            contador_quadros==0,erro,sizeof(erro))) {
        definir_erro(erro);
        goto fail;
    }
    contador_quadros++;
    /* A biblioteca pode deixar avisos GL pendentes que nao invalidam o frame;
       o resultado e validado pela leitura abaixo. */
    for(int i=0;i<16 && glGetError()!=GL_NO_ERROR;++i) {}
    if(!anexar_saida()) goto fail;
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    /* A linha 0 da textura de entrada volta como a linha 0 da leitura. */
    glReadPixels(0,0,saida_w,saida_h,GL_RGBA,GL_UNSIGNED_BYTE,result);
    if(glGetError()!=GL_NO_ERROR) { definir_erro("erro OpenGL ao ler o resultado"); goto fail; }
    for(size_t i=3;i<saida;i+=4) result[i]=255;
    if(SDL_GL_MakeCurrent(oldwin,old)!=0) goto fail;

    soma_ms+=(SDL_GetPerformanceCounter()-inicio)*1000.0/SDL_GetPerformanceFrequency();
    ++medidas;
    if(contador_quadros==1 || medidas==600) {
        char texto[200];
        SDL_snprintf(texto,sizeof(texto),
                     "Slang: %dx%d -> %dx%d, media %.2f ms por quadro (%d amostras)",
                     w,h,saida_w,saida_h,soma_ms/medidas,medidas);
        registrar(texto);
        soma_ms=0;medidas=0;
    }
    return result;
fail:
    falhar(SHADER_SLANG_TESTE,oldwin,old);
    return NULL;
}

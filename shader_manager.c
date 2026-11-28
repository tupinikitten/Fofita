#include "shader_manager.h"
#include <SDL2/SDL_opengl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

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
static SDL_Window *win;
static SDL_GLContext ctx;
static GLuint program, input, output, fbo;
static unsigned char *rgba, *result;
static size_t capacity;
static int state, tw, th;
int video_shader=0;
int shader_falhou(void){return state<0 && video_shader; }
static ShaderFn use_program, delete_shader, delete_program;
static DeleteFn delete_fbo;
static BindFn bind_fbo;
static AttachTexFn attach_tex;
static CheckFn check_fbo;
#define LOAD(type,name,glname) type name=(type)SDL_GL_GetProcAddress(glname); if(!name) goto fail
static void probe_librashader(void){
    typedef size_t (*VersionFn)(void);
    char *base=SDL_GetBasePath();
    if(!base) return;
    size_t n=strlen(base)+sizeof("librashader.dll");
    char *path=malloc(n);
    if(!path){SDL_free(base);return;}
    snprintf(path,n,"%slibrashader.dll",base);
    SDL_free(base);
    void *lib=SDL_LoadObject(path);
    free(path);
    if(!lib){SDL_Log("Librashader: indisponivel; mantendo passthrough");return;}
    VersionFn abi=(VersionFn)SDL_LoadFunction(lib,"libra_instance_abi_version");
    VersionFn api=(VersionFn)SDL_LoadFunction(lib,"libra_instance_api_version");
    if(abi&&api&&abi()==2&&api()>=5)
        SDL_Log("Librashader: ABI %u API %u (compativel)",(unsigned)abi(),(unsigned)api());
    else SDL_Log("Librashader: API/ABI incompativel; mantendo passthrough");
    SDL_UnloadObject(lib);
}
static int init(void) {
    probe_librashader();
    int major=0,minor=0,profile=0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,&major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,&minor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,&profile);
    /* Probe modern core context independently; keep the validated legacy renderer. */
    SDL_Window *previous_window=SDL_GL_GetCurrentWindow();
    SDL_GLContext previous_context=SDL_GL_GetCurrentContext();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *probe=SDL_CreateWindow("Fofita GL 3.3 probe",0,0,1,1,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    SDL_GLContext modern=probe?SDL_GL_CreateContext(probe):NULL;
    if(modern){
        GLint actual_major=0,actual_minor=0;
        glGetIntegerv(GL_MAJOR_VERSION,&actual_major);
        glGetIntegerv(GL_MINOR_VERSION,&actual_minor);
        SDL_Log("Shader Slang probe: OpenGL %d.%d (%s)",actual_major,actual_minor,
                actual_major>3||(actual_major==3&&actual_minor>=3)?"compativel":"insuficiente");
        SDL_GL_DeleteContext(modern);
    } else SDL_Log("Shader Slang probe: OpenGL 3.3 indisponivel");
    if(probe) SDL_DestroyWindow(probe);
    if(previous_context) SDL_GL_MakeCurrent(previous_window,previous_context);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,0);
    win=SDL_CreateWindow("Fofita shader probe",0,0,1,1,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    if(win) ctx=SDL_GL_CreateContext(win);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,minor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,profile);
    if(!ctx) return 0;
    LOAD(CreateShaderFn,create_shader,"glCreateShader");
    LOAD(ShaderSourceFn,source,"glShaderSource");
    LOAD(ShaderFn,compile,"glCompileShader");
    LOAD(GetShaderFn,get_shader,"glGetShaderiv");
    LOAD(CreateProgramFn,create_program,"glCreateProgram");
    LOAD(AttachFn,attach,"glAttachShader");
    LOAD(ShaderFn,link,"glLinkProgram");
    LOAD(GetProgramFn,get_program,"glGetProgramiv");
    LOAD(GenFn,gen_fbo,"glGenFramebuffers");
    use_program=(ShaderFn)SDL_GL_GetProcAddress("glUseProgram");
    delete_shader=(ShaderFn)SDL_GL_GetProcAddress("glDeleteShader");
    delete_program=(ShaderFn)SDL_GL_GetProcAddress("glDeleteProgram");
    delete_fbo=(DeleteFn)SDL_GL_GetProcAddress("glDeleteFramebuffers");
    bind_fbo=(BindFn)SDL_GL_GetProcAddress("glBindFramebuffer");
    attach_tex=(AttachTexFn)SDL_GL_GetProcAddress("glFramebufferTexture2D");
    check_fbo=(CheckFn)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
    if(!use_program||!delete_shader||!delete_program||!delete_fbo||!bind_fbo||!attach_tex||!check_fbo) goto fail;
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
    if(!ok) goto fail;
    glGenTextures(1,&input); glGenTextures(1,&output); gen_fbo(1,&fbo);
    return input&&output&&fbo;
fail:
    return 0;
}
void shader_fechar(void){
    SDL_Window *oldwin=SDL_GL_GetCurrentWindow();
    SDL_GLContext old=SDL_GL_GetCurrentContext();
    if(ctx&&SDL_GL_MakeCurrent(win,ctx)==0){
        if(program&&delete_program) delete_program(program);
        if(fbo&&delete_fbo) delete_fbo(1,&fbo);
        if(input) glDeleteTextures(1,&input);
        if(output) glDeleteTextures(1,&output);
    }
    if(ctx) SDL_GL_DeleteContext(ctx);
    if(win) SDL_DestroyWindow(win);
    if(old&&old!=ctx) SDL_GL_MakeCurrent(oldwin,old);
    ctx=NULL;win=NULL;program=input=output=fbo=0;tw=th=0;
    free(rgba);free(result);rgba=result=NULL;capacity=0;state=0;
}
const void *shader_processar(const void *data,int w,int h,int pitch,Uint32 format,int *out_pitch){
    if(!video_shader) return NULL;
    if(state<0||!data||w<=0||h<=0||w>4096||h>4096||pitch<=0) return NULL;
    SDL_Window *oldwin=SDL_GL_GetCurrentWindow();
    SDL_GLContext old=SDL_GL_GetCurrentContext();
    if(!state){if(!init()) goto fail;state=1;SDL_Log("Shader GPU passthrough ativo (experimental)");}
    if(SDL_GL_MakeCurrent(win,ctx)!=0) goto fail;
    size_t n=(size_t)w*h*4;
    if(n>capacity){unsigned char *a=malloc(n),*b=malloc(n);if(!a||!b){free(a);free(b);goto fail;}free(rgba);free(result);rgba=a;result=b;capacity=n;}
    if(SDL_ConvertPixels(w,h,format,data,pitch,SDL_PIXELFORMAT_RGBA32,rgba,w*4)!=0) goto fail;
    glBindTexture(GL_TEXTURE_2D,input);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    if(w!=tw||h!=th){
        glBindTexture(GL_TEXTURE_2D,output);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        tw=w;th=h;
    }
    bind_fbo(GL_FRAMEBUFFER,fbo);
    attach_tex(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
    if(check_fbo(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) goto fail;
    glViewport(0,0,w,h);glBindTexture(GL_TEXTURE_2D,input);use_program(program);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex2f(-1,-1);
    glTexCoord2f(1,0);glVertex2f(1,-1);
    glTexCoord2f(1,1);glVertex2f(1,1);
    glTexCoord2f(0,1);glVertex2f(-1,1);
    glEnd();
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    if(glGetError()!=GL_NO_ERROR) goto fail;
    if(SDL_ConvertPixels(w,h,SDL_PIXELFORMAT_RGBA32,rgba,w*4,format,result,w*SDL_BYTESPERPIXEL(format))!=0) goto fail;
    if(SDL_GL_MakeCurrent(oldwin,old)!=0) goto fail;
    *out_pitch=w*SDL_BYTESPERPIXEL(format);return result;
fail:
    SDL_Log("Shader GPU indisponivel; fallback SDL");
    shader_fechar();state=-1;
    SDL_GL_MakeCurrent(oldwin,old);
    return NULL;
}

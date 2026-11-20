#include "save_manager.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

/* Apenas caminhos controlados pela aplicacao; nenhuma parte do nome da
   ROM/core e usada diretamente sem higienizacao. */
static void file_stem(const char *path, char *out, size_t cap) {
    const char *name=path;
    for(const char *p=path; *p; ++p)
        if(*p=='/' || *p=='\\') name=p+1;

    const char *end=name+strlen(name);
    const char *last_dot=NULL;
    for(const char *p=name; p<end; ++p)
        if(*p=='.') last_dot=p;
    if(last_dot && last_dot>name) end=last_dot;

    size_t len=0;
    for(const char *p=name; p<end && len+1<cap && len<40; ++p) {
        unsigned char ch=(unsigned char)*p;
        if((ch>='a' && ch<='z') ||
           (ch>='A' && ch<='Z') ||
           (ch>='0' && ch<='9') || ch=='-' || ch=='_')
            out[len++]=(char)ch;
        else if(len && out[len-1]!='_')
            out[len++]='_';
    }
    if(!len) { memcpy(out,"jogo",5); return; }
    out[len]='\0';
}

static uint64_t game_path_hash(const char *path) {
    uint64_t hash=UINT64_C(14695981039346656037);
    for(const unsigned char *p=(const unsigned char*)path; *p; ++p) {
        unsigned char ch=*p;
        if(ch=='\\') ch='/';
#ifdef _WIN32
        if(ch>='A' && ch<='Z') ch=(unsigned char)(ch-'A'+'a');
#endif
        hash^=(uint64_t)ch;
        hash*=UINT64_C(1099511628211);
    }
    return hash;
}

static bool ensure_directory(const char *path) {
    struct stat st;
    if(stat(path,&st)==0) return (st.st_mode&S_IFMT)==S_IFDIR;
#ifdef _WIN32
    if(_mkdir(path)!=0 && errno!=EEXIST) return false;
#else
    if(mkdir(path,0700)!=0 && errno!=EEXIST) return false;
#endif
    return stat(path,&st)==0 && (st.st_mode&S_IFMT)==S_IFDIR;
}

void save_ram_end(SaveRamSession *session) {
    if(!session) return;
    free(session->last_saved);
    *session=(SaveRamSession){0};
}

const char *save_ram_path(const SaveRamSession *session) {
    return session && session->path[0] ? session->path : NULL;
}

SaveRamStartResult save_ram_begin(SaveRamSession *session,
                                 const char *save_dir,
                                 const char *core_path,
                                 const char *game_path,
                                 void *ram, size_t size) {
    if(!session || session->active || session->last_saved ||
       !save_dir || !*save_dir || !core_path || !*core_path ||
       !game_path || !*game_path || !ram ||
       !size || size>FOFITA_SRAM_MAX_BYTES)
        return SAVE_RAM_BLOCKED;

    /* Zerar primeiro para garantir que, em erro, nao ha checkpoint valido. */
    *session=(SaveRamSession){0};

    char core[48],game[48],directory[FOFITA_SAVE_PATH_MAX];
    file_stem(core_path,core,sizeof(core));
    file_stem(game_path,game,sizeof(game));
    size_t n=strlen(save_dir);
    const char *sep=(save_dir[n-1]=='/' || save_dir[n-1]=='\\') ? "" : "/";
    int d=snprintf(directory,sizeof(directory),"%s%s%s",save_dir,sep,core);
    if(d<0 || (size_t)d>=sizeof(directory) || !ensure_directory(directory))
        return SAVE_RAM_BLOCKED;

    int len=snprintf(session->path,sizeof(session->path),"%s/%s-%016llx.srm",
                     directory,game,(unsigned long long)game_path_hash(game_path));
    if(len<0 || (size_t)len>=sizeof(session->path)) {
        save_ram_end(session);
        return SAVE_RAM_BLOCKED;
    }

    unsigned char *snapshot=(unsigned char*)malloc(size);
    if(!snapshot) {
        save_ram_end(session);
        return SAVE_RAM_BLOCKED;
    }

    FILE *file=fopen(session->path,"rb");
    if(!file) {
        if(errno!=ENOENT) {
            free(snapshot);
            return SAVE_RAM_BLOCKED;
        }
        memcpy(snapshot,ram,size);
        session->last_saved=snapshot;
        session->size=size;
        session->active=true;
        return SAVE_RAM_NEW;
    }

    /* Nunca copiar bytes parciais para RAM; um arquivo truncado tambem
       nao deve ser sobrescrito ao sair do jogo. */
    bool ok=false;
    if(fseek(file,0,SEEK_END)==0) {
        long file_len=ftell(file);
        if(file_len>=0 && (size_t)file_len==size &&
           fseek(file,0,SEEK_SET)==0 &&
           fread(snapshot,1,size,file)==size)
            ok=true;
    }
    if(fclose(file)!=0) ok=false;
    if(!ok) {
        free(snapshot);
        return SAVE_RAM_BLOCKED;
    }

    memcpy(ram,snapshot,size);
    session->last_saved=snapshot;
    session->size=size;
    session->active=true;
    return SAVE_RAM_RESTORED;
}

static bool atomic_replace(const char *source, const char *target) {
#ifdef _WIN32
    return MoveFileExA(source,target,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(source,target)==0;
#endif
}

bool save_ram_checkpoint(SaveRamSession *session,
                         const void *ram, size_t size) {
    if(!session || !session->active || !session->last_saved ||
       !ram || size!=session->size)
        return false;

    if(memcmp(session->last_saved,ram,size)==0) return true;

    char temporary[FOFITA_SAVE_PATH_MAX+48];
#ifdef _WIN32
    unsigned long pid=(unsigned long)_getpid();
#else
    unsigned long pid=(unsigned long)getpid();
#endif
    int n=snprintf(temporary,sizeof(temporary),"%s.tmp.%lu",
                   session->path,pid);
    if(n<0 || (size_t)n>=sizeof(temporary)) return false;

    FILE *file=fopen(temporary,"wb");
    if(!file) return false;

    bool wrote=fwrite(ram,1,size,file)==size;
    if(wrote && fflush(file)!=0) wrote=false;
    if(fclose(file)!=0) wrote=false;
    if(!wrote) {
        remove(temporary);
        return false;
    }
    if(!atomic_replace(temporary,session->path)) {
        remove(temporary);
        return false;
    }
    memcpy(session->last_saved,ram,size);
    return true;
}

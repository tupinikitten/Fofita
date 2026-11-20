#include "state_store.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/types.h>
#endif

#define STATE_HEADER_BYTES 40u
static const unsigned char state_magic[8]={'F','O','F','S','T','v','0','1'};
#define FNV_OFFSET UINT64_C(14695981039346656037)
#define FNV_PRIME UINT64_C(1099511628211)

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *p=(const unsigned char*)data;
    for(size_t i=0; i<size; ++i) {
        hash^=(uint64_t)p[i];
        hash*=FNV_PRIME;
    }
    return hash;
}

static bool hash_file(const char *path, uint64_t *out) {
    FILE *f=fopen(path,"rb");
    if(!f) return false;
    uint64_t hash=FNV_OFFSET;
    unsigned char buf[16384];
    size_t len;
    size_t total=0;
    while((len=fread(buf,1,sizeof(buf),f))!=0) {
        hash=hash_bytes(hash,buf,len);
        total+=len;
    }
    bool ok=total>0 && !ferror(f);
    if(fclose(f)!=0) ok=false;
    if(ok) *out=hash;
    return ok;
}

static uint64_t hash_path(const char *path) {
    uint64_t hash=FNV_OFFSET;
    for(const unsigned char *p=(const unsigned char*)path; *p; ++p) {
        unsigned char ch=*p;
        if(ch=='\\') ch='/';
#ifdef _WIN32
        if(ch>='A' && ch<='Z') ch=(unsigned char)(ch-'A'+'a');
#endif
        hash=hash_bytes(hash,&ch,1);
    }
    return hash;
}

static void safe_stem(const char *path, char out[64]) {
    const char *name=path;
    for(const char *p=path; *p; ++p)
        if(*p=='/' || *p=='\\') name=p+1;
    const char *end=name+strlen(name);
    const char *dot=NULL;
    for(const char *p=name; p<end; ++p)
        if(*p=='.') dot=p;
    if(dot && dot>name) end=dot;

    size_t n=0;
    for(const char *p=name; p<end && n<40; ++p) {
        unsigned char ch=(unsigned char)*p;
        bool allowed=(ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') ||
                     (ch>='0'&&ch<='9') || ch=='-' || ch=='_';
        if(allowed) out[n++]=(char)ch;
        else if(n && out[n-1]!='_') out[n++]='_';
    }
    if(!n) { memcpy(out,"jogo",5); return; }
    out[n]='\0';
}

static bool ensure_dir(const char *dir) {
    struct stat st;
    if(stat(dir,&st)==0) return (st.st_mode&S_IFMT)==S_IFDIR;
#ifdef _WIN32
    if(_mkdir(dir)!=0 && errno!=EEXIST) return false;
#else
    if(mkdir(dir,0700)!=0 && errno!=EEXIST) return false;
#endif
    return stat(dir,&st)==0 && (st.st_mode&S_IFMT)==S_IFDIR;
}

static void write_u64(unsigned char *p,uint64_t v) {
    for(unsigned i=0;i<8;++i) { p[i]=(unsigned char)(v&255u); v>>=8; }
}
static uint64_t read_u64(const unsigned char *p) {
    uint64_t v=0;
    for(unsigned i=0;i<8;++i) v|=((uint64_t)p[i])<<(8*i);
    return v;
}

void state_store_clear(StateStore *store) {
    if(store) *store=(StateStore){0};
}

bool state_store_init(StateStore *store, const char *root,
                      const char *core_path, const char *rom_path) {
    if(!store || store->active || !root || !*root ||
       !core_path || !*core_path || !rom_path || !*rom_path)
        return false;
    *store=(StateStore){0};
    uint64_t core_hash=0,rom_hash=0;
    if(!hash_file(core_path,&core_hash) || !hash_file(rom_path,&rom_hash))
        return false;

    char core[64];
    safe_stem(core_path,core);
    size_t n=strlen(root);
    int written=snprintf(store->directory,sizeof(store->directory),"%s%s%s",
        root,root[n-1]=='/' || root[n-1]=='\\'?"":"/",core);
    if(written<0 || (size_t)written>=sizeof(store->directory) ||
       !ensure_dir(store->directory)) {
        state_store_clear(store);
        return false;
    }
    safe_stem(rom_path,store->game_stem);
    store->path_hash=hash_path(rom_path);
    store->core_hash=core_hash;
    store->rom_hash=rom_hash;
    store->active=true;
    return true;
}

bool state_store_slot_path(const StateStore *store, unsigned slot,
                           char *out, size_t cap) {
    if(!store || !store->active || slot<1 || slot>FOFITA_STATE_SLOTS ||
       !out || !cap) return false;
    int n=snprintf(out,cap,"%s/%s-%016llx-slot%u.fst",
        store->directory,store->game_stem,
        (unsigned long long)store->path_hash,slot);
    return n>=0 && (size_t)n<cap;
}

static bool replace_file(const char *source, const char *dest) {
#ifdef _WIN32
    return MoveFileExA(source,dest,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(source,dest)==0;
#endif
}

static bool write_state_file(const char *path,const StateStore *store,
                             const void *data,size_t size) {
    unsigned char header[STATE_HEADER_BYTES];
    memcpy(header,state_magic,8);
    write_u64(header+8,(uint64_t)size);
    write_u64(header+16,store->core_hash);
    write_u64(header+24,store->rom_hash);
    write_u64(header+32,hash_bytes(FNV_OFFSET,data,size));
    FILE *file=fopen(path,"wb");
    if(!file) return false;
    bool ok=fwrite(header,1,sizeof(header),file)==sizeof(header) &&
            fwrite(data,1,size,file)==size;
    if(ok && fflush(file)!=0) ok=false;
    if(fclose(file)!=0) ok=false;
    if(!ok) remove(path);
    return ok;
}

StateIOResult state_store_read(const StateStore *store, unsigned slot,
                              void *data, size_t expected_size) {
    if(!data || !expected_size || expected_size>FOFITA_STATE_MAX_BYTES)
        return STATE_IO_UNSUPPORTED;
    char path[FOFITA_STATE_PATH_MAX];
    if(!state_store_slot_path(store,slot,path,sizeof(path)))
        return STATE_IO_UNSUPPORTED;
    FILE *file=fopen(path,"rb");
    if(!file) return errno==ENOENT ? STATE_IO_EMPTY : STATE_IO_ERROR;

    StateIOResult result=STATE_IO_CORRUPT;
    unsigned char header[STATE_HEADER_BYTES];
    if(fread(header,1,sizeof(header),file)!=sizeof(header)) goto end;
    if(memcmp(header,state_magic,8)!=0) goto end;

    if(read_u64(header+8)!=(uint64_t)expected_size ||
       read_u64(header+16)!=store->core_hash ||
       read_u64(header+24)!=store->rom_hash) {
        result=STATE_IO_INCOMPATIBLE;
        goto end;
    }
    if(fseek(file,0,SEEK_END)!=0) { result=STATE_IO_ERROR; goto end; }
    long length=ftell(file);
    if(length<0) { result=STATE_IO_ERROR; goto end; }
    if((uint64_t)length!=STATE_HEADER_BYTES+(uint64_t)expected_size)
        goto end;
    if(fseek(file,STATE_HEADER_BYTES,SEEK_SET)!=0) {
        result=STATE_IO_ERROR; goto end;
    }
    /* Copiar para area temporaria: dado corrompido nunca altera 'data'.
       O core so recebe bytes depois que todos os testes passaram. */
    unsigned char *temporary=(unsigned char*)malloc(expected_size);
    if(!temporary) { result=STATE_IO_ERROR; goto end; }
    if(fread(temporary,1,expected_size,file)==expected_size &&
       hash_bytes(FNV_OFFSET,temporary,expected_size)==read_u64(header+32)) {
        memcpy(data,temporary,expected_size);
        result=STATE_IO_OK;
    }
    free(temporary);
end:
    if(ferror(file)) result=STATE_IO_ERROR;
    if(fclose(file)!=0) result=STATE_IO_ERROR;
    return result;
}

StateIOResult state_store_write(const StateStore *store, unsigned slot,
                               const void *data, size_t size) {
    if(!data || !size || size>FOFITA_STATE_MAX_BYTES)
        return STATE_IO_UNSUPPORTED;
    char path[FOFITA_STATE_PATH_MAX];
    if(!state_store_slot_path(store,slot,path,sizeof(path)))
        return STATE_IO_UNSUPPORTED;

    /* Recusar substituir slots incompatíveis/corrompidos: nada é apagado. */
    void *old=NULL;
    bool prior=false;
    FILE *probe=fopen(path,"rb");
    if(probe) {
        fclose(probe);
        old=malloc(size);
        if(!old) return STATE_IO_ERROR;
        StateIOResult result=state_store_read(store,slot,old,size);
        if(result!=STATE_IO_OK) { free(old); return result; }
        prior=true;
    } else if(errno!=ENOENT) {
        return STATE_IO_ERROR;
    }

    char tmp[FOFITA_STATE_PATH_MAX+16];
    char bak[FOFITA_STATE_PATH_MAX+16];
    char btmp[FOFITA_STATE_PATH_MAX+24];
    int a=snprintf(tmp,sizeof(tmp),"%s.new",path);
    int b=snprintf(bak,sizeof(bak),"%s.bak",path);
    int c=snprintf(btmp,sizeof(btmp),"%s.bak.new",path);
    if(a<0 || b<0 || c<0 ||
       (size_t)a>=sizeof(tmp) || (size_t)b>=sizeof(bak) ||
       (size_t)c>=sizeof(btmp)) {
        free(old); return STATE_IO_ERROR;
    }

    if(!write_state_file(tmp,store,data,size)) {
        free(old); return STATE_IO_ERROR;
    }
    if(prior) {
        /* Mantém um backup do slot anterior antes da troca atomica. */
        if(!write_state_file(btmp,store,old,size) ||
           !replace_file(btmp,bak)) {
            remove(tmp);
            remove(btmp);
            free(old);
            return STATE_IO_ERROR;
        }
    }
    free(old);
    if(!replace_file(tmp,path)) {
        remove(tmp);
        return STATE_IO_ERROR;
    }
    return STATE_IO_OK;
}

const char *state_io_description(StateIOResult result) {
    switch(result) {
    case STATE_IO_OK: return "concluido";
    case STATE_IO_EMPTY: return "slot vazio";
    case STATE_IO_UNSUPPORTED: return "core nao suporta esse estado";
    case STATE_IO_INCOMPATIBLE: return "estado incompativel com core/ROM/tamanho";
    case STATE_IO_CORRUPT: return "estado corrompido (arquivo preservado)";
    case STATE_IO_ERROR: return "erro de leitura/escrita (arquivo preservado)";
    default: return "erro desconhecido";
    }
}

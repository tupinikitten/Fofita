#include "core_paths.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/types.h>
#endif

#define CORE_PATH_MAX 1024
static char system_path[CORE_PATH_MAX];
static char save_path[CORE_PATH_MAX];

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

void core_paths_clear(void) {
    system_path[0]='\0';
    save_path[0]='\0';
}

bool core_paths_init(const char *root) {
    core_paths_clear();
    if(!root || !root[0]) return false;

    size_t n=strlen(root);
    const char *sep=(root[n-1]=='/' || root[n-1]=='\\') ? "" : "/";
    char sys[CORE_PATH_MAX],save[CORE_PATH_MAX];
    int a=snprintf(sys,sizeof(sys),"%s%ssystem",root,sep);
    int b=snprintf(save,sizeof(save),"%s%ssaves",root,sep);
    if(a<0 || b<0 || (size_t)a>=sizeof(sys) || (size_t)b>=sizeof(save))
        return false;
    if(!ensure_directory(sys) || !ensure_directory(save)) return false;

    memcpy(system_path,sys,(size_t)a+1);
    memcpy(save_path,save,(size_t)b+1);
    return true;
}
const char *core_paths_system_dir(void) {
    return system_path[0] ? system_path : NULL;
}
const char *core_paths_save_dir(void) {
    return save_path[0] ? save_path : NULL;
}

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define TEST_MKDIR(p) _mkdir(p)
#define TEST_RMDIR(p) _rmdir(p)
#else
#include <unistd.h>
#define TEST_MKDIR(p) mkdir(p,0700)
#define TEST_RMDIR(p) rmdir(p)
#endif
#include "../core_paths.h"

int main(void) {
    const char *root="b1-test-dirs";
    char system[256],saves[256];
    assert(TEST_MKDIR(root)==0);
    assert(core_paths_init(root));
    assert(core_paths_system_dir()!=NULL);
    assert(core_paths_save_dir()!=NULL);
    snprintf(system,sizeof(system),"%s",core_paths_system_dir());
    snprintf(saves,sizeof(saves),"%s",core_paths_save_dir());
    assert(strcmp(system,"b1-test-dirs/system")==0);
    assert(strcmp(saves,"b1-test-dirs/saves")==0);
    assert(core_paths_init(root)); /* repetir nao apaga os diretorios */
    core_paths_clear();
    assert(!core_paths_system_dir() && !core_paths_save_dir());
    assert(!core_paths_init(NULL));
    assert(!core_paths_init(""));

    assert(TEST_RMDIR(system)==0);
    assert(TEST_RMDIR(saves)==0);
    assert(TEST_RMDIR(root)==0);
    puts("B1: diretorios persistentes OK");
    return 0;
}

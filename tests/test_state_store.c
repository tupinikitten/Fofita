#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#define RMDIR(p) _rmdir(p)
#else
#include <unistd.h>
#define MKDIR(p) mkdir(p,0700)
#define RMDIR(p) rmdir(p)
#endif
#include "../state_store.h"

static void create_file(const char *path,const void *data,size_t len) {
    FILE *f=fopen(path,"wb");
    assert(f && fwrite(data,1,len,f)==len && fclose(f)==0);
}
static void backup_payload(const char *path,const unsigned char *expected,size_t size) {
    FILE *f=fopen(path,"rb");
    assert(f && fseek(f,40,SEEK_SET)==0);
    unsigned char got[32]={0};
    assert(size<=sizeof(got));
    assert(fread(got,1,size,f)==size);
    assert(memcmp(got,expected,size)==0 && fclose(f)==0);
}
int main(void) {
    const char *root="b22-test-states";
    const char *states="b22-test-states/states";
    const char *core="b22-test-states/gambatte_libretro.dll";
    const char *rom="b22-test-states/Pokemon.gb";
    const char *rom2="b22-test-states/Pokemon2.gb";
    assert(MKDIR(root)==0);
    assert(MKDIR(states)==0);
    create_file(core,"core-release-1",14);
    create_file(rom,"ROM_A_original",14);
    create_file(rom2,"ROM_B_original",14);

    StateStore store={0};
    assert(state_store_init(&store,states,core,rom));
    assert(store.active);
    char path1[FOFITA_STATE_PATH_MAX],path2[FOFITA_STATE_PATH_MAX];
    char path3[FOFITA_STATE_PATH_MAX],bak[FOFITA_STATE_PATH_MAX+16];
    assert(state_store_slot_path(&store,1,path1,sizeof(path1)));
    assert(state_store_slot_path(&store,2,path2,sizeof(path2)));
    assert(state_store_slot_path(&store,3,path3,sizeof(path3)));
    assert(!state_store_slot_path(&store,0,path1,sizeof(path1))); /* nao altera path */
    assert(!state_store_slot_path(&store,4,path1,sizeof(path1)));
    assert(state_store_slot_path(&store,1,path1,sizeof(path1)));
    assert(strcmp(path1,path2)!=0 && strcmp(path2,path3)!=0);
    unsigned char game[8]={1,2,3,4,5,6,7,8};
    unsigned char restored[8]={0};
    assert(state_store_read(&store,1,restored,sizeof(restored))==STATE_IO_EMPTY);
    assert(state_store_write(&store,1,game,sizeof(game))==STATE_IO_OK);
    assert(state_store_read(&store,1,restored,sizeof(restored))==STATE_IO_OK);
    assert(memcmp(game,restored,sizeof(game))==0);
    assert(state_store_read(&store,1,restored,7)==STATE_IO_INCOMPATIBLE);
    assert(state_store_write(&store,1,game,FOFITA_STATE_MAX_BYTES+1u)==STATE_IO_UNSUPPORTED);
    assert(state_store_write(&store,0,game,sizeof(game))==STATE_IO_UNSUPPORTED);

    /* Os 3 slots de um unico jogo nunca se sobrescrevem. */
    const unsigned char old1[8]={1,2,3,4,5,6,7,8};
    const unsigned char slot2[8]={11,12,13,14,15,16,17,18};
    const unsigned char slot3[8]={21,22,23,24,25,26,27,28};
    assert(state_store_write(&store,2,slot2,sizeof(slot2))==STATE_IO_OK);
    assert(state_store_write(&store,3,slot3,sizeof(slot3))==STATE_IO_OK);
    memset(restored,0,sizeof(restored));
    assert(state_store_read(&store,2,restored,sizeof(restored))==STATE_IO_OK);
    assert(memcmp(restored,slot2,sizeof(restored))==0);
    assert(state_store_read(&store,3,restored,sizeof(restored))==STATE_IO_OK);
    assert(memcmp(restored,slot3,sizeof(restored))==0);

    /* Sobrescrever um slot mantem estado anterior no .bak. */
    game[0]=99;
    assert(state_store_write(&store,1,game,sizeof(game))==STATE_IO_OK);
    assert(snprintf(bak,sizeof(bak),"%s.bak",path1)>0);
    backup_payload(bak,old1,sizeof(old1));
    assert(state_store_read(&store,1,restored,sizeof(restored))==STATE_IO_OK);
    assert(memcmp(restored,game,sizeof(game))==0);

    /* Trocar conteudo de ROM ou core no mesmo caminho INVALIDA o slot. */
    StateStore another={0};
    create_file(rom,"ROM_A_modified",14);
    assert(state_store_init(&another,states,core,rom));
    memset(restored,0xA5,sizeof(restored));
    assert(state_store_read(&another,1,restored,sizeof(restored))==STATE_IO_INCOMPATIBLE);
    for(size_t n=0;n<sizeof(restored);++n) assert(restored[n]==0xA5);
    assert(state_store_write(&another,1,game,sizeof(game))==STATE_IO_INCOMPATIBLE);
    state_store_clear(&another);
    create_file(rom,"ROM_A_original",14);

    create_file(core,"core-release-2",14);
    assert(state_store_init(&another,states,core,rom));
    assert(state_store_read(&another,1,restored,sizeof(restored))==STATE_IO_INCOMPATIBLE);
    state_store_clear(&another);
    create_file(core,"core-release-1",14);

    /* Identificar outro jogo e outro sistema sem misturar arquivos. */
    assert(state_store_init(&another,states,core,rom2));
    char otherpath[FOFITA_STATE_PATH_MAX];
    assert(state_store_slot_path(&another,1,otherpath,sizeof(otherpath)));
    assert(strcmp(otherpath,path1)!=0);
    assert(state_store_read(&another,1,restored,sizeof(restored))==STATE_IO_EMPTY);
    state_store_clear(&another);

    /* Verificar checksum: qualquer byte alterado impede entregar ao core. */
    FILE *f=fopen(path2,"r+b"); assert(f);
    assert(fseek(f,40,SEEK_SET)==0 && fputc(0xFF,f)!=EOF && fclose(f)==0);
    memset(restored,0xA5,sizeof(restored));
    assert(state_store_read(&store,2,restored,sizeof(restored))==STATE_IO_CORRUPT);
    for(size_t n=0;n<sizeof(restored);++n) assert(restored[n]==0xA5);
    assert(state_store_write(&store,2,game,sizeof(game))==STATE_IO_CORRUPT);

    /* Arquivo truncado e rejeitado, nao reposto automaticamente. */
    create_file(path3,"broken",6);
    assert(state_store_read(&store,3,restored,sizeof(restored))==STATE_IO_CORRUPT);
    assert(state_store_write(&store,3,game,sizeof(game))==STATE_IO_CORRUPT);
    struct stat st; assert(stat(path3,&st)==0 && st.st_size==6);

    state_store_clear(&store);
    assert(remove(path1)==0);
    assert(remove(path2)==0);
    assert(remove(path3)==0);
    assert(remove(bak)==0);
    assert(RMDIR("b22-test-states/states/gambatte_libretro")==0);
    assert(remove(core)==0);
    assert(remove(rom)==0);
    assert(remove(rom2)==0);
    assert(RMDIR(states)==0);
    assert(RMDIR(root)==0);
    puts("B2.2: 3 slots, .bak, integridade e compatibilidade OK");
    return 0;
}

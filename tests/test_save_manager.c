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
#include "../save_manager.h"

static void check_file(const char *path, const unsigned char *expected,
                       size_t size) {
    FILE *f=fopen(path,"rb");
    assert(f);
    unsigned char actual[32]={0};
    assert(size<=sizeof(actual));
    assert(fread(actual,1,size,f)==size);
    assert(fgetc(f)==EOF);
    assert(fclose(f)==0);
    assert(memcmp(actual,expected,size)==0);
}

int main(void) {
    const char *root="b2-test-sram";
    assert(TEST_MKDIR(root)==0);

    unsigned char mem[8]={0,1,2,3,4,5,6,7};
    const unsigned char original[8]={0,1,2,3,4,5,6,7};
    SaveRamSession session={0};
    assert(save_ram_begin(&session,root,"cores/fceumm_libretro.dll",
                          "roms/nes/exemplo.nes",mem,sizeof(mem))==SAVE_RAM_NEW);
    assert(session.active);
    char nes_path[FOFITA_SAVE_PATH_MAX];
    assert(save_ram_path(&session));
    snprintf(nes_path,sizeof(nes_path),"%s",save_ram_path(&session));
    assert(strstr(nes_path,"fceumm_libretro")!=NULL);

    /* Um jogo novo nao grava arquivo ate o conteudo da SRAM mudar. */
    assert(save_ram_checkpoint(&session,mem,sizeof(mem)));
    FILE *check=fopen(nes_path,"rb");
    assert(!check);
    mem[3]=77;
    assert(save_ram_checkpoint(&session,mem,sizeof(mem)));
    check_file(nes_path,mem,sizeof(mem));
    mem[4]=66;
    assert(!save_ram_checkpoint(&session,mem,sizeof(mem)-1));
    check_file(nes_path,(const unsigned char[]){0,1,2,77,4,5,6,7},8);
    assert(save_ram_checkpoint(&session,mem,sizeof(mem)));
    check_file(nes_path,mem,sizeof(mem));
    save_ram_end(&session);
    assert(!session.active && !save_ram_path(&session));

    /* Reiniciar o programa simulado deve restaurar os bytes anteriores. */
    unsigned char restored[8]={0};
    assert(save_ram_begin(&session,root,"cores/fceumm_libretro.dll",
                          "roms/nes/exemplo.nes",restored,sizeof(restored))==
           SAVE_RAM_RESTORED);
    assert(memcmp(restored,mem,sizeof(mem))==0);
    save_ram_end(&session);

    /* Salvo por core e por jogo, compartilhando a DLL Gambatte para GB/GBC. */
    unsigned char gb[8]={10,11,12,13,14,15,16,17};
    assert(save_ram_begin(&session,root,"cores/gambatte_libretro.dll",
                          "roms/gb/exemplo.gb",gb,sizeof(gb))==SAVE_RAM_NEW);
    char gb_path[FOFITA_SAVE_PATH_MAX];
    snprintf(gb_path,sizeof(gb_path),"%s",save_ram_path(&session));
    assert(strcmp(gb_path,nes_path)!=0);
    gb[0]=50;
    assert(save_ram_checkpoint(&session,gb,sizeof(gb)));
    save_ram_end(&session);

    unsigned char gbc[8]={20,21,22,23,24,25,26,27};
    assert(save_ram_begin(&session,root,"cores/gambatte_libretro.dll",
                          "roms/gbc/exemplo.gbc",gbc,sizeof(gbc))==SAVE_RAM_NEW);
    char gbc_path[FOFITA_SAVE_PATH_MAX];
    snprintf(gbc_path,sizeof(gbc_path),"%s",save_ram_path(&session));
    assert(strcmp(gbc_path,gb_path)!=0);
    gbc[0]=60;
    assert(save_ram_checkpoint(&session,gbc,sizeof(gbc)));
    save_ram_end(&session);

    /* ROM cujo arquivo foi truncado: proteger save e RAM atuais. */
    check=fopen(nes_path,"wb");
    assert(check);
    assert(fwrite(original,1,3,check)==3);
    assert(fclose(check)==0);
    unsigned char untouched[8]={9,9,9,9,9,9,9,9};
    assert(save_ram_begin(&session,root,"cores/fceumm_libretro.dll",
                          "roms/nes/exemplo.nes",untouched,sizeof(untouched))==
           SAVE_RAM_BLOCKED);
    for(unsigned i=0;i<sizeof(untouched);i++) assert(untouched[i]==9);
    assert(!save_ram_checkpoint(&session,untouched,sizeof(untouched)));
    save_ram_end(&session);
    struct stat st;
    assert(stat(nes_path,&st)==0 && st.st_size==3);

    /* Defesa contra entradas invalidas ou tamanhos de SRAM excessivos. */
    assert(save_ram_begin(&session,root,"cores/fceumm_libretro.dll",
                          "roms/nes/exemplo.nes",NULL,8)==SAVE_RAM_BLOCKED);
    assert(save_ram_begin(&session,root,"cores/fceumm_libretro.dll",
                          "roms/nes/exemplo.nes",untouched,
                          FOFITA_SRAM_MAX_BYTES+1u)==SAVE_RAM_BLOCKED);

    assert(remove(nes_path)==0);
    assert(remove(gb_path)==0);
    assert(remove(gbc_path)==0);
    assert(TEST_RMDIR("b2-test-sram/fceumm_libretro")==0);
    assert(TEST_RMDIR("b2-test-sram/gambatte_libretro")==0);
    assert(TEST_RMDIR(root)==0);
    puts("B2.1: SRAM - escrita segura, reinicio e protecao de arquivos OK");
    return 0;
}

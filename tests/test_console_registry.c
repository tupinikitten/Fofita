#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../console_registry.h"

static const ConsoleConfig *find_id(const char *id) {
    size_t count=0;
    const ConsoleConfig *all=console_registry_get_all(&count);
    for(size_t i=0; i<count; ++i)
        if(strcmp(all[i].id,id)==0) return &all[i];
    return NULL;
}
int main(void) {
    size_t count=0;
    const ConsoleConfig *all=console_registry_get_all(&count);
    assert(all);
    assert(count==14);
    assert(!find_id("arcade"));
    assert(!find_id("neogeo"));

    const ConsoleConfig *gb=find_id("gb");
    const ConsoleConfig *gbc=find_id("gbc");
    const ConsoleConfig *nes=find_id("nes");
    assert(gb && gbc && nes);
    assert(gb!=gbc);
    assert(gb->caminho_core && gbc->caminho_core && nes->caminho_core);
    assert(strcmp(gb->caminho_core,"cores/gambatte_libretro.dll")==0);
    assert(strcmp(gbc->caminho_core,gb->caminho_core)==0);
    assert(strcmp(nes->caminho_core,"cores/fceumm_libretro.dll")==0);
    assert(strcmp(gb->pasta_roms,"roms/gb")==0);
    assert(strcmp(gbc->pasta_roms,"roms/gbc")==0);
    assert(console_registry_find_by_extension(".gb")==gb);
    assert(console_registry_find_by_extension(".dmg")==gb);
    assert(console_registry_find_by_extension(".gbc")==gbc);
    assert(console_registry_find_by_extension(".GBC")==gbc);
    assert(console_registry_find_by_extension(".GB")==gb);
    assert(console_registry_find_by_extension(".nes")==nes);
    assert(!console_registry_find_by_extension(".zip"));

    const char *sega_ids[]={"sms","gamegear","genesis","dreamcast"};
    for(size_t i=0; i<sizeof(sega_ids)/sizeof(sega_ids[0]); ++i) {
        const ConsoleConfig *system=find_id(sega_ids[i]);
        assert(system && system->familia && strcmp(system->familia,"Sega")==0);
    }
    /* Nao exigir a DLL nos testes: o core binario e instalado separadamente. */
    puts("B1.2: Game Boy/Game Boy Color e registro de consoles OK");
    return 0;
}

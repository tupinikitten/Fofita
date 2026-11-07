#include "console_registry.h"
#include <string.h>
#include <stdio.h>

#define EXT_LIST(name, ...) static const char *const name[] = { __VA_ARGS__ }

EXT_LIST(ext_atari2600, ".a26", ".bin", ".rom");
EXT_LIST(ext_nes, ".nes", ".unf", ".unif");
EXT_LIST(ext_snes, ".sfc", ".smc");
EXT_LIST(ext_gb, ".gb");
EXT_LIST(ext_gbc, ".gbc");
EXT_LIST(ext_gba, ".gba");
EXT_LIST(ext_n64, ".z64", ".n64", ".v64");
EXT_LIST(ext_ds, ".nds");
EXT_LIST(ext_sms, ".sms");
EXT_LIST(ext_gamegear, ".gg");
EXT_LIST(ext_genesis, ".md", ".gen", ".bin");
EXT_LIST(ext_ps1, ".cue", ".chd", ".pbp", ".iso");
EXT_LIST(ext_psp, ".iso", ".cso", ".pbp");
EXT_LIST(ext_dreamcast, ".cdi", ".gdi", ".chd");
EXT_LIST(ext_neogeo, ".zip", ".7z");
EXT_LIST(ext_arcade, ".zip", ".7z");

/*
 * Os sistemas sem molde ainda ficam cadastrados, mas não aparecem com um
 * cartucho incorreto. Os moldes e coordenadas serão adicionados quando os
 * respectivos assets forem preparados.
 * pasta_roms usa subpastas por sistema; a leitura também aceita ROMs antigas
 * diretamente em roms/.
 */
static const ConsoleConfig consoles[] = {
    {"atari2600", "Atari 2600", ext_atari2600, sizeof(ext_atari2600)/sizeof(ext_atari2600[0]), "roms/atari2600", "assets/atari2600", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/atari_icon.png"},
    /* Coordenadas medidas no arquivo templates/nes_template.png (730x835).
       molde_w/molde_h DEVEM ser o tamanho real do PNG; off_x/off_y/label_w/label_h
       sao medidos nesse mesmo PNG. A etiqueta do NES ocupa x=298..619, y=31..550.
       Os cantos inferiores do rebaixo tem raio ~6 px (medido no template). */
    {"nes", "Nintendo Entertainment System", ext_nes, sizeof(ext_nes)/sizeof(ext_nes[0]), "roms/nes", "labels/nes", "assets/nes/body/cartridge_base.png", 730,835, 298,31,322,520, CAPA_AUTO, 6, "cores/fceumm_libretro.dll", "ui/consoles/nes_icon.png"},
    {"snes", "Super Nintendo", ext_snes, sizeof(ext_snes)/sizeof(ext_snes[0]), "roms/snes", "assets/snes", "templates/snes_template.png", 400,400, 40,100,320,220, CAPA_CONTAIN, 0, NULL, NULL},
    {"gb", "Game Boy", ext_gb, sizeof(ext_gb)/sizeof(ext_gb[0]), "roms/gb", "assets/gb", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/gb_icon.png"},
    {"gbc", "Game Boy Color", ext_gbc, sizeof(ext_gbc)/sizeof(ext_gbc[0]), "roms/gbc", "assets/gbc", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/gbc_icon.png"},
    {"gba", "Game Boy Advance", ext_gba, sizeof(ext_gba)/sizeof(ext_gba[0]), "roms/gba", "assets/gba", "templates/gba_template.png", 400,400, 60,120,280,150, CAPA_CONTAIN, 0, NULL, "ui/consoles/gba_icon.png"},
    {"n64", "Nintendo 64", ext_n64, sizeof(ext_n64)/sizeof(ext_n64[0]), "roms/n64", "assets/n64", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"nds", "Nintendo DS", ext_ds, sizeof(ext_ds)/sizeof(ext_ds[0]), "roms/nds", "assets/nds", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/ds_icon.png"},
    {"sms", "Master System", ext_sms, sizeof(ext_sms)/sizeof(ext_sms[0]), "roms/sms", "assets/sms", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"gamegear", "Game Gear", ext_gamegear, sizeof(ext_gamegear)/sizeof(ext_gamegear[0]), "roms/gamegear", "assets/gamegear", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"genesis", "Mega Drive / Genesis", ext_genesis, sizeof(ext_genesis)/sizeof(ext_genesis[0]), "roms/genesis", "assets/genesis", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"ps1", "PlayStation", ext_ps1, sizeof(ext_ps1)/sizeof(ext_ps1[0]), "roms/ps1", "assets/ps1", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/ps1_icon.png"},
    {"psp", "PlayStation Portable", ext_psp, sizeof(ext_psp)/sizeof(ext_psp[0]), "roms/psp", "assets/psp", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, "ui/consoles/psp_icon.png"},
    {"dreamcast", "Dreamcast", ext_dreamcast, sizeof(ext_dreamcast)/sizeof(ext_dreamcast[0]), "roms/dreamcast", "assets/dreamcast", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"neogeo", "Neo Geo", ext_neogeo, sizeof(ext_neogeo)/sizeof(ext_neogeo[0]), "roms/neogeo", "assets/neogeo", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL},
    {"arcade", "Arcade", ext_arcade, sizeof(ext_arcade)/sizeof(ext_arcade[0]), "roms/arcade", "assets/arcade", NULL, 400,400, 0,0,0,0, CAPA_CONTAIN, 0, NULL, NULL}
};

static int equals_ignore_case(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

const ConsoleConfig *console_registry_get_all(size_t *quantidade) {
    if (quantidade) *quantidade = sizeof(consoles) / sizeof(consoles[0]);
    return consoles;
}

int console_registry_esta_ativo(const ConsoleConfig *console) {
    if (!console || !console->caminho_core) return 0;
    FILE *f = fopen(console->caminho_core, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

const ConsoleConfig *console_registry_find_by_extension(const char *extensao) {
    size_t count = sizeof(consoles) / sizeof(consoles[0]);
    for (size_t i = 0; i < count; ++i)
        for (size_t j = 0; j < consoles[i].quantidade_extensoes; ++j)
            if (equals_ignore_case(extensao, consoles[i].extensoes[j])) return &consoles[i];
    return NULL;
}
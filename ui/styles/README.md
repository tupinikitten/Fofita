# Recursos de estilo da interface do Fofita

Os seis icones laterais possuem nomes canonicos, com um arquivo por preset:

| Acao | Light | Dark | Colorful |
| --- | --- | --- | --- |
| Conquistas | `light/icons/trophy_icon_light.png` | `dark/icons/trophy_icon_dark.png` | `colorful/icons/trophy_icon_colorful.png` |
| Configuracoes | `light/icons/settings_icon_light.png` | `dark/icons/settings_icon_dark.png` | `colorful/icons/settings_icon_colorful.png` |
| Filtros | `light/icons/filter_icon_light.png` | `dark/icons/filter_icon_dark.png` | `colorful/icons/filter_icon_colorful.png` |
| Favoritos | `light/icons/heart_icon_light.png` | `dark/icons/heart_icon_dark.png` | `colorful/icons/heart_icon_colorful.png` |
| Aparencia | `light/icons/bucket_icon_light.png` | `dark/icons/bucket_icon_dark.png` | `colorful/icons/bucket_icon_colorful.png` |
| Sair | `light/icons/out_icon_light.png` | `dark/icons/out_icon_dark.png` | `colorful/icons/out_icon_colorful.png` |

**Light** identifica os icones claros, para wallpapers escuros.
**Dark** identifica os icones escuros, para wallpapers claros.
**Colorful** preserva as cores individuais dos PNGs.

O contraste automatico escolhe entre Light e Dark ao analisar o wallpaper.
Aparencia permite desativar o contraste automatico e selecionar um preset manual.

Os PNGs da pasta Dark que nao tinham versao com sufixo (trophy, heart e out)
foram substituidos por copias das silhuetas atuais da pasta Light, para
preservar o desenho mais recente. O Fofita tinge as silhuetas para a cor escura
ao carregar cada preset, portanto a cor de origem do PNG nao altera o resultado.

As setas continuam em `light/arrows/` e `dark/arrows/`, com nomes
`left_arrow.png` e `right_arrow.png`. Colorful usa o par de maior contraste
conforme o wallpaper.

**Nao utilizar nomes legados** como `trophy_icon.png`, `heart_icon.png`
ou `brush_icon.png`. O balde substitui o pincel no menu Aparencia.

Ao adicionar um preset, manter os seis arquivos no padrao `<nome>_icon_<preset>.png`.
O empacotamento Windows verifica a presenca desses arquivos e rejeita nomes legados.

Para testar um build, extrair o artifact em uma pasta nova, evitando
misturar recursos antigos de pacotes previamente extraidos.

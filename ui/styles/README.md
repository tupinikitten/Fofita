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

## Pacotes criados pelo usuario

Agora a opcao **Aparencia > Personalizar barra lateral** permite selecionar pacotes proprios. Os presets oficiais Light, Dark e Colorful continuam disponiveis como padrao e com contraste automatico. **Um pacote pessoal substitui apenas os seis icones; nao cria novas acoes na barra.**

Crie uma pasta em `ui/styles/`, com nome de ate 32 caracteres formado por letras sem acentos, numeros, `_` ou `-`. Nao use `light`, `dark` ou `colorful` como nome para um novo pacote.

Por exemplo, para um pacote chamado `minhafofita`:

```text
ui/styles/minhafofita/
└── icons/
    ├── trophy_icon_minhafofita.png
    ├── settings_icon_minhafofita.png
    ├── filter_icon_minhafofita.png
    ├── heart_icon_minhafofita.png
    ├── bucket_icon_minhafofita.png
    └── out_icon_minhafofita.png
```

Recomenda-se **PNG transparente 512 x 512**, mantendo margens visuais semelhantes entre os seis desenhos. As cores dos PNGs personalizados serao preservadas. So aparecem na lista os pacotes com todos os seis arquivos no formato esperado. Um PNG corrompido tera fallback para o icone do preset oficial; ele nao deve impedir abrir o menu.

A Fofita procura ate 32 pacotes extras, ordenados alfabeticamente. Depois de copiar uma pasta com o programa aberto, escolha **Atualizar pacotes** no submenu. A escolha e salva em `fofita_menu.cfg`. A opcao **Padrao** retorna aos presets automaticos/manuais originais.

O submenu tambem permite:
- mostrar a **pilula** translucida ou deixar os **icones soltos** sobre o wallpaper;
- **mostrar ou ocultar o carrossel de consoles** do topo (sem impedir a troca de console por teclado/controle: foco no cabecalho + esquerda/direita; quando oculto, o nome do console aparece em um aviso breve).

Essas preferencias nao alteram as ROMs, o backend, os cartuchos nem as capas dos cards. Os pacotes de cada usuario podem ficar apenas na pasta local da Fofita; nao e necessario inclui-los no GitHub.

# Notas das alterações — encaixe das artcovers

- As capas originais continuam podendo ter qualquer resolução; não é necessário convertê-las todas para 512x512.
- O cadastro de cada console agora tem `modo_encaixe_capa`:
  - `CAPA_CONTAIN`: mantém toda a imagem e sua proporção, podendo deixar margens.
  - `CAPA_CROP`: mantém a proporção e recorta o excesso para preencher a etiqueta.
  - `CAPA_STRETCH`: preenche a etiqueta inteira, podendo deformar a imagem.
- O desenho consulta a dimensão real da textura com `SDL_QueryTexture` e calcula o encaixe na área de etiqueta configurada para o molde.
- O modo inicial dos sistemas cadastrados é `CAPA_CONTAIN`, priorizando evitar distorções. O modo pode ser alterado individualmente na entrada do console em `console_registry.c`.
- Para imagens futuras, uma versão em cache com maior dimensão de 512 px é adequada à biblioteca; 1024 px pode ser mantida para zoom/alta qualidade. Isso é uma recomendação, não uma exigência do carregador.

## Validação

Os módulos sem dependência de SDL foram verificados com `gcc -fsyntax-only`. A compilação integral do frontend e o teste visual no Windows ainda precisam ser executados num ambiente com MinGW, SDL2 e SDL2_image instalados; esses componentes de desenvolvimento não estão disponíveis neste ambiente.


## Correcao adicional apos teste visual (NES)

- O molde `assets/nes_template.png` mede 687x764 pixels, mas o cadastro antigo o tratava como 400x400. Isso deformava o cartucho e tornava incorretas as coordenadas da etiqueta.
- O cadastro do NES agora usa as dimensoes reais do PNG e uma regiao de etiqueta aproximada em x=273, y=0, largura=323, altura=520.
- A arte do NES usa `CAPA_CROP`, preenchendo a etiqueta sem deformar a imagem. Como a arte disponivel e uma capa frontal de caixa, nao uma digitalizacao exata da etiqueta fisica, pode ser necessario refinar alguns pixels ou trocar por uma imagem especifica de etiqueta.
- O renderizador agora preserva a proporcao real do molde dentro da caixa visual solicitada. Isso evita distorcer templates com proporcoes diferentes.
- Esta alteracao foi inspecionada no codigo e no PNG. A compilacao integral e o teste visual no Windows ainda precisam ser confirmados pelo usuario.


## Ajuste adicional: centralizacao do carrossel
- O cartucho selecionado agora e posicionado a partir do centro real da saida do SDL Renderer, em vez de usar a coordenada horizontal fixa `x=260`.
- Os cartuchos vizinhos sao posicionados simetricamente ao redor do selecionado.
- O ajuste acompanha a dimensao atual da janela/renderizador.
- A logica da etiqueta foi mantida nesta etapa; a estrategia de buscar/preparar e armazenar etiquetas no estilo do Slot fica para a etapa seguinte, depois de validar a geometria e a centralizacao.


## Menu inicial (frontend)

O Fofita agora abre num menu; o jogo so roda depois de escolher uma ROM.

### Teclas
| Onde | Tecla | Acao |
|---|---|---|
| Menu (carrossel) | ← / → | trocar de cartucho |
| Menu (carrossel) | Enter ou X | iniciar o jogo em foco |
| Menu | ↑ | foco no cabecalho de consoles |
| Cabecalho | ← / → | trocar de console (seletor rapido) |
| Cabecalho | H | ocultar o console atual do menu (nunca o unico visivel) |
| Cabecalho | Shift+H | mostrar de volta todos os consoles ocultos |
| Cabecalho | ↓ ou Enter | voltar ao carrossel |
| Menu | ↓ (no carrossel) | foco no botao "Sair do Fofita"; Enter fecha o app |
| Jogo | Espaco ou Esc | volta ao menu sem fechar o app (descarrega o jogo e o nucleo) |

### Como funciona
- `console_registry.c`: cada console ganhou `caminho_core` e `caminho_icone` (campos finais). Um console so aparece no cabecalho se tiver `caminho_core` e o arquivo da DLL existir (hoje so o NES, `cores/fceumm_libretro.dll`). Para ativar outro console, basta preencher o `caminho_core` dele.
- Consoles ocultos ficam em `fofita_menu.cfg` (linhas `oculto=<id>`, ignorado pelo git).
- `ui_carousel.c`: o carrossel mostra so os jogos do console selecionado (`carousel_definir_console`) e desenha dentro da area que o menu manda.
- `libretro_core.c`: `unload_game()` chama `retro_unload_game`, libera a ROM e `close_core()` libera o nucleo. Cada jogo carrega o nucleo do seu console de novo.
- `ui_text.c`: fonte bitmap 5x7 embutida (sem SDL_ttf). Letras acentuadas aparecem sem acento ("Acao") e caracteres fora do Latin-1 viram "?".
- `ui_theme.c`: papel de parede, pasta do estilo de icones e cores num lugar so.
- `video_audio.c`: agora inicia `IMG_INIT_JPG` (o wallpaper e .jpg) e tem `descartar_textura_jogo()` / `limpar_audio()` para a volta ao menu.
- Arquivos novos para entrar na compilacao: `ui_menu.c`, `ui_text.c`, `ui_theme.c`.

### Assets (`assets/ui/`)
```
wallpapers/bk-01.jpg
consoles/<console>_icon.png        icones coloridos dos consoles (servem para qualquer estilo)
estilos/escuro/arrows/*.png        setas (icones pretos, para fundos claros)
estilos/escuro/icons/*.png         trofeu, engrenagem, funil, coracao, pincel
```
Um novo estilo (claro, colorido) e uma pasta nova em `assets/ui/estilos/` com os mesmos nomes de arquivo, mais uma entrada em `ui_theme.c`.

### Validacao
Neste ambiente nao ha SDL2 nem MinGW. Todos os arquivos passaram em `gcc -fsyntax-only -Wall -Wextra` contra cabecalhos simulados do SDL, e o menu real (ui_menu, ui_text, ui_carousel, console_registry) foi executado contra um SDL simulado em software, com capturas de tela conferidas (layout, carrossel, ocultar/restaurar console, persistencia do `.cfg`, acentos). **Nao testado:** compilacao com SDL2 real, audio/video do nucleo e o ciclo menu -> jogo -> menu no Windows.

## Pendente (organizado para depois)
- Barra lateral recolhivel (seta ^ oculta): troféu = RetroAchievements, engrenagem = configuracoes, funil = filtros da biblioteca, coracao = favoritos, pincel = personalizacao visual. Os PNGs ja estao em `assets/ui/estilos/escuro/icons/`.
- Personalizacao de wallpapers (inclusive animados) e de icones, com estilos pre-prontos (escuro para fundo claro, claro para fundo escuro, colorido). O ponto de entrada e `ui_theme.c`; falta a escolha do usuario e um seletor.
- Tela de configuracoes para gerenciar consoles ocultos (hoje: H / Shift+H no cabecalho).
- Acesso aos metadados do jogo pelo cartucho (antes era o Espaco sobre o jogo; Espaco agora volta ao menu).

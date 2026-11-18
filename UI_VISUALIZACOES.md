# Fofita — visualizacoes e menu compacto

Esta etapa substitui a lateral expansivel por uma unica pilula fixa e
prepara a escolha entre cartuchos e cards digitais.

## Interface

- A lateral permanece compacta; Tab alterna o foco entre lateral e carrossel.
- Page Up/Page Down entram na lateral e percorrem suas opcoes.
- Setas verticais percorrem as acoes da lateral; Enter confirma; Esc retorna.
- Os icones originais dos tres presets continuam em `ui/styles/`.
- O contraste automatico ja existente continua analisando o wallpaper.
- Os atalhos visuais na parte inferior foram removidos, mas seus comandos
  continuam ativos. O slot permanece reservado para animacao futura.
- Wallpaper, cores e visualizacao podem ser personalizados separadamente.

## Cartuchos e cards

Em **Aparencia > Visualizacao**, Enter alterna entre **Cartuchos** e **Cards**.
A escolha e salva em `fofita_menu.cfg`:

```ini
game_view=cartridge
```

ou

```ini
game_view=card
```

A ausencia da chave preserva o modo cartucho como padrao.

Arquivos:
- `ui_game_view.h/.c`: modo ativo, independente dos jogos.
- `ui_cards.h/.c`: desenho de um card com a capa carregada e fallback
  legivel quando a arte nao esta disponivel.
- `ui_carousel.c`: continua gerenciando selecao, indices, filtros e
  cores; so escolhe o renderizador de cartucho ou card.
- `ui_menu.c`: menu Aparencia, configuracao e componente lateral.

Os cards nao carregam ou copiam ROMs; reutilizam o catalogo ja existente.
Jogar continua chamando a mesma integracao Libretro.

## Validacao

No Codespaces:
```bash
bash .devcontainer/check-linux.sh
```

No GitHub Actions, a compilacao Windows inclui os novos modulos porque
o workflow usa `./*.c`. Um build verde confirma a compilacao, nao a
fidelidade grafica: comparar no Windows com os screenshots de referencia.
Os artifacts nao distribuem ROMs.

## Pendente

- Refinar a aparencia final dos cards a partir de referencias visuais.
- Verificar a legibilidade em wallpapers claros e escuros no Windows.
- Integrar a futura animacao de encaixe.
- Evoluir o tratamento de favoritos e filtros.

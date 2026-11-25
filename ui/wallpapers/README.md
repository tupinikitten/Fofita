# Wallpapers da Fofita

Esta pasta aceita imagens **PNG, JPG/JPEG e GIF**. O menu fica em
**Customizacao (balde) > Wallpapers**.

## Universal ou por console

1. Escolha o console desejado no carrossel superior.
2. Abra **Customizacao > Wallpapers**.
3. Na primeira opcao, selecione o destino:
   - **Todos os consoles**: define o wallpaper universal.
   - **Somente [console atual]**: cria uma excecao para esse console.
4. Em **Escolher wallpaper**, navegue pelos arquivos de `ui/wallpapers/`
   com as setas; Enter aplica ao destino escolhido e Esc cancela.
5. **Usar wallpaper universal neste console** remove a excecao, sem apagar
   a imagem. Para o destino universal, **Restaurar wallpaper universal padrao**
   volta a `ui/wallpapers/bk-01.jpg`.

Nao e necessario criar uma pasta de wallpaper para cada console: a Fofita
armazena apenas o caminho do arquivo escolhido para cada ID de sistema
em `fofita_menu.cfg`. O mesmo PNG ou GIF pode ser reutilizado em varios
consoles, e o wallpaper muda automaticamente ao alternar de sistema.

A opcao **GIFs animados** vale para toda a interface. Quando desligada, um
GIF e exibido estaticamente. Durante a emulacao a interface deixa de
desenhar o menu, logo a animacao nao consome quadros de renderizacao.

## Formatos e qualidade

- **1920 x 1080 (16:9)** e a dimensao recomendada para uma imagem estatica.
- O programa usa enquadramento **cobrir**, sem deformar. Imagens em outras
  proporcoes podem ser recortadas nas bordas.
- A previa de um GIF exibe o primeiro quadro; a animacao toca apos aplicar.
- O contraste automatico da barra e dos titulos usa o primeiro quadro
  do GIF. Isso evita alternancias de cor a cada quadro.
- GIFs devem ter quadros pequenos e otimizados. A animacao suporta no
  maximo 96 quadros, dimensoes de ate 1920 x 1080, 16 MiB de arquivo
  e aproximadamente 64 MiB de texturas de quadros.
- Animacoes acima dos limites, com erro de decodificacao ou em versoes
  do SDL2_image sem suporte a GIF animado, tentam aparecer como imagem
  estatica em vez de bloquear a interface.
- A decodificacao de GIF animado depende de **SDL2_image 2.6+**.
- A lista de wallpapers ainda tem um limite de 64 arquivos; ampliar
  essa biblioteca de maneira dinamica sera uma etapa separada.

As mudancas de wallpaper nao alteram ROMs, cartuchos nem o backend.

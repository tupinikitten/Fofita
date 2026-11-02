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

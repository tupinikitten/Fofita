# Icones oficiais dos consoles

Esta pasta contem as artes oficiais do cabecalho da Fofita. Os arquivos sao
substituiveis gradualmente: nao e necessario refazer todos os consoles.

## Template de criacao

- Tamanho recomendado para a arte-fonte: **512 x 512 px (PNG RGBA)**.
- Fundo transparente e margens de seguranca semelhantes entre as artes.
- Centralize o desenho no canvas sem alterar sua proporcao natural.
- O template e uma recomendacao, **nao uma exigencia**. PNGs com outras
  resolucoes e proporcoes razoaveis tambem podem ser usados.
- Mantenha os nomes oficiais, como `nes_icon.png`, `gb_icon.png`,
  `gbc_icon.png`, `ps1_icon.png`, etc. Consulte `console_registry.c`
  para os caminhos usados por cada sistema.

## Otimizacao automatica

Ao abrir o programa ou recarregar os pacotes de consoles, a Fofita carrega
cada PNG, faz a reducao com **media de area com transparencia ponderada** e
libera a superficie original. Apenas uma textura compacta fica na memoria.

A textura intermediaria cabe em ate 168 x 108 px, preservando a proporcao.
No cabecalho, o icone selecionado e exibido em caixa de ate 84 x 54 px;
os demais tem caixas menores. O SDL aplica a escala linear final.

Isso melhora o aspecto de artes com muitos detalhes quando reduzidas.
Nao garante que microtextos continuem legiveis em 84 px de largura:
prefira silhuetas fortes e detalhes suficientemente espessos.

O redimensionamento nao altera o PNG em disco e **nao exige recompilar**
quando a substituicao for feita na pasta local do programa; reinicie o
aplicativo ou use o submenu para atualizar os pacotes de consoles.

Pacotes opcionais de usuarios continuam em `ui/consoles/styles/` e usam
a mesma otimizacao. Nao ha necessidade de mover as artes oficiais para la.

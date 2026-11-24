# Capas exclusivas dos cards digitais

O local recomendado agora e **`labels/<console>/cards/`**, dentro da pasta das etiquetas de cada sistema. O caminho antigo `labels/cards/<console>/` continua aceito como fallback para bibliotecas anteriores.

## Exemplo

```text
labels/
└── nes/
    ├── Metroid (USA).png           # Etiqueta do cartucho fisico
    ├── Legend of Zelda, The (USA).png
    └── cards/
        ├── Metroid (USA).png       # Arte exclusiva do card digital
        └── Legend of Zelda, The (USA).png
```

**O nome da capa digital pode ser identico ao da etiqueta fisica.** Sao arquivos distintos em pastas diferentes. O nome deve ser igual ao nome da ROM sem extensao, respeitando espacos e pontuacao. Exemplo: `roms/nes/Metroid (USA).nes` usa `labels/nes/cards/Metroid (USA).png`.

A Fofita procura na ordem:
1. `labels/<console>/cards/<nome>.png` (preferencia)
2. `labels/cards/<console>/<nome>.png` (legado)
3. etiqueta tradicional ja carregada de `labels/<console>/<nome>.png`

O template recomendado e **512 x 512 px, formato PNG, proporcao 1:1**, com margem de seguranca de ~32 px. O frame de destaque e os cantos arredondados sao desenhados pela Fofita; nao precisam fazer parte da arte. Outras resolucoes tambem funcionam.

Depois de adicionar novos PNGs com a Fofita aberta, pressione **F5** no modo Cards para recarregar as imagens do console ativo, ou alterne de Cartuchos para Cards em Aparencia. O editor manual de enquadramento permanece disponivel via **E**, com ajustes gravados em `fofita_cards.cfg` por ROM e sem alterar os PNGs originais.

## Layout dos cards

Em **Customizacao > Visualizacao dos jogos**, existem opcoes separadas para:
- **3 cards** (padrao): um principal e vizinhos
- **5 cards**: ate dois vizinhos de cada lado, se houver jogos suficientes
- **Grade**: ate duas fileiras paginadas, com selecao por teclado, controle ou mouse

Selecionar um dos tres layouts ativa o modo Cards; a opcao **Exibicao: Cartuchos / Cards** permite voltar ao visual fisico sem perder o layout escolhido. A preferencia fica salva em `fofita_menu.cfg`.

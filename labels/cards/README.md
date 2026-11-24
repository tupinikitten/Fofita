# Capas exclusivas dos cards digitais

A Fofita procura aqui uma arte opcional para cada jogo em visualizacao **Cards**.
Os cartuchos fisicos continuam a usar as capas de `labels/<console>/`.

## Template recomendado

- **512 x 512 pixels, proporcao 1:1**, formato PNG
- Margem visual de seguranca: 32 px em cada lado; elementos importantes preferencialmente dentro de 448 x 448 px
- Cantos arredondados e borda de selecao sao desenhados pelo programa: a arte nao precisa incorpora-los
- Preferir imagens sem molduras externas; evitar textos no limite do recorte
- O programa aceita outras resolucoes: 512 x 512 e recomendacao, nao requisito

## Pastas e nomes

Os arquivos ficam em `labels/cards/<id-console>/<nome-exato-da-ROM-sem-extensao>.png`.

Exemplo para NES:

```text
labels/
├── nes/
│   └── Metroid (USA).png
└── cards/
    └── nes/
        └── Metroid (USA).png
```

O nome precisa corresponder ao nome da ROM (sem a extensao), como ocorre nas etiquetas fisicas; respeite espacos e pontuacao. Quando a arte digital nao existir ou nao puder ser aberta, a Fofita continua exibindo a etiqueta tradicional, sem erro fatal.

O editor manual dos cards (tecla **E** ao selecionar um jogo em modo Cards) continua disponivel para ajustes de zoom e enquadramento, salvos separadamente em `fofita_cards.cfg`. Nenhuma dessas opcoes modifica os arquivos PNG originais.

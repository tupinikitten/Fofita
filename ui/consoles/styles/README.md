# Pacotes pessoais de icones dos consoles

Em **Customizacao > Consoles**, e possivel escolher um pacote de icones para a faixa superior.

Crie uma pasta com um nome de letras, numeros, `_` ou `-` (ate 32 caracteres), por exemplo `retro`:

```text
ui/consoles/styles/retro/
├── nes_icon.png
├── gb_icon.png
├── gbc_icon.png
└── psp_icon.png
```

Cada arquivo se chama `<id-do-console>_icon.png` e o ID precisa ser o definido pelo registro do console, por exemplo `nes`, `gb`, `gbc`, `gba`, `ps1`, `psp`, `nds`, `atari2600`. Para IDs adicionais, consulte `console_registry.c`.

**Nao e necessario incluir todos os consoles.** Um arquivo `nes_icon.png` ja basta para a Fofita reconhecer o pacote; qualquer console sem PNG proprio continua usando `ui/consoles/<id>_icon.png`, quando disponivel.

Recomendacao: PNG quadrado 512 x 512, com fundo transparente e margens semelhantes. A imagem e encaixada na caixa do console, sem distorcer. Para aplicar imagens adicionadas com o programa aberto, use **Atualizar pacotes** no submenu, depois selecione a pasta pelo nome.

Os pacotes de consoles sao independentes dos temas da sidebar em `ui/styles/`; os ajustes sao salvos em `fofita_menu.cfg`. Nao crie um novo console apenas adicionando um PNG: esta personalizacao altera somente a aparencia dos sistemas ja registrados, sem instalar cores nem mudar a emulacao.

A mesma tela tambem contem **Consoles no topo: Mostrar / Ocultar**. Essa preferencia controla somente a visibilidade da faixa, nao o cadastro dos sistemas nem a integracao com o backend.

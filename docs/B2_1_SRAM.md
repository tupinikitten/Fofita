# Fofita — B2.1: progresso interno (SRAM) persistente

**Branch:** `feature/backend-foundation`  
**Referência visual preservada:** commit `666f0aa`

## Objetivo e funcionamento

Adicionar **salvamentos internos dos jogos** (SRAM / memory save), **não** savestates. O Fofita agora:

1. Consulta `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)` e `retro_get_memory_size` após carregar a ROM.
2. Se o core expuser uma SRAM válida, tenta restaurar o arquivo `.srm` correspondente antes do primeiro frame.
3. Enquanto o jogo estiver ativo, verifica mudanças periodicamente, aproximadamente a cada 30 segundos, e grava somente se os bytes mudaram.
4. Ao sair do jogo ou fechar o aplicativo normalmente, grava antes de `retro_unload_game`.
5. Usa arquivo temporário e substituição por renomeação (Windows: `MoveFileExA` com `MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`; Linux: `rename`). Se a escrita/substituição falhar, o arquivo anterior permanece.
6. Impede sobrescrever um arquivo de SRAM cujo tamanho não corresponde ao tamanho informado pelo core. A SRAM do jogo não é modificada nesses casos; a escrita é bloqueada na sessão e uma mensagem aparece no console.

## Caminhos

O diretório de usuário é decidido por `SDL_GetPrefPath("TupiniKitten","Fofita")` e contém `saves/`, introduzido na B1. Para os cores atuais, os saves seguem o formato:

```text
saves/
  fceumm_libretro/
    nome-do-jogo-<hash-do-caminho>.srm
  gambatte_libretro/
    nome-do-jogo-<hash-do-caminho>.srm
```

O nome é higienizado e o hash estável do caminho evita que dois jogos com o mesmo nome sejam confundidos. A localização da ROM deve permanecer a mesma para continuar usando o save existente. **Não mover nem renomear ROMs depois de começar a salvar sem antes fazer backup do diretório de saves.**

Cores Gambatte de GB e GBC compartilham o mesmo diretório de core, mas jogos com caminhos diferentes possuem arquivos distintos.

## Cuidados e limitações

- A primeira versão cobre **SRAM** exposta diretamente pelo core; não implementa `RETRO_MEMORY_RTC`, memory cards próprios do core, conteúdo compactado nem savestates.
- Os saves são escritos no disco **fora da pasta de ROMs** e fora do repositório Git; não embarcar saves pessoais nos pacotes Windows.
- A gravação em arquivo temporário protege o arquivo anterior em caso de falha de escrita. **Backups históricos (.bak), checksum e recuperação de arquivos inválidos** serão tratados numa etapa posterior da B2. Não prometer proteção contra toda perda de energia.
- Arquivos inacessíveis ou com tamanho incompatível não são automaticamente corrigidos nem apagados; o jogo pode continuar, mas a persistência fica desativada até que o arquivo seja inspecionado.
- O limite inicial por região SRAM é 32 MiB, evitando alocações absurdas causadas por cores defeituosos.
- Os testes unitários usam RAM simulada e não substituem testes reais nos jogos NES/GB/GBC.
- Não houve mudança em `ui_menu.*`, `ui_carousel.*`, `video_audio.*` ou no layout.

## Testes manuais para validar B2.1

1. Baixar o build B2.1 e instalá-lo em uma pasta separada, mantendo o core Gambatte em `cores/`.
2. Abrir um jogo com salvamento interno (por exemplo um RPG de GB/GBC) e **salvar pelo próprio menu do jogo**.
3. Voltar ao dashboard com Esc, fechar o Fofita, abrir de novo e entrar no jogo; verificar a opção de continuar com progresso.
4. Fazer o mesmo com um jogo NES **que tenha SRAM/bateria**; nem todos os jogos NES salvam progresso interno.
5. Verificar a criação de `.srm` no diretório de dados do usuário e testar um segundo jogo para confirmar separação.
6. Testar voltar ao menu várias vezes; áudio, controles, GB/GBC e visual do NES devem permanecer iguais.

**Próxima entrega B2.2:** savestates com três slots e identificação de versão/core por jogo; depois integrar ao menu rápido. A melhoria gráfica de filtros/shaders permanece para F4/B4.

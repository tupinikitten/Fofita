# Fofita — B2.2: savestates em 3 slots

**Branch de trabalho:** `feature/backend-foundation`. A interface aprovada
(dashboard, cartuchos/cards, sidebar, Aparência) permanece inalterada.

## O que foi implementado

Savestates guardam o **estado instantâneo do emulador** e não dependem do
"Save" que aparece dentro do Pokémon/Zelda. São um recurso diferente da
SRAM automática da B2.1; não desativam nem substituem os saves internos.

- Três slots **1, 2, 3** por ROM e core, separados da SRAM.
- O slot selecionado começa em 1 a cada abertura de jogo.
- Os arquivos ficam em `states/<nome-do-core>/<jogo>-<hash-do-caminho>-slotN.fst`,
  dentro da pasta de dados do usuário (SDL_GetPrefPath).
- Escrita em arquivo temporário + substituição por renomeação. Ao substituir
  um slot existente e válido, é preservado seu conteúdo anterior em `.bak`.
- Cabeçalho de versão 1, tamanho do estado, fingerprints dos **bytes da ROM e
  do arquivo do core**, além de checksum dos dados. Um arquivo truncado,
  corrompido ou incompatível não é entregue a `retro_unserialize`.
- Arquivos corrompidos ou incompatíveis não são sobrescritos automaticamente.
  Faça um backup e remova/renomeie o arquivo com problema antes de reutilizar
  esse slot.
- Limite de 64 MiB por estado. Cores sem suporte a serialização informam
  indisponibilidade em vez de criar um arquivo inválido.
- Não permite salvar antes do primeiro quadro emulado.
- Um savestate restaurado pode retornar o progresso interno (SRAM) a um
  instante anterior. **Não carregue um estado antigo sem entender que isso
  pode alterar dados de salvamento da partida.**

## Atalhos provisórios (somente dentro de jogo)

| Tecla | Função |
|---|---|
| **F5** | Salvar no slot atual |
| **F6** | Próximo slot (1 → 2 → 3 → 1) |
| **F8** | Carregar o slot atual |
| **Esc / Espaço** | Voltar ao dashboard, como antes |

O resultado da operação aparece brevemente no **título da janela**.
Os eventos não se repetem quando a tecla é mantida pressionada.
F5, F6 e F8 ficam reservadas e não podem ser atribuídas a botões do jogo,
para evitar colisão com o remapeamento existente. Ao restaurar, a fila de
áudio antiga é limpa.

**Não alteramos a interface gráfica**: o submenu/menu rápido para selecionar,
salvar e carregar estados visualmente fica para a próxima integração de
interface (B3). O teclado é suficiente para validar a B2.2.

## Casos de teste

1. Use a versão compilada da branch e copie as DLLs do core e as ROMs.
2. Entre no Pokémon GB; caminhe até uma posição e pressione **F5** (slot 1).
3. Caminhe até outra posição, **F6** (slot 2) e **F5**.
4. Ande mais; pressione **F8** no slot 2 e verifique a posição.
5. Pressione **F6** duas vezes para retornar ao slot 1, depois **F8**.
   Confirme que voltou à primeira posição.
6. Volte com Esc ao dashboard; entre no Zelda NES, salve no slot 1.
   Reabra Pokémon: seus três slots devem permanecer separados.
7. Feche Fofita, reabra os jogos e repita F8: os estados devem persistir.
8. Confirme que os salvamentos internos SRAM continuam funcionando normalmente.

**Não enviar saves pessoais ou ROMs ao repositório.**
Os testes GitHub Actions exercitam arquivos simulados, checksums, isolamentos,
backups, seleção de slots e compilação Windows. A compatibilidade real com
Gambatte/FCEUmm ainda depende dos testes manuais da usuária.

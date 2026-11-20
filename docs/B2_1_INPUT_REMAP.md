# Fofita — Remapeamento de teclado e controle

## Objetivo

Adicionar **Configurações > Remapear controles** sem criar uma nova ação
na barra lateral ou mudar o desenho aprovado do dashboard.

## Experiência

- **Configurações > Remapear controles** abre a lista de Cima, Baixo,
  Esquerda, Direita, A, B, Start e Select.
- A linha **Dispositivo** alterna entre **Teclado** e **Controle**.
- Escolher uma ação e confirmar entra em modo de captura:
  pressione a nova tecla ou o novo botão físico. **Esc cancela**.
- Se o botão/tecla já estiver vinculado a outra ação, elas trocam de lugar.
  Isso evita que o jogo perca uma ação e que duas ações fiquem simultâneas
  no mesmo botão.
- **Restaurar botões padrão** retorna aos controles anteriores.
- As opções são persistidas em `fofita_controles.cfg`, na pasta de dados
  de usuário criada por SDL_GetPrefPath ("TupiniKitten"/"Fofita").
- O analógico esquerdo continua servindo como direcional adicional,
  independentemente do mapeamento dos botões do D-pad.
- Para o teclado, **Esc e Espaço ficam reservados para voltar ao Fofita**
  e não podem ser atribuídos ao jogo.
- Para gamepad, **Back/Select + Start físicos juntos** continuam sendo
  o comando de saída de emergência, mesmo que essas teclas tenham sido
  remapeadas para outras funções no jogo.
- As teclas/botões de navegação no **dashboard não são remapeadas**;
  somente os comandos enviados aos cores Libretro.

## Padrões de teclado

| Ação | Tecla |
| --- | --- |
| Direções | Setas |
| A | X |
| B | Z |
| Start | Enter |
| Select | Shift direito |

## Padrões do controle

| Ação | Botão |
| --- | --- |
| Direções | D-pad |
| A | A |
| B | B |
| Start | Start |
| Select | Back/Select |

## Testes

1. Abrir Configurações, escolher **Remapear controles**, mudar A para
   outra tecla, iniciar NES/GB/GBC e conferir o novo mapeamento.
2. Confirmar que B em menus de Zelda/Pokémon não fecha o Fofita.
3. Reiniciar Fofita: mapeamento permanece e pode ser restaurado.
4. Conectar gamepad, escolher dispositivo Controle e alterar A ou B.
5. Verificar que Esc/Espaço e Back+Start continuam saindo do jogo.
6. Comparar aparência do menu e moldura com a versão aprovada.

A UI usa a mesma geometria, paleta e rotina de seleção existente.
Os testes do GitHub Actions verificam a compilação e a lógica de
persistência/mapeamento; a navegação visual exige teste no Windows.

## Limites atuais

- Esta primeira versão oferece um perfil global para teclado e um para
  gamepad; perfis por console/jogo e controles múltiplos ficam para
  uma futura fase de configuração mais avançada.
- Os eixos analógicos têm sensibilidade fixa, separada do D-pad.
- Não permite remapear comandos exclusivos do menu, para não impedir
  acesso a Configurações.

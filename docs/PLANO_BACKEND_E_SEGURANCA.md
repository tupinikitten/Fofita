# Fofita — Plano do Backend, Emulação e Segurança

> **Status:** planejamento; implementação não iniciada por este documento.
> **Base verificada:** `feature/ui-views-linux-preview`, commit `666f0aa` (19/11/2026).
> **Documento complementar:** [Plano do Frontend e Integração Git](PLANO_FRONTEND_E_INTEGRACAO_GIT.md).

## 1. Objetivo e limites

Evoluir o Fofita de um launcher com execução Libretro inicial para um frontend de emulação com mais sistemas, saves, savestates, cheats, configurações por core, overlays/frames, shaders e integração futura com RetroAchievements. A interface visual aprovada não deve ser reescrita por mudanças do backend.

**O que existe no código atual:**
- `console_registry.c` registra **16 sistemas**, mas apenas NES aponta explicitamente para `cores/fceumm_libretro.dll` e é ativado pelo teste de existência do arquivo.
- `libretro_core.c` usa `LoadLibraryA` e `GetProcAddress` (Windows), inicializa o core, carrega a ROM inteira na RAM e chama `retro_run`; o callback de ambiente implementa essencialmente formato de pixel e geometria.
- `mini_frontend.c` alterna entre o menu e o jogo, sem menu rápido em execução.
- `input.c` cobre principalmente jogador 1 e parte dos botões.
- `video_audio.c` desenha vídeo com SDL2, ainda sem composição de overlays ou shaders.
- Não há implementação consolidada de SRAM persistida, savestates, cheats ou conta RetroAchievements. O build Windows **não prova** funcionamento em Nintendo Switch.

## 2. Arquitetura por responsabilidades

```text
Interface (menu principal e menu rápido)
       |
       v
game_session / capacidades do jogo
       |
       +--> core_manager / plataforma (Windows ou Switch)
       |           |
       |           +--> libretro_core (callbacks, memória, áudio/vídeo)
       |
       +--> save_manager (SRAM, estados, backups)
       +--> cheat_manager (códigos e regras de compatibilidade)
       +--> game_video (composição, frames e filtros)
       +--> achievements_adapter (rcheevos rc_client)
                      |
                      +--> network_https (TLS, timeout, validação)
                      +--> credential_store (segredos por plataforma)
```

Separação obrigatória: o frontend **solicita ações** e exibe estados; o backend decide se uma capacidade está disponível e responde com sucesso/erro. Funcionalidades inexistentes no core devem ser desabilitadas sem travar o aplicativo.

## 3. Etapas de implementação

### B0 — Fundamentos e testes de regressão

- [ ] Criar `game_session.h/.c` com estados `MENU / STARTING / PLAYING / PAUSED / STOPPING / ERROR` e fluxo de retorno seguro.
- [ ] Introduzir uma estrutura consultável de capacidades: `can_save_state`, `can_cheat`, `has_sram`, `supports_disk`, `supports_achievements`, `supports_shaders` etc. Não confundir função existente com suporte real do core.
- [ ] Colocar logs e erros do Libretro atrás de funções próprias; evitar dados sensíveis nos logs futuros.
- [ ] Separar adaptador Windows (`LoadLibrary`) do caminho de carregamento para Switch/homebrew; verificar mecanismo de execução de cores na plataforma de destino antes de prometer suporte.
- [ ] Repetir testes: abrir Fofita, jogar NES, controles, áudio, retorno ao menu e execução repetida sem vazamento.

**Aceite:** mesmo comportamento do build aprovado; estrutura pronta para módulos novos, sem mudanças visuais.

### B1 — Ciclo de vida dos cores e expansão de consoles

- [ ] Implementar funções essenciais e opcionais do ciclo Libretro, descoberta das capacidades, informações de sistema e ambiente; consultar a API oficial antes de ativar cada callback.
- [ ] Implementar diretórios controlados de `system/BIOS`, `saves`, `states` e `cache`, com caminhos portáveis e validação de acesso.
- [ ] Respeitar o formato de conteúdo exigido pelo core (ROM em memória **ou caminho**, inclusive cores com `need_fullpath`); evitar carregar imagens grandes de disco inteiras em RAM.
- [ ] Expandir por lotes: **GB/GBC, GBA, SNES, Master System e Mega Drive**; depois sistemas de maior custo e dependências (PS1, N64, DS, PSP, Dreamcast e Arcade).
- [ ] Para cada sistema, registrar: core selecionado, extensões, BIOS necessárias, formato de conteúdo, limite de memória e desempenho medido.
- [ ] Auditar licença e distribuição de cada core e dependência; não incluir ROMs comerciais nem BIOS de terceiros sem autorização.

**Aceite:** cada console aparece como jogável apenas quando o core e os requisitos mínimos realmente estiverem disponíveis; testes com conteúdo livre/legal.

### B2 — Salvamentos normais e savestates

- [ ] Persistir SRAM/memória de salvamento (`retro_get_memory_data` / `retro_get_memory_size`, quando aplicável), separadamente dos savestates.
- [ ] Adicionar `save_manager.h/.c` com caminhos por sistema, jogo, core e slot; gravação por arquivo temporário + renomeação, validação de tamanho e backup.
- [ ] Usar `retro_serialize_size`, `retro_serialize` e `retro_unserialize` somente quando o core permitir; expor incompatibilidade de estado/versão de core em vez de carregar às cegas.
- [ ] Testar: salvar, desligar, reiniciar, carregar, sair em segurança, falha de escrita e arquivo corrompido.
- [ ] Definir política de autosave opcional, sem assumir que todos os cores suportam serialização.

**Aceite:** um jogo de teste retoma corretamente a SRAM e três slots de estado podem ser gravados/abertos sem corromper arquivos anteriores.

### B3 — Controles, menu rápido e cheats

- [ ] Separar captura de entrada do menu e do jogo, admitir múltiplos jogadores e mapear botões/atalhos sem colisões.
- [ ] Introduzir `cheat_manager.h/.c` para cheats que o core aceite, via `retro_cheat_reset` e `retro_cheat_set`; esconder controles indisponíveis.
- [ ] Integrar comandos de pausar, continuar, reiniciar, capturar estado e voltar ao Fofita ao contrato do menu rápido.
- [ ] Ao ativar RetroAchievements Hardcore no futuro: **bloquear ou desativar corretamente** cheats, savestates e outros recursos proibidos pelo modo; sinalizar claramente a troca de modo e seguir as regras atuais da plataforma.

**Aceite:** atalhos confiáveis e nenhuma alteração de memória/estado sem ação explícita da pessoa usuária.

### B4 — Vídeo, frames, overlays e shaders

- [ ] Separar `game_video.h/.c` para apresentação do frame, aspect ratio, integer scaling, letterbox e composição.
- [ ] Implementar primeiro **frames PNG** e overlays simples (camadas decorativas sem alterar memória/execução do jogo).
- [ ] Só depois estudar filtros e shaders: distinguir filtro SDL2 de verdadeiro pipeline de GPU; selecionar API compatível com Windows e, futuramente, Switch.
- [ ] Manter opção **Sem efeito** e fallback caso o shader não seja suportado.
- [ ] Medir tempo de frame, RAM e consumo em portátil/dock; evitar prometer desempenho sem teste em hardware.

**Aceite:** frame/overlay pode ser ativado/desativado sem interferir no áudio e na taxa de quadros do jogo de referência.

## 4. RetroAchievements e segurança desde o desenho da arquitetura

Esta etapa pode ser implementada **depois** de B0–B3, mas os contratos e limites de segurança devem ser reservados desde agora.

### B5.1 — Escolha técnica e API correta

- Preferir a biblioteca C oficial **rcheevos**, especialmente a API `rc_client` para integração de emuladores. Travar em uma versão/release revisada; verificar licença, atualizações e integridade das dependências.
- **Atenção:** `rcheevos` não implementa HTTP/HTTPS. O Fofita precisa fornecer uma rotina de rede e entregar respostas/callbacks ao cliente.
- Não confundir **Web API** pública, **Connect API** de emuladores e o guia de aplicações *standalone*. Não embarcar uma API key geral do projeto dentro do executável.
- Seguir o login de `rc_client` com credenciais fornecidas pela pessoa e token de sessão/lembrança retornado pela integração. Não inventar OAuth2 disponível para essa finalidade.
- Implementar hash/identificação do jogo, mapa de leitura de memória por core/sistema, início/fim de sessão, eventos e notificações; falhas de conexão nunca devem impedir jogar offline.

### B5.2 — Transporte HTTPS e resistência a falhas

Criar `network_https.h/.c` como interface única de rede, independente do renderer e do menu.

**Requisitos de segurança mínimos:**
1. **Somente HTTPS** para autenticação e chamadas com tokens; validar certificado, cadeia de confiança e hostname. Não desabilitar verificação TLS, nem aceitar redirecionamento para HTTP.
2. Timeouts explícitos; cancelamento no logout/fechamento, retries limitados com backoff (não reenviar indefinidamente requisições sensíveis), limites de tamanho de resposta e tratamento de códigos HTTP.
3. `User-Agent` identificando Fofita, versão e plataforma conforme a documentação de `rc_client`.
4. Nunca registrar senha, token, cabeçalho de autorização ou URL com parâmetros secretos; sanitizar mensagens de erro, telemetria e logs de CI.
5. Tratar respostas externas como dados não confiáveis: validar tamanho, schema, limites de strings e URLs de imagens antes de carregar ou exibir.
6. Não bloquear o loop do jogo com chamadas de rede: execução assíncrona, callbacks com ciclo de vida definido e passagem segura para a thread principal.
7. Respeitar rate limits, cache com expiração e política de falha offline.

**Credenciais:** usar o gerenciador seguro do Windows quando disponível e avaliar mecanismo de armazenamento seguro na plataforma Switch/libnx antes de oferecer “lembrar de mim”. Se não houver proteção persistente adequada no ambiente homebrew, preferir **sessão em memória** e exigir novo login. Nunca salvar senha/token em `fofita_menu.cfg`, JSON de UI, screenshots, Git ou artifact.

**Ameaças previstas:** Wi-Fi hostil/MITM, arquivo de configuração roubado, logs vazando segredos, redirecionamento de domínio, respostas grandes/corrompidas, interrupção da conexão, sessão vencida e falha de energia durante escrita.

### B5.3 — Privacidade e regras de conquistas

- Login e conexão devem ser **opt-in**, com botão de desconexão e sessão claramente identificada.
- Mostrar de forma clara o que é enviado: identificação da conta, jogo em uso, eventos e status exigidos pelo serviço. Especificar retenção de cache local e mecanismo de limpeza.
- Não enviar arquivos de ROM, saves nem dados pessoais desnecessários.
- Exibir Softcore/Hardcore e restrições de cheats/savestates; não registrar conquistas indevidas e testar com modos seguros antes de ativar Hardcore.
- Considerar termos de uso, política de privacidade e atualização do cliente caso a API mude.

**Aceite de segurança antes do lançamento:** login/token testados com certificado inválido, falta de rede, timeout, token revogado, logout e troca de usuário; inspeção do repositório, binários e logs para ausência de segredos; teste de reconexão sem perda de progresso local.

## 5. Sequência sugerida de entregas

| Ordem | Entrega | Dependência | Teste mínimo |
| --- | --- | --- | --- |
| 1 | B0: sessão e capacidades | Build atual | NES + retorno ao menu |
| 2 | B1: abstração de core | B0 | NES e um novo sistema funcionando |
| 3 | B2: SRAM/savestates | B1 | reinício, slots e erros de I/O |
| 4 | B3: input/cheats/menu rápido | B1–B2 + contrato UI | pausa, comandos e bloqueios |
| 5 | B4: vídeo/overlays | B1 | desempenho e fallback |
| 6 | B5: RetroAchievements + HTTPS | B0–B3 + rede multiplataforma | suíte de segurança e offline |

A sequência poderá mudar conforme compatibilidade dos cores; cada capacidade deve ser implementada incrementalmente, não como um pacote gigante.

## 6. Integração Git e proteção do build aprovado

- **Ponto de partida:** commit `666f0aa` — build Windows aprovado do menu; não sobrescrever essa referência.
- **Backend:** nova branch `feature/backend-foundation` criada desse commit. Mudanças prioritariamente em `libretro_core.*`, `console_registry.*`, `input.*` e novos módulos.
- **Frontend:** branch `feature/ui-next` criada do mesmo ponto. Contratos em headers estáveis; evitar edições simultâneas em `mini_frontend.c` e `video_audio.c`.
- **Integração:** criar branch `integration/next` a partir da mesma base e integrar as frentes por PRs pequenos, depois de verificações de CI e testes manuais.
- **Release:** atualizar `main` somente depois de smoke tests da integração no Windows e, quando existir, build/teste Switch. Sem force push, sem apagar histórico e sem alterar o executável-base para implementar documentação.

Detalhes operacionais, comandos Git, conflitos e critérios conjuntos estão no [Plano do Frontend e Integração Git](PLANO_FRONTEND_E_INTEGRACAO_GIT.md).

## 7. Referências oficiais para a implementação

- [Integração rcheevos/rc_client](https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration)
- [Biblioteca rcheevos (README)](https://github.com/RetroAchievements/rcheevos)
- [Documentação RetroAchievements — distinção Web API / integração](https://api-docs.retroachievements.org/)
- [Cabeçalho oficial Libretro](https://github.com/libretro/libretro-common/blob/master/include/libretro.h)
- [libnx, ambiente homebrew para Switch](https://github.com/switchbrew/libnx)

> **Este documento descreve trabalho futuro.** Nenhuma credencial, conexão externa ou funcionalidade de conquistas foi adicionada ao executável apenas por criar este plano.

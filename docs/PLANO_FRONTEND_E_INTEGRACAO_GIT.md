# Fofita — Plano do Frontend e Integração pelo Git

> **Status:** planejamento e critérios de aceitação; nenhuma alteração de UI é feita por este documento.
> **Referência visual estável:** `feature/ui-views-linux-preview`, commit `666f0aa`, build Windows aprovado em 19/11/2026.
> **Documento complementar:** [Plano do Backend, Emulação e Segurança](PLANO_BACKEND_E_SEGURANCA.md).

## 1. Princípio do projeto

Preservar a interface minimalista do Fofita enquanto a emulação ganha recursos. O menu principal deve continuar rápido, personalizável e independente dos sistemas, e as funcionalidades avançadas devem aparecer **durante o jogo** em um menu rápido próprio.

**Decisões visuais já tomadas:**
- Sidebar **fixa, compacta e não expansível**, com seis ações: Conquistas, Configurações, Filtros, Favoritos, Aparência e Sair.
- Presets `Light` (ícones claros para wallpapers escuros), `Dark` (escuros para wallpapers claros) e `Colorful` (cores alternativas próprias). O modo automático escolhe contraste conforme wallpaper.
- Wallpaper independente da escolha entre cartuchos/cards e do estilo dos ícones.
- Cartucho físico como visualização padrão; cards digitais opcionais (`ui_game_view`), preferência persistida em `fofita_menu.cfg`.
- Sem rodapé de botões visuais; área inferior reservada para animação futura do encaixe.
- **Não reconstruir a moldura de seleção por outra rotina sem teste visual**. Os testes anteriores mostraram regressões por contornos que preenchiam o centro. A versão aprovada é a referência a conservar.

## 2. Frentes e entregas do frontend

### F0 — Congelar referência, medir e documentar

- [ ] Salvar screenshots de referência de Light/Dark/Colorful e Configurações/Aparência nos wallpapers claro e escuro.
- [ ] Definir checklist visual: moldura sem preenchimento, espessura aprovada, centro dos ícones, legibilidade e ausência de pixels/cantos estranhos.
- [ ] Usar 1280×720 como coordenadas lógicas atuais e verificar saída do SDL2 em 720p e 1080p, sem assumir que o Switch já foi testado.
- [ ] Validar controle, teclado e mouse; lembrar que o build do GitHub Actions apenas verifica compilação, não aprovação visual.

**Aceite:** capturas do build `666f0aa` são o padrão para comparação de cada PR futuro.

### F1 — Dois estilos de ícones dos consoles

Criar alternância independente entre:
1. **Flat miniatura** — representação colorida do console.
2. **Silhueta minimalista** — representação simples de alto contraste.

- [ ] Organizar os assets em `ui/consoles/flat/` e `ui/consoles/silhouette/`, com nomes padronizados por sistema, sem apagar referências atuais antes da migração.
- [ ] Preferir PNG transparente **512×512 como arte-mestre**; manter margens ópticas equivalentes, baixo ruído e uma versão já reduzida de qualidade para exibição.
- [ ] Criar opção “Estilo dos consoles” em Aparência. Persistir em configuração distinta de `game_view` e de `icon_manual`.
- [ ] Preservar seleção, foco e navegação ao trocar estilo; fallback legível para console sem imagem.
- [ ] Testar diferentes wallpapers no portátil/dock quando houver hardware disponível; 512px é resolução de origem, não tamanho final de exibição.

**Aceite:** troca de estilo sem recarregar ROM, sem mover o carrossel e sem modificar os três presets da sidebar.

### F2 — Refinamento de cartuchos e cards

- [ ] Refinar proporções, sombras e posicionamento das capas nos cards digitais, reutilizando o mesmo jogo selecionado.
- [ ] Manter o modo cartucho como padrão e a alternância em Aparência.
- [ ] Melhorar carregamento/cache de capas com fallback para arquivos ausentes e imagens inválidas.
- [ ] Preparar animação de encaixe do cartucho sem bloquear o loop SDL nem confundir animação com inicialização real do core.
- [ ] Garantir boa navegação com controle e consistência visual em diferentes resoluções.

**Aceite:** mudar para cards e voltar aos cartuchos não altera qual jogo está em foco nem quebra o lançamento Libretro.

### F3 — Menu rápido de jogo: funcionalidade separada

O menu rápido **não deve ser um painel extra dentro da sidebar principal**. Deve aparecer quando o jogo estiver pausado, chamando um contrato exposto pelo backend.

Esboço da navegação:

```text
Jogo em execução
    └── Menu rápido
        ├── Continuar
        ├── Salvar estado / Carregar estado
        ├── Reiniciar
        ├── Cheats
        ├── Vídeo: frames, overlays, filtros e shaders
        ├── Controles e opções do core
        └── Voltar ao dashboard
```

- [ ] Criar `ui_quick_menu.h/.c` com estados de foco e controles acessíveis por teclado e gamepad.
- [ ] Solicitar capacidades ao backend em vez de assumir que todo core suporta tudo.
- [ ] Mostrar estados indisponíveis com explicação breve, não com botões que falham silenciosamente.
- [ ] Garantir retorno ao jogo sem perder áudio, vídeo e estado; pausar somente quando o backend confirmar que a sessão está pronta.
- [ ] Definir confirmação para ações destrutivas, como sobrescrever um slot ou reiniciar.

**Aceite:** menu pode ser testado com backend simulado; não é necessário ter todos os emuladores implementados para validar layout.

### F4 — Personalização do vídeo do jogo

- [ ] Distinguir **wallpaper do dashboard** de **frame/overlay do jogo** — configurações diferentes.
- [ ] Catálogo de frames PNG, com prévia e opção Sem frame.
- [ ] Overlay acima ou ao lado da imagem sem esconder textos de erro, indicadores ou foco do menu.
- [ ] Menu de filtros e shaders deve mostrar somente opções suportadas pelo backend gráfico; sempre disponibilizar opção Sem filtro.
- [ ] Persistir escolhas por sistema, por jogo ou globais conforme decisão explícita no futuro; não presumir herança que surpreenda a usuária.

**Aceite:** presets visuais e opções avançadas não se confundem e efeitos gráficos podem ser desativados com um único comando.

### F5 — RetroAchievements: conta, conquistas e privacidade

A conexão de conta é uma função de **backend + rede segura**. O frontend cuida somente da experiência e não manipula diretamente senhas ou chamadas HTTP.

- [ ] Criar “Conquistas > Conectar ao RetroAchievements” e uma seção de conta em Configurações.
- [ ] Exibir estados distintos: `Desconectado`, `Conectando`, `Conectado como @usuário`, `Sem internet` e `Sessão expirada`.
- [ ] Oferecer login com campo de senha mascarado; não mostrar credenciais em screenshots, avisos, logs ou arquivos de configuração.
- [ ] Exibir “Lembrar de mim” **somente** se o armazenamento seguro tiver suporte validado na plataforma. Em Switch/homebrew, prever sessão temporária como fallback.
- [ ] Fornecer “Desconectar”, indicação de uso da rede e instrução sobre informações transmitidas ao RetroAchievements, antes do primeiro login.
- [ ] Mostrar lista de conquistas, progresso, status Softcore/Hardcore, notificações de desbloqueio, badges com carregamento assíncrono e placeholder offline.
- [ ] Ao selecionar Hardcore, explicar as restrições de cheats, savestates e outros recursos incompatíveis **antes da confirmação**; o backend efetivamente aplica os bloqueios.
- [ ] Nunca interromper o jogo em andamento só porque a conexão falhou. Mostrar erro recuperável e permitir continuar offline.

**Aceite:** fluxo de UI de login e logout funciona com respostas simuladas (sucesso, erro, timeout, certificado inválido e offline), sem armazenar tokens na UI.

## 3. Contratos entre Frontend e Backend

Para permitir desenvolvimento em paralelo, definir interfaces comuns **antes** de modificar os dois lados:

| Contrato | Backend fornece | Frontend utiliza |
| --- | --- | --- |
| `game_session` | estado e eventos da sessão atual | menu/jogo/pausa/retorno |
| `core_capabilities` | funcionalidades realmente disponíveis | habilitar/desabilitar opções |
| `save_manager` | listar slots, criar/carregar, erros | seleção, avisos, confirmação |
| `cheat_manager` | lista e ativação permitida | controles e bloqueios |
| `game_video` | filtros, frames e opções compatíveis | prévia e seleção |
| `achievements_adapter` | sessão, progresso, conta e eventos | lista, badge, notificações |
| `network_https` e `credential_store` | transporte e segredos | **nunca acessar diretamente** |

**Regras de implementação:** headers estáveis, erros tipados/códigos claros, callbacks assíncronos com entrega segura na thread principal, nenhum campo de senha/token em structs da UI persistidas. Se uma API não estiver pronta, desenvolver um stub/mock para testar a tela sem integrar código incompleto ao menu principal.

## 4. Processo Git: evolução em paralelo sem perder o menu

### 4.1. Referência estável

A base visual aprovada é `666f0aa`. Registrar esse SHA e opcionalmente criar uma tag depois de conferi-lo. Uma branch não é backup imutável; **commit/tag identificam** o ponto de restauração.

**Não fazer alterações de backend diretamente em `main`.** A `main` recebe somente versões integradas e testadas.

### 4.2. Criar branches independentes

Executar uma vez num Codespaces limpo, após verificar que não há trabalho local pendente:

```bash
git status
git fetch origin
git switch feature/ui-views-linux-preview
git pull --ff-only origin feature/ui-views-linux-preview

# Partir ambas da mesma referência aprovada:
git switch -c feature/backend-foundation 666f0aa
git push -u origin feature/backend-foundation

git switch -c feature/ui-next 666f0aa
git push -u origin feature/ui-next

git switch -c integration/next 666f0aa
git push -u origin integration/next
```

> Antes de usar os nomes acima, verificar no GitHub se alguma branch já foi criada. Nesse caso, reutilizar a existente; não sobrescrevê-la. Para scripts/scripts de trabalho futuro, atualizar a base apenas conscientemente.

### 4.3. Divisão de arquivos e redução de conflitos

**Frontend cuida principalmente:** `ui_menu.*`, `ui_carousel.*`, `ui_cards.*`, `ui_game_view.*`, `ui_theme.*`, assets em `ui/` e novo `ui_quick_menu.*`.

**Backend cuida principalmente:** `libretro_core.*`, `console_registry.*`, `input.*` e novos ```text
core_manager.*, game_session.*, save_manager.*, cheat_manager.*,
game_video.*, achievements_adapter.*, network_https.*, credential_store.*
```

**Arquivos compartilhados:** `mini_frontend.c`, `video_audio.c`, headers com eventos e CI. Alterações nesses arquivos exigem contrato acordado e PR pequeno, para não misturar mudanças visuais e de sessão.

### 4.4. Ciclo de desenvolvimento e integração

1. Criar commits pequenos na branch correspondente, com objetivo verificável.
2. Abrir Pull Request para `integration/next`. Verificar diff, testes de compilação e smoke tests.
3. Integrar **uma frente por vez**; revalidar a outra após a junção.
4. Resolver conflitos mantendo o comportamento visual aprovado e os contratos de backend — nunca aceitar todo o arquivo de um lado automaticamente.
5. Testar `integration/next` no Windows; quando Switch estiver habilitado, adicionar build e teste próprios.
6. Somente após um marco de release, criar PR de `integration/next` para `main`, com tag de versão. Evitar force push e histórico reescrito.

Exemplo para atualizar sua branch de trabalho depois de uma integração:

```bash
git fetch origin
git switch feature/ui-next
git merge origin/integration/next
# Resolver apenas os conflitos reais, testar e fazer commit se necessário.
git push origin feature/ui-next
```

**Importante:** `git pull --rebase` em branch com commits locais compartilhados pode exigir cuidado. Para iniciantes e trabalho paralelo com PR, preferir merge explícito quando houver commits publicados; nunca usar `git push --force` como solução automática.

### 4.5. Checklist obrigatório antes de mesclar

- [ ] GitHub Actions Windows verde; checagem de sintaxe Linux verde quando disponível.
- [ ] Abrir o Fofita e comparar sidebar, molduras e três presets com screenshots de referência.
- [ ] Testar cartuchos/cards, preferências persistidas, controles e resolução.
- [ ] Executar ROM de teste, verificar áudio e vídeo, voltar ao menu e repetir.
- [ ] Recursos não suportados pelo core aparecem como indisponíveis, não como botão quebrado.
- [ ] Saves não foram sobrescritos por acidente; manter dados do usuário fora de `git`.
- [ ] Nenhuma credencial/token foi adicionada a código, logs, config, artifact ou screenshot.
- [ ] Offline e falhas de rede não impedem jogar.
- [ ] As alterações do PR são isoladas, revisáveis e possuem caminho simples de reversão.
- [ ] Build Switch só pode ser considerado validado quando existir toolchain, CI e teste real no hardware alvo.

## 5. Organização das entregas conjuntas

| Marco | UI | Backend | Integração |
| --- | --- | --- | --- |
| M0 | Screenshots de referência e contratos | `game_session` e capacidades | O jogo NES segue funcionando |
| M1 | Estilo flat/silhueta | Novos cores e diretórios | Carrossel exibe só sistemas jogáveis |
| M2 | Menu rápido com mocks | SRAM e savestates | UI chama ações reais e trata falhas |
| M3 | Frames e overlays configuráveis | Compositor de vídeo | Sem perda de FPS ou áudio |
| M4 | Configuração de cheats e Hardcore | Cheats + restrições | Bloqueios coerentes e avisos |
| M5 | Login e lista de conquistas | rcheevos + HTTPS + segredo seguro | Testes de segurança, offline e conta |
| M6 | Revisão visual para portátil/dock | Portabilidade e desempenho | Candidato a release |

## 6. Primeira entrega sugerida

**Sprint inicial em paralelo:**
- **Frontend:** preservar o build aprovado, preparar ícones flat/silhueta e um protótipo independente do menu rápido.
- **Backend:** estabelecer `game_session`, detectar capacidades do core e preparar arquitetura de diretórios e carregamento correto.
- **Ponto de encontro:** um contrato de eventos e capacidades testável sem implementar RetroAchievements ou shaders agora.

O planejamento de segurança não deve esperar a etapa do login: proibir segredos no Git, padronizar erros/logs e reservar `credential_store`/`network_https` desde já. A implementação real da rede vem depois, com testes dedicados.

## 7. Referências cruzadas

- [Plano do Backend, Emulação e Segurança](PLANO_BACKEND_E_SEGURANCA.md).
- [Guia oficial de integração RetroAchievements/rcheevos](https://github.com/RetroAchievements/rcheevos/wiki/rc_client-integration).
- [Libretro API](https://github.com/libretro/libretro-common/blob/master/include/libretro.h).
- [Build Windows Fofita no GitHub Actions](../.github/workflows/build-windows.yml).

> **Nota:** este documento é um plano de trabalho e integração. Ele não cria as três branches, não modifica recursos gráficos, não adiciona contas nem modifica o executável.

# Fofita — Planejamento atualizado do Visual Engine

Atualizado em 27/11/2026. Documento de planejamento, não declaração de implementação concluída.

## Princípios

- Simplicidade: poucas opções, bons padrões automáticos, sem reproduzir toda a complexidade do RetroArch.
- Preservar a interface aprovada, a nitidez e a proporção original dos consoles.
- Configurações > Vídeo do jogo concentra as opções de emulação; Customização permanece dedicada ao dashboard.
- Modos de proporção fiel: Original do console (padrão; NES 4:3, GBA 3:2, GB/GBC 10:9) e Pixels quadrados (resolução nativa). Não oferecer 4:3 forçado como terceira opção.
- Efeitos opcionais, desligáveis, com fallback simples e sem interferir em áudio, controles ou saves.
- Não expor recursos incompletos nem prometer suporte ao Switch antes de testes reais.

## Roadmap

| Etapa | Entrega | Critério de aceite |
| --- | --- | --- |
| V0 | Correção inicial | Nitidez, menu, persistência e restauração validados pela usuária no build 429f13d. Balanceamento ainda não demonstrou diferença perceptível; proporções precisam ser simplificadas. |
| V1 | Integração experimental | Planejar branch integration/next, incorporar incrementalmente frontend, backend e vídeo estáveis, sem exigir conclusão de todas as funcionalidades e sem alterar main. |
| V2 | Motor de vídeo independente | Separar game_video.c/h, manter duas proporções, avaliar Sharp Bilinear ou alternativa simples para escala fracionária; comparar visualmente com nearest. Evitar opções redundantes. |
| V3 | Frames e overlays | Composição PNG simples, posicionamento correto da tela, opção sem frame e fallback para assets inválidos. |
| V4 | Pipeline de shaders | Avaliar librashader e APIs gráficas, licenças, desempenho e portabilidade. Priorizar poucos efeitos úteis (LCD/CRT); não adicionar dependências sem benefício demonstrado. |
| V5 | Combos visuais | Presets JSON simples agrupando efeitos, paletas, frames e escala, sem duplicar configurações. |
| V6 | DMG Original | Primeiro combo completo de Game Boy clássico, com efeitos opcionais e comparação visual. |
| V7 | Editor visual | Prévia e edição simples de combos; preferências globais, por console e por jogo somente conforme necessidade, evitando complexidade prematura. |

## Integração e dependências

Referências verificadas na revisão anterior: frontend 3f99dc6, backend 60ccdda e visual 429f13d. São pontos históricos, não necessariamente os HEADs atuais.

- Manter feature/ui-next, feature/backend-foundation e feature/visual-engine independentes.
- A integração experimental pode começar antes da conclusão das três frentes, mas não é requisito para continuar o desenvolvimento isolado de vídeo. Verificar HEADs e diferenças antes de criar ou atualizar integration/next.
- Integrar uma frente por vez, resolver conflitos em ui_menu.c, mini_frontend.c, video_audio.c e CI preservando contratos e funcionalidades aprovadas. Não substituir arquivos compartilhados integralmente sem revisão.
- Recursos incompletos ficam indisponíveis ou desativados. Não mesclar na main antes de testes de regressão.
- Validar compilação Windows, navegação, wallpapers, controles, SRAM, savestates, retorno ao menu, proporções e desempenho. Compilação bem-sucedida não equivale a teste visual.
- O menu rápido e outros recursos de backend continuam com seus próprios planos; não são pré-requisitos para experimentar o motor visual isoladamente.

## Próxima entrega mínima

Simplificar proporções para as duas opções aprovadas, migrando a antiga preferência 4:3 forçado para Original do console; verificar o balanceamento em diferentes tamanhos de janela e manter apenas opções com diferença útil. Preservar preferências e restauração. Avaliar a integração experimental separadamente, sem bloquear a evolução do Visual Engine.

## Backlog futuro — preenchimento automático de artes

Implementar após estabilizar a interface e o motor visual, em módulo independente (sugestão: artwork_manager.c/h), com integração ao frontend.

- Fonte: [Libretro Thumbnails](https://github.com/libretro-thumbnails) e https://thumbnails.libretro.com/; mapear IDs dos consoles para os nomes das coleções.
- Priorizar Named_Boxarts; Named_Snaps e Named_Titles são alternativas opcionais (telas de título, não necessariamente logos isolados).
- Preservar artes locais/personalizadas; consultar apenas jogos sem arte, normalizar nomes e comparar região/versão sem substituir por correspondência ambígua.
- Baixar sob demanda, em segundo plano, via HTTPS com validação TLS, timeout, limite de tamanho e validação de PNG; cache local e gravação temporária antes de renomear.
- Atualizar o card sem bloquear o menu; manter placeholder se offline ou sem correspondência. Evitar tentativas repetidas e permitir desativar a busca automática.
- Verificar termos de uso, direitos das imagens e limites do servidor antes de distribuir ou automatizar downloads. Não enviar ROMs.
- Aceite: testar NES, GB e GBC, arte personalizada preservada, cache offline, falhas de rede e nomes ambíguos. Não é pré-requisito dos shaders.

## Checklist de implementação dos shaders (V2–V6)

1. Corrigir serrilhamento da UI separadamente; manter filtragem do jogo independente.
2. Isolar game_video.c/h: entrada do frame libretro, proporção, escala, viewport e composição.
3. Validar nearest e escala inteira/fracionária; avaliar Sharp Bilinear sem multiplicar opções.
4. Avaliar backend GPU compatível com SDL2 (OpenGL no Windows como candidato) e librashader; verificar licenças, integração e caminho futuro para Switch. SDL_Renderer sozinho não oferece pipeline de shaders arbitrários.
5. Criar pipeline mínimo: upload da textura, compilação de shader, uniforms de resolução de entrada/saída e tempo quando necessário, renderização e fallback sem efeito. Gerenciar recursos ao redimensionar/trocar jogo.
6. Implementar LCD leve (grade de pixels e contraste), depois CRT simples (scanlines e máscara opcional); paleta DMG e ghosting leve como efeitos opcionais, este último exige histórico de frames. Evitar curvatura/bloom e multipass complexo inicialmente.
7. Compor frame PNG/overlay após efeito do jogo, sem aplicar shader à interface; manter proporção e área útil.
8. Expor poucos presets: Desligado (padrão), LCD, CRT; persistir preferências e restaurar padrões. Combo DMG Original após validação, com JSON simples.
9. Testar NES/GB/GBC, escala 720p/1080p e fracionária, troca de jogo, retorno ao menu, FPS/latência, falha de shader e fallback. Switch exige validação em hardware real.

## Direção visual aprovada — poucos shaders, bons shaders (27/11/2026)

Referências visuais fornecidas: fotos e capturas de LCD DMG com grade perceptível e paleta amarelo-esverdeada, GBC com pixels translúcidos e CRT com máscara visível, scanlines e vidro curvado. São **referências de resultado**, não prova de que os shaders já funcionem no Fofita.

### Presets selecionados por família de consoles

| Família | Preset recomendado | Referência técnica | Comportamento pretendido |
| --- | --- | --- | --- |
| GB / Game Boy DMG | **DMG Clássico** | [kathoc/brickboy-dmg-shader](https://github.com/kathoc/brickboy-dmg-shader) | LCD passivo, matriz de pixels, persistência discreta e paleta amarelo-esverdeada/verde-oliva inspirada nas imagens enviadas |
| GBC / Game Boy Color | **GBC Pixel Transparency** | [mattakins/Pixel_Transparency](https://github.com/mattakins/Pixel_Transparency/releases) | Grade LCD e pixel transparency; não somar automaticamente outro shader LCD, pois a versão 3.0 já integra a grade |
| GBA e outros portáteis de LCD | **LCD Limpo** | Efeito leve a definir/testar | Grade discreta e cores preservadas; não aplicar paleta monocromática DMG nem transparência GBC indistintamente |
| NES, SNES, Mega Drive, Master System e sistemas de TV equivalentes | **CRT Clássico** | Shader CRT a avaliar e validar | Scanlines e máscara/subpixels equilibradas, com **curvatura suave nos cantos opcional**; evitar deformação excessiva |
| Sistemas sem perfil validado | **Desligado / imagem nítida** | Renderização base | Não aplicar efeito inadequado por suposição |

A indicação automática depende do console efetivamente identificado. A preferência manual do usuário prevalece. Não pressupor CRT para consoles portáteis ou DMG para todos os LCDs no modo automático. A família do console é apenas recomendação: permitir seleção manual de qualquer preset em qualquer console/jogo, desde que tecnicamente compatível.

### Paleta DMG de referência

- Meta visual: amarelo-esverdeado quente no fundo, verde-oliva profundo nos pixels escuros, quatro níveis distinguíveis; **não** adotar automaticamente o verde-menta claro da segunda referência.
- Extrair uma primeira paleta aproximada das referências fornecidas apenas para prototipagem; a captura/fotografia pode ter iluminação e correção de cor, portanto não tratá-la como medição de hardware.
- Usar o sistema de correção/paleta do shader DMG onde possível; evitar duas colorizações conflitantes (core + shader). Comparar em jogos claros, escuros e com movimentação.
- Manter grade e ghosting moderados para preservar legibilidade; intensidade fina somente se testes mostrarem necessidade.

### Três paletas DMG selecionáveis

Propostas iniciais, sujeitas a validação visual, com quatro tonalidades cada:
- DMG Original: #D5CD49, #A2AB38, #586B25, #263C16.
- Verde clássico: #C4E58B, #8CB85D, #487844, #183C25.
- Pocket: #DCE5E3, #A3B2B4, #637378, #20292F (monocromático frio inspirado na referência fotográfica enviada; valores aproximados).

Permitir paleta DMG em outros consoles por escolha explícita, com conversão de luminância quando necessária. Não limitar efeitos por ID de console. Manter recomendação automática e escolha manual independente; oferecer paleta original/sem conversão quando aplicável.

### CRT com vidro curvado

- Preset base: CRT nítido, scanlines moderadas, máscara/subpixels finos, brilho e contraste preservados.
- Controle simples específico do preset CRT: **Tela curva: desligada (padrão) / ligada**. Quando ligada, curvatura geométrica sutil com cantos arredondados, vinheta leve e margens seguras; não cortar HUD, placares ou legendas.
- Evitar incluir uma moldura PNG obrigatória; permitir combinação futura com frames/overlays sem duplicar bordas.
- Testar especialmente a legibilidade de textos e a escala em 720p/1080p, além do custo de GPU.

### Menu enxuto

Manter em **Configurações > Vídeo do jogo** (não em Customização do dashboard):

- **Visual:** Automático por console / Desligado / DMG Clássico / GBC Pixel Transparency / LCD Limpo / CRT Clássico (expor apenas presets implementados e validados, independentemente do console).
- A escolha automática oferece DMG Clássico para GB, Pixel Transparency para GBC, LCD Limpo para GBA e CRT Clássico para consoles de TV **apenas depois de cada preset ser validado**; enquanto isso, fallback sem shader.
- **Tela curva:** opção contextual dentro do CRT, desligada inicialmente.
- Conservar Original do console e Pixels quadrados; investigar widescreen adaptativo e aprimorado, sem esticamento convencional. Não multiplicar filtros, paletas e efeitos independentes no menu principal.
- Persistir preferência global e, quando necessário, exceção por console; preservar preferência manual, restaurar padrões e impedir apenas efeitos tecnicamente incompatíveis, nunca restringir por família de console.

### Dependências, licença e limites

- Os dois projetos de referência fornecem shaders para RetroArch; **não são plugins instaláveis diretamente no SDL_Renderer atual**. Avaliar suporte a presets Slang multipass (librashader ou integração gráfica equivalente), texturas auxiliares, histórico de frames, framebuffer float, desempenho e restauração de contexto.
- O repositório brickboy-dmg-shader declara licença **Apache-2.0**; permite uso, modificação e redistribuição inclusive comercial; incluir licença, preservar avisos aplicáveis, indicar arquivos modificados e reproduzir avisos NOTICE quando existentes. Não exige relicenciar todo o Fofita.
- O repositório Pixel_Transparency declara autoria/copyright e publica versões Slang e GLSL. **Não foi identificada uma licença de redistribuição explícita na consulta**: confirmar autorização/licença antes de copiar, modificar ou embutir arquivos no Fofita. Até lá, manter apenas link/referência e protótipo próprio ou integração autorizada.
- A versão 3.0 de Pixel Transparency integra a grade LCD em seu próprio pipeline; a implementação GLSL antiga não acompanha integralmente a versão 3.0. Movimentação por giroscópio é opcional e não presumida em Windows/Switch.
- Testar no Windows primeiro, sem prometer compatibilidade no Switch antes de medições e validação em hardware.
- Ordem de trabalho: corrigir serrilhamento da UI; consolidar game_video.c/h; validar pipeline GPU/Slang; **DMG Clássico**; **GBC Pixel Transparency** (condicionado a licença/integração); **CRT Clássico + tela curva**; LCD Limpo; presets automáticos; combos JSON e overlays.

### Critérios de aceite

1. GB mostra cores DMG quentes/oliva e grade legível sem borrar os sprites; comparação lado a lado com imagem sem filtro.
2. GBC exibe efeito distinto no modo automático e não recebe paleta DMG salvo escolha manual; nenhum shader LCD duplicado. Testar seleção cruzada de presets entre consoles.
3. CRT oferece tela normal ou curva sem recortar elementos da interface do jogo.
4. O mesmo jogo funciona com filtros desligados; troca de console/restauração de configuração não deixa efeitos residuais.
5. Desempenho, shaders quebrados, falta de GPU compatível e falha de carregamento resultam em fallback seguro.
6. A UI do dashboard não recebe os shaders de jogo; telas, ícones e frames são tratados separadamente.

## V7 — Editor de combos visuais (conceito aprovado)

Seguir o protótipo apresentado anteriormente e confirmado pela usuária em 27/11/2026: prévia no topo, nome do combo, console de referência, paleta, efeito de tela, frame, overlay e controles de intensidade.

- Modo simples: selecionar preset pronto, automático ou desligado; aplicar e restaurar padrões.
- Modo avançado: criar/editar combo, escolher nome, console de referência (opcional, não restringe aplicação), paleta, shader, frame e overlay; mostrar apenas parâmetros relevantes ao efeito selecionado (grade LCD, persistência, scanlines, máscara, curvatura etc.).
- Prévia em tempo real usando o pipeline real quando disponível; antes disso, identificar qualquer prévia ilustrativa como tal.
- Ambos os modos usam a mesma estrutura de configuração JSON versionada; presets originais preservados, combos personalizados editáveis, duplicáveis e removíveis.
- Permitir aplicar qualquer combo a qualquer console/jogo tecnicamente compatível; recomendação automática não é restrição.
- Oferecer salvar, aplicar, cancelar e restaurar; não alterar preferências definitivas ao experimentar a prévia.
- Implementar após estabilizar pipeline de shaders, frames e primeiros presets; não expor controles sem efeito funcional.
- Validar desempenho, fallback, persistência, troca de console e navegação por teclado, controle e mouse.

### Filtro de recursos no editor de combos

O seletor de console de referência funciona como filtro de catálogo, não como restrição de aplicação. Ao escolher GB, GBC, GBA, NES ou outro console, apresentar paletas, shaders, frames e overlays coerentes com a família selecionada. A opção Todos exibe o catálogo completo. Preservar escolhas existentes ao trocar o filtro; não substituir silenciosamente componentes do combo. Se uma escolha estiver fora do filtro, mantê-la identificada como selecionada. Frames e overlays são recursos independentes, com opção Nenhum. Paleta Pocket revisada para monocromático frio, inspirada na foto enviada, mantendo quatro tonalidades e três paletas especiais no total.

## Complemento — tela cheia e frames minimalistas (27/11/2026)

- Consenso: frames inspirados apenas na moldura física da tela, não na carcaça com controles. Priorizar GB, GBC e GBA; posteriormente molduras inspiradas em televisões CRT para consoles de mesa. Aproveitar margens da tela 16:9 do Switch, preservar área útil e proporção, permitir desativar frame e overlay separadamente. GBA pode usar logotipo inferior; GB/GBC podem usar logotipo lateral e indicador de energia, conforme referências fornecidas. Testar 720p/1080p.
- Decisão revisada: Original do console (padrão), Pixels quadrados, Widescreen adaptativo (16:9 não linear) e Widescreen aprimorado/nativo quando core e jogo suportarem. Excluir esticamento convencional e 4:3 forçado. Frames devem ser desativados ou adaptados quando não houver margem livre, sem cobrir conteúdo.
- Investigar widescreen aprimorado por core/jogo, distinto do esticamento do frontend. Priorizar suporte nativo quando disponível; para SNES avaliar bsnes-hd beta, Mode 7 e configurações/patches específicos por jogo. Não alterar ROM original automaticamente. Não prometer compatibilidade universal ou desempenho no Switch.
- Avaliar escala não linear como alternativa experimental, mantendo centro menos distorcido à custa das laterais; não confundir com expansão real do campo de visão. Sharp Bilinear melhora amostragem, não corrige geometria.
- Se widescreen aprimorado não estiver disponível, manter proporção original ou oferecer widescreen adaptativo por escolha explícita; nunca esticamento convencional. Preservar preferências por jogo/console quando implementadas.
- Referências consultadas: [Retro Game Corps — Widescreen SNES Guide](https://retrogamecorps.com/2023/02/22/widescreen-snes-guide/) e [RetroArch Emu — Aspect Ratio and Screen Fill](https://retroarchemu.gitlab.io/how-can-i-get-the-game-to-fill-the-screen-and-play-in-169-in-retroarch/). O primeiro descreve suporte específico de bsnes-hd e ressalvas de desempenho/compatibilidade; o segundo é referência geral, não documentação oficial de capacidades de cores.
- Aceite: comparar geometria original e 16:9, legibilidade, ausência de recorte, interação com shaders/frames, fallback e desempenho. Recursos ainda não implementados.

## Decisão sobre widescreen

Aprovadas todas as alternativas exceto esticamento convencional. Widescreen adaptativo é responsabilidade do motor visual; widescreen aprimorado depende das capacidades do core e, quando necessário, da ROM/patch específico. Não anunciar suporte nativo sem validação. Desabilitar opção aprimorada quando indisponível, com fallback para proporção original. Não modificar ROMs automaticamente.

## Escopo final inicial e sequência de implementação

Decisão mais recente: somente um overlay especial, Sombra LCD, além de Nenhum. Reproduzir sombra interna nas bordas do painel DMG conforme referência enviada em 27/11/2026. Avaliar primeiro o air-gap drop shadow do Brickboy; se suficiente, expor seu parâmetro sem duplicar a sombra com PNG. Caso necessário, criar camada independente ajustada à abertura da tela. Não adicionar reflexos, sujeira ou outros overlays até novas referências aprovadas. Catálogo dos demais consoles inicialmente oferece Nenhum; filtro Todos permite escolha manual quando tecnicamente compatível. Esta decisão substitui sugestões anteriores de overlays adicionais.

Sequência de builds pequenas:
1. V2.0: verificar HEAD e integração atual; reproduzir serrilhamento da sidebar e frames, corrigir isoladamente, validar regressões em 720p/1080p e escala fracionária.
2. V2.1: separar game_video.c/h, manter comportamento atual, testar NES/GB/GBC, áudio, controles, retorno ao menu e preferências.
3. V2.2: protótipo GPU/Slang, avaliar librashader e alternativas, licenças, dependências Windows, caminho Switch; primeiro shader passthrough e fallback.
4. V2.3: Sharp Bilinear e widescreen adaptativo não linear; preservar original e pixels quadrados, sem esticamento convencional. Widescreen aprimorado somente com suporte comprovado do core/jogo.
5. V4.1: DMG Brickboy, três paletas especiais (DMG Original, Verde clássico, Pocket frio), cores originais, grade, persistência e sombra LCD opcional.
6. V4.2: Pixel Transparency condicionado à licença; LCD Limpo; CRT Clássico com curvatura opcional. Validar cada preset separadamente.
7. V3/V5: frames minimalistas GB/GBC/GBA e depois CRT, composição e presets JSON; sem reduzir área útil desnecessariamente.
8. V7: editor simples/avançado com prévia real, filtro de catálogo por console e Todos, escolhas cruzadas, salvar/aplicar/cancelar e persistência.
9. Validação integrada Windows e posteriormente Switch real. Busca automática de artes permanece no backlog independente.

Não avançar uma etapa que introduza regressão sem correção ou fallback. Cada entrega deve indicar commit, build, testes automatizados e checklist manual. A ordem acima prevalece sobre a numeração histórica do roadmap quando houver divergência.

## Documentos relacionados

- [Plano do frontend e integração Git](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_FRONTEND_E_INTEGRACAO_GIT.md)
- [Plano do backend e segurança](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_BACKEND_E_SEGURANCA.md)
- [Savestates B2.2](https://github.com/tupinikitten/Fofita/blob/feature/backend-foundation/docs/B2_2_SAVESTATES.md)
- [Wallpapers](https://github.com/tupinikitten/Fofita/blob/feature/ui-next/ui/wallpapers/README.md)

## V2.1 — início da fundação

Serrilhamento corrigido conforme retorno da usuária. Adiar normalização do tamanho visual dos ícones de fontes diferentes de 512x512 para o frontend. Extração do processamento de frames, textura, escala e proporção para game_video.c/h; manter SDL_Renderer e comportamento atual. Compilação e testes manuais pendentes.

### Validação V2.1

Build cdafdf7: testes manuais aprovados pela usuária em 27/11/2026, incluindo redimensionamento. CI: success. Próxima etapa: V2.2, avaliação e protótipo do pipeline de shaders, preservando fallback SDL.

### V2.2a — protótipo GPU

Passagem GLSL 1.20 passthrough em contexto OpenGL separado, opt-in FOFITA_SHADER_TEST=1, retorno ao SDL em falha. Readback síncrono apenas para prova de conceito, não arquitetura final. Librashader/Slang e Switch ainda não integrados. Validar compilação e execução antes de avançar.

### Validação V2.2a

Testes manuais aprovados em 27/11/2026 com FOFITA_SHADER_TEST=1 e quatro mensagens de inicialização GPU passthrough. CI: success. Próximo: controle de shader no menu e avaliação librashader/Slang. Fallback em falha ainda não testado explicitamente; Switch e desempenho não validados.

### V2.2b — seleção pelo menu

Configurações > Video: Shader Nenhum/Passthrough (teste), persistência video_shader em fofita_menu.cfg, restaurar padrões desativa. Menu substitui variável de ambiente. Fallback mantém log existente; aviso visual de falha e teste forçado pendentes. Sem alteração de efeitos ou integração Slang nesta build.

### Validação V2.2b

Testes manuais aprovados pela usuária em 27/11/2026. Renomear entrada Video para Configurações de vídeo conforme solicitado.

### V2.2c.1 — compatibilidade OpenGL moderna

V2.2b e novo nome aprovados. Avaliada documentação oficial librashader v0.12.1: OpenGL 3.3+ e API C com loader dinâmico; release consultada sem assets. Primeiro testar contexto core 3.3 independente, registrar versão real e manter passthrough 2.1. Próximos: verificar licença e distribuição/build fixado, integrar API C e preset de teste; renderizar na resolução de saída, medir readback e avaliar Switch separadamente. Nenhuma DLL externa incluída nesta etapa.

### Validação V2.2c.1 e automação de testes

27/11/2026: usuária confirmou três inicializações com OpenGL 3.3 compatível e passthrough ativo. CI b443920 concluída com sucesso. Exibição do nome Configurações de vídeo não reconfirmada explicitamente nesta mensagem.

Diretriz aprovada: automatizar regressões progressivamente no GitHub Actions. Priorizar testes unitários de escala/proporção, conversão de pixels, configuração e fallback injetado; depois smoke tests de cores com ROMs livres/licenciadas e comparação de frames com tolerância. Separar testes sem GPU dos que exigem driver/contexto real; não presumir GPU física no runner hospedado. Manter avaliação estética, controles e desempenho em hardware real como testes manuais. Iniciar infraestrutura de testes junto às próximas entregas, sem bloquear integração Slang por uma suíte completa.

### V2.2c.2a — loader opcional

Nome do menu reconfirmado. Loader SDL verifica DLL ao lado do executável, ABI 2/API 5 conforme header oficial v0.12.0. Release x64 disponível em https://github.com/SnowflakePowered/librashader/releases/tag/librashader-v0.12.0 ; SHA256 521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183. Empacotamento adiado até verificar licenças MPL/GPL e componentes do binário. Sem execução Slang ainda. Ausência da DLL mantém passthrough.

## V2.2c.2 e V2.2c.3 — execução Slang pelo librashader (28/11/2026)

**Estado: implementado e validado por testes automatizados no CI. Pendente: validação manual em GPU real.**

### Implementado

- `slang_backend.c/h`: API C do librashader 0.12.0 (ABI 2, API 5), carregada em tempo de execução pela pasta do executável. ABI/API são conferidas antes de resolver o restante.
- `shader_manager.c/h`: contexto OpenGL 3.3 core em janela oculta, separado do SDL_Renderer; o contexto anterior é restaurado. O preset é renderizado na resolução do retângulo do jogo na tela e volta por `glReadPixels` (síncrono) para uma textura SDL. Não se aplica à interface.
- `game_video.c`: guarda uma cópia do último frame; refaz a saída em frame novo ou mudança de tamanho; reutiliza em frames repetidos. A imagem sem efeito permanece atualizada como fallback.
- Preset próprio de duas passagens em `shaders/fofita_teste/` (tom quente + grade por pixel), sem código de terceiros. É um preset de **teste**, não um dos visuais do roadmap.
- Menu: **Configurações > Configurações de vídeo > Shader**: Nenhum / Passthrough (teste) / Slang (teste), persistido em `video_shader`; mostra "falhou" e um aviso ao voltar ao menu. Diagnóstico em `fofita_shader.log`.
- `video_math.c` e `video_config.c`: proporção, conversão de pixels e configuração isoladas para testes.
- Correção: o botão fechar da janela principal encerra o app mesmo com a janela oculta do shader (no SDL2 clássico, `SDL_QUIT` só é enviado quando a última janela fecha; o sdl2-compat atual não conta janelas ocultas).

### Dependência e licença

- O binário oficial 0.12.0 para Windows x64 (zip SHA-256 `521fe0f364bfa705883f9e99fb1733a3a24f0f403bcc5591b6b78f6cff289183`) **não é usado**: importa `D3DX9_43.dll`, `MSVCP140.dll` e `VCRUNTIME140*.dll` e não carregou no runner Windows.
- O CI compila a biblioteca do commit `87e8a97b50516d997defeaa168173dcd185d4022`, com `Cargo.lock` travado, Rust 1.93.0, somente runtime OpenGL e CRT estático (`tools/compilar_librashader.sh`). A integridade é garantida pelo commit e pelo lockfile; a DLL compilada não tem hash fixado. A DLL resultante importa apenas bibliotecas do Windows.
- Redistribuição sob MPL-2.0; header sob MIT (idêntico ao da tag). O pacote leva `licenses/librashader/` (licença, origem do código e avisos de terceiros gerados do `Cargo.lock`: 132 pacotes; a geração falha se algum só permitir GPL/LGPL/AGPL pela expressão declarada ou não declarar licença). Isso é uma verificação automatizada de metadados, não uma revisão jurídica.

### Resultados automatizados (run 46146800926, commit 6a241a9)

| Teste | Ambiente | Resultado |
| --- | --- | --- |
| `test_unit`: proporção, configuração, conversão de pixels, falhas do carregador (ausente, corrompida, ABI errada, símbolo ausente) | Linux (ASan/UBSan) e Windows | 56/56 em cada |
| `test_gpu`: Slang pixel a pixel, formatos RGB555/RGB565/XRGB8888, redimensionamento, mudança de resolução, preset ausente/inválido/com erro de compilação, recuperação, passthrough | Windows; SDL_Renderer software, OpenGL, Direct3D 11 e Direct3D 9 | 85/85 em cada |
| `test_core`: núcleo real, sem shader, Slang em duas sessões e passthrough | NES (fceumm do repositório), GB e GBC (gambatte `b14881d`, compilado só para teste) | 17/17 em cada |
| `test_core` NES com o app em pasta com espaços e acentos | Windows | 17/17 |
| `test_menu`: navegação por teclado até o Shader, persistência, reabertura e restaurar padrões | Windows | 32/32 |
| Abertura do `Fofita.exe` do pacote com PATH sem MSYS2 | Windows | permaneceu aberto 8 s |

### Limites desta validação

- O OpenGL do CI é o Mesa llvmpipe (CPU). Não houve teste em GPU física nem medida de desempenho representativa; os tempos do log do CI não servem como referência.
- Não automatizados: mouse e controle no menu, áudio, entrada durante o jogo, SRAM/savestates, jogos comerciais, estética e 720p/1080p em monitor real. O teste de abertura não navega pelo app.
- O readback síncrono na resolução de saída pode custar desempenho, sobretudo em telas grandes. Após 600 quadros o `fofita_shader.log` registra a média por quadro; medir em hardware real antes de adicionar efeitos mais pesados.
- Falha de carregamento tem fallback testado. Travamento ou perda de contexto do driver durante o jogo não foi testado.
- Apenas Windows x64. **Switch não validado**: a abordagem (DLL Rust, OpenGL de desktop 3.3, janela oculta e readback) precisa ser reavaliada antes de qualquer promessa.
- O pacote não distribui o núcleo de GB/GBC; é preciso `cores/gambatte_libretro.dll` local.

### Pendências fora do escopo desta etapa

- O pacote já distribuía `cores/fceumm_libretro.dll` e DLLs do MSYS2 sem avisos de licença correspondentes; há também DLLs versionadas na raiz do repositório. Revisar antes de qualquer distribuição pública.

### Próximo passo

Após o checklist manual em GPU real (incluindo a média por quadro do log): V2.3, Sharp Bilinear e widescreen adaptativo. Sharp Bilinear pode ser um preset próprio no pipeline já validado, com alternativa sem GPU a avaliar.

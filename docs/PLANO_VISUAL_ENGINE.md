# Fofita — Planejamento atualizado do Visual Engine

Atualizado em 27/11/2026. Documento de planejamento, não declaração de implementação concluída.

## Princípios

- Simplicidade: poucas opções, bons padrões automáticos, sem reproduzir toda a complexidade do RetroArch.
- Preservar a interface aprovada, a nitidez e a proporção original dos consoles.
- Configurações > Vídeo do jogo concentra as opções de emulação; Customização permanece dedicada ao dashboard.
- Apenas duas opções de proporção: Original do console (padrão; NES 4:3, GBA 3:2, GB/GBC 10:9) e Pixels quadrados (resolução nativa). Não oferecer 4:3 forçado como terceira opção.
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
- Pocket: #E0E4D6, #A5AE9D, #626C60, #252D27.

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
- Conservar somente as duas proporções já aprovadas: Original do console e Pixels quadrados. Não multiplicar filtros, paletas e efeitos independentes no menu principal.
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

## Documentos relacionados

- [Plano do frontend e integração Git](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_FRONTEND_E_INTEGRACAO_GIT.md)
- [Plano do backend e segurança](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_BACKEND_E_SEGURANCA.md)
- [Savestates B2.2](https://github.com/tupinikitten/Fofita/blob/feature/backend-foundation/docs/B2_2_SAVESTATES.md)
- [Wallpapers](https://github.com/tupinikitten/Fofita/blob/feature/ui-next/ui/wallpapers/README.md)

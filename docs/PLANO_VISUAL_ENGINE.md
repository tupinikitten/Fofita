# Fofita — Planejamento atualizado do Visual Engine

Atualizado em 25/11/2026. Documento de planejamento, não declaração de implementação concluída.

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

## Documentos relacionados

- [Plano do frontend e integração Git](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_FRONTEND_E_INTEGRACAO_GIT.md)
- [Plano do backend e segurança](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_BACKEND_E_SEGURANCA.md)
- [Savestates B2.2](https://github.com/tupinikitten/Fofita/blob/feature/backend-foundation/docs/B2_2_SAVESTATES.md)
- [Wallpapers](https://github.com/tupinikitten/Fofita/blob/feature/ui-next/ui/wallpapers/README.md)

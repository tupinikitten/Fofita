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

## Documentos relacionados

- [Plano do frontend e integração Git](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_FRONTEND_E_INTEGRACAO_GIT.md)
- [Plano do backend e segurança](https://github.com/tupinikitten/Fofita/blob/feature/ui-views-linux-preview/docs/PLANO_BACKEND_E_SEGURANCA.md)
- [Savestates B2.2](https://github.com/tupinikitten/Fofita/blob/feature/backend-foundation/docs/B2_2_SAVESTATES.md)
- [Wallpapers](https://github.com/tupinikitten/Fofita/blob/feature/ui-next/ui/wallpapers/README.md)

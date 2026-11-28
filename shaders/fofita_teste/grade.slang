#version 450
// Shader de teste escrito para o Fofita. Escurece a borda inferior e direita
// de cada pixel do jogo; usa SourceSize e a resolucao final da tela.

layout(push_constant) uniform Push
{
    vec4 SourceSize;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec3 cor = texture(Source, vTexCoord).rgb;
    vec2 dentro = fract(vTexCoord * params.SourceSize.xy);
    float linha = dentro.y > 0.7 ? 0.5 : 1.0;
    float coluna = dentro.x > 0.7 ? 0.75 : 1.0;
    FragColor = vec4(cor * linha * coluna, 1.0);
}

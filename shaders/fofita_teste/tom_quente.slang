#version 450
// Shader de teste escrito para o Fofita. Aplica um tom quente evidente para
// confirmar visualmente que o pipeline Slang esta executando.

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
    FragColor = vec4(cor * vec3(1.0, 0.9, 0.72), 1.0);
}

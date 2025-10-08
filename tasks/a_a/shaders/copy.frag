#version 450

layout(push_constant) uniform params
{
  uvec2 iResolution;
  uvec2 iMouse;
  float iTime;
  int objectsAmount;
  int mouseControlType;
};

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 out_fragColor;

void main()
{
  out_fragColor = texture(tex, gl_FragCoord.xy / vec2(iResolution));
}
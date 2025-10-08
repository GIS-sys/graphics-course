#version 450

layout(push_constant) uniform params
{
  uvec2 iResolution;
  uvec2 iMouse;
  float iTime;
  int objectsAmount;
  int mouseControlType;
};

layout(set = 0, binding = 1) uniform sampler2D fileTex;

layout(location = 0) out vec4 out_fragColor;

float PI = 3.14159265358979323846;

void main()
{
  out_fragColor = texture(fileTex, gl_FragCoord.xy / vec2(iResolution));
}

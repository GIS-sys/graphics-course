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

// FXAA implementation
#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL (1.0 / FXAA_SPAN_MAX)
#define FXAA_REDUCE_MIN (1.0 / 128.0)

vec4 fxaa(vec2 uv, sampler2D tex, vec2 resolution)
{
  vec2 rcpFrame = 1.0 / resolution;
  vec2 uvN = uv - rcpFrame * 0.5;
  vec2 uvP = uv + rcpFrame * 0.5;
  vec3 rgbNW = texture(tex, uvN).xyz;
  vec3 rgbNE = texture(tex, vec2(uvP.x, uvN.y)).xyz;
  vec3 rgbSW = texture(tex, vec2(uvN.x, uvP.y)).xyz;
  vec3 rgbSE = texture(tex, uvP).xyz;
  vec3 rgbM = texture(tex, uv).xyz;
  vec3 luma = vec3(0.299, 0.587, 0.114);
  float lumaNW = dot(rgbNW, luma);
  float lumaNE = dot(rgbNE, luma);
  float lumaSW = dot(rgbSW, luma);
  float lumaSE = dot(rgbSE, luma);
  float lumaM = dot(rgbM, luma);
  float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
  float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
  vec2 dir;
  dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
  dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
  float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
  float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
  dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * rcpFrame;
  vec3 rgbA = (1.0 / 2.0) * (texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).xyz + texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).xyz);
  vec3 rgbB = rgbA * (1.0 / 2.0) + (1.0 / 4.0) * (texture(tex, uv + dir * (0.0 / 3.0 - 0.5)).xyz + texture(tex, uv + dir * (3.0 / 3.0 - 0.5)).xyz);
  float lumaB = dot(rgbB, luma);
  if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
    return vec4(rgbA, 1.0);
  } else {
    return vec4(rgbB, 1.0);
  }
}

void main()
{
  vec2 uv = gl_FragCoord.xy / vec2(iResolution);
  out_fragColor = fxaa(uv, tex, vec2(iResolution));
}
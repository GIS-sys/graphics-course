#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 0, rgba8) uniform writeonly image2D fogTexture;

layout(push_constant) uniform FogParams {
    float time;
    vec2 resolution;
} params;

void main() {}
//
// float hash(vec2 p) {
//     return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
// }
//
// float noise(vec2 p) {
//     vec2 i = floor(p);
//     vec2 f = fract(p);
//
//     vec2 u = f * f * (3.0 - 2.0 * f);
//
//     float a = hash(i + vec2(0.0, 0.0));
//     float b = hash(i + vec2(1.0, 0.0));
//     float c = hash(i + vec2(0.0, 1.0));
//     float d = hash(i + vec2(1.0, 1.0));
//
//     return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
// }
//
// float fbm(vec2 p) {
//     float value = 0.0;
//     float amplitude = 0.5;
//     float frequency = 1.0;
//
//     for (int i = 0; i < 4; i++) {
//         value += amplitude * noise(frequency * p);
//         amplitude *= 0.5;
//         frequency *= 2.0;
//     }
//
//     return value;
// }
//
// void main() {
//     ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
//     vec2 uv = vec2(texelCoord) / params.resolution;
//
//     // Add time-based animation
//     vec2 animatedUV = uv + params.time * 0.1;
//
//     // Generate multiple octaves of noise for more interesting fog
//     float fogValue = fbm(animatedUV * 3.0);
//     fogValue += 0.5 * fbm(animatedUV * 6.0 + vec2(params.time * 0.2));
//     fogValue += 0.25 * fbm(animatedUV * 12.0 - vec2(params.time * 0.3));
//
//     // Normalize and adjust contrast
//     fogValue = fogValue / 1.75;
//     fogValue = smoothstep(0.2, 0.8, fogValue);
//
//     // Output as grayscale
//     vec4 color = vec4(vec3(fogValue), 1.0);
//     imageStore(fogTexture, texelCoord, color);
// }


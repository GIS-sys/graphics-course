#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform params {
    vec4 ambientLight;
    uvec2 iResolution;
    uvec2 iMouse;
    float iTime;
    float fogGeneralDensity;
    float diffuseVal;
    float specPow;
    float specVal;
    float fogWindStrength;
    float fogWindSpeed;
    int objectsAmount;
    int mouseControlType;
    int particleCount;
    int fogDivisions;
    int fogEnabled;
} pc;

// Noise functions for procedural fog
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(hash(i + vec2(0.0, 0.0)),
                   hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)),
                   hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 6; i++) {
        value += amplitude * noise(frequency * p);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

// Fog density function with time animation
float fogDensity(vec3 pos, float time) {
    // Animate with wind
    vec2 wind = vec2(time * 0.1, time * 0.05) * pc.fogWindSpeed;

    // Base density with height falloff
    float heightFactor = exp(-pos.y * 0.1);

    // Animated noise for realistic fog movement
    float density = fbm(pos.xz * 0.01 + wind) * 0.5 +
                   fbm(pos.xz * 0.02 - wind * 1.3) * 0.25;
    density /= pc.fogWindStrength;

    return density * heightFactor * pc.fogGeneralDensity;
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;

    // position
    vec2 mouse = vec2(0.0, 0.0);
    vec3 position = vec3(0.0, 0.0, 0.0);
    if (pc.mouseControlType == 0) {
        mouse = vec2(0.333, 0.0);
    } else if (pc.mouseControlType == 1) {
        mouse = pc.iMouse.xy * 1.0f / pc.iResolution.xy - vec2(0.0, 0.5);
        position = 30.0 * vec3(sin(mouse.x * 6.28), cos(mouse.x * 6.28) * sin(-mouse.y * 6.28), cos(mouse.x * 6.28) * cos(-mouse.y * 6.28));
    } else if (pc.mouseControlType == 2) {
        mouse = pc.iMouse.xy * 1.0f / pc.iResolution.xy - vec2(0.0, 0.5);
    }
    // ray
    vec2 uv = fragCoord / pc.iResolution.xy * 2.0 - 1.0;
    uv.x *= pc.iResolution.x / pc.iResolution.y;
    vec3 ray = vec3(uv[0], uv[1], 1.0);
    ray /= length(ray);
    mat3 camera = mat3(
        1, 0, 0,
        0, cos(-mouse.y * 6.28), -sin(-mouse.y * 6.28),
        0, sin(-mouse.y * 6.28), cos(-mouse.y * 6.28)
    ) * mat3(
        cos(mouse.x * 6.28), 0, sin(mouse.x * 6.28),
        0, 1, 0,
        -sin(mouse.x * 6.28), 0, cos(mouse.x * 6.28)
    );
    ray *= -camera;

    // God rays simulation
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.2));

    int steps = min(pc.fogDivisions, 64);
    float stepSize = 50.0 / float(steps);

    vec3 rayPos = vec3(0.0, 0.0, 0.0);

    float accumulatedLight = 0.0;
    float transmittance = 1.0;

    for (int i = 0; i < steps; i++) {
        rayPos += ray * stepSize;

        float density = fogDensity(rayPos, pc.iTime);
        float scattering = density * stepSize;
        accumulatedLight += scattering * transmittance;

        // Update transmittance (Beer-Lambert law)
        transmittance *= exp(-density * stepSize);
        if (transmittance < 0.01) break;
    }

    // Apply light direction factor
    // float lightFactor = max(0.0, dot(ray, lightDir)) * 2.0;
    // accumulatedLight *= lightFactor;

    // Output fog color (soft white/blue)
    vec3 fogColor = mix(vec3(0.8, 0.3, 1.0), vec3(1.0), accumulatedLight * 0.5);
    outColor = vec4(fogColor, accumulatedLight);
}


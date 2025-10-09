#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(local_size_x = 16, local_size_y = 16) in;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform params
{
    vec4 ambientLight;
    vec4 holeDelta;
    uvec2 iResolution;
    uvec2 iMouse;
    float iTime;
    float fogGeneralDensity;
    float diffuseVal;
    float specPow;
    float specVal;
    float fogWindStrength;
    float fogWindSpeed;
    float holeRadius;
    float holeBorderLength;
    float holeBorderWidth;
    int objectsAmount;
    int mouseControlType;
    int particleCount;
    int fogDivisions;
    int fogEnabled;
};

layout(binding = 0) uniform sampler2D colorTex;
layout(binding = 1) uniform sampler2D fileTex;

struct Particle {
    vec3 position;
    float size;
    vec4 color;
};

layout(binding = 2) buffer ParticlesBuffer {
    Particle particles[];
};

layout(binding = 3) uniform sampler2D fogTex;





// utils

float random(vec2 p) {
    vec2 K1 = vec2(
        23.14069263277926,
        2.665144142690225
    );
    return fract(cos(dot(p,K1)) * 12345.6789);
}

float mmax3(float a, float b, float c) {
    return max(a, max(b, c));
}

float mmax4(float a, float b, float c, float d) {
    return max(max(a, b), max(c, d));
}

float mmax6(float a, float b, float c, float d, float e, float f) {
    return max(mmax3(a, b, c), mmax3(d, e, f));
}

float mmin3(float a, float b, float c) {
    return -mmax3(-a, -b, -c);
}

float mmin4(float a, float b, float c, float d) {
    return -mmax4(-a, -b, -c, -d);
}

float mmin6(float a, float b, float c, float d, float e, float f) {
    return -mmax6(-a, -b, -c, -d, -e, -f);
}

float maxvec3(vec3 a) {
    return mmax3(a.x, a.y, a.z);
}

vec3 mirror(in vec3 target, in vec3 axis) {
    vec3 axis_normalized = axis * dot(target, axis) / length(axis);
    return 2.0 * axis_normalized - target;
}

vec3 clamp_color(in vec3 color) {
    return vec3(
        max(0.0, min(1.0, color[0])),
        max(0.0, min(1.0, color[1])),
        max(0.0, min(1.0, color[2]))
    );
}

float remainder(float a, float b) {
    b = abs(b);
    a -= float(int(a / b)) * b;
    if (a < 0.0) a += b;
    return a;
}

float remainder(float a) { return remainder(a, 1.0); }



// sdf for primitives

float sdf_sphere(in vec3 pos, vec3 center, float radius) {
    return length(pos - center) - radius;
}

float sdf_semispace(in vec3 pos, in vec3 point, in vec3 norm) {
    return dot(point - pos, norm) / length(norm);
}

float sdf_box(in vec3 pos, vec3 corner, vec3 up, vec3 right, vec3 far) {
    return mmax6(
        sdf_semispace(pos, corner, up),
        sdf_semispace(pos, corner + up, -up),
        sdf_semispace(pos, corner, right),
        sdf_semispace(pos, corner + right, -right),
        sdf_semispace(pos, corner, far),
        sdf_semispace(pos, corner + far, -far)
    );
}

// sdf for objects

float sdf_floor(in vec3 pos) {
    vec3 CENTER = vec3(0, -40, 0);
    vec3 NORM = vec3(0, -1, 0);
    return sdf_semispace(pos, CENTER, NORM);
}

float sdf_wall_1(in vec3 pos) {
    vec3 CORNER = vec3(-holeBorderLength / 2, 0, holeRadius) + holeDelta.xyz;
    vec3 UP = vec3(0.0, 0.2, 0.0);
    vec3 RIGHT = vec3(holeBorderLength, 0.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, holeBorderWidth);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}

float sdf_wall_2(in vec3 pos) {
    vec3 CORNER = vec3(-holeBorderLength / 2, 0, -holeRadius) + holeDelta.xyz;
    vec3 UP = vec3(0.0, 0.2, 0.0);
    vec3 RIGHT = vec3(holeBorderLength, 0.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, -holeBorderWidth);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}

float sdf_wall_3(in vec3 pos) {
    vec3 CORNER = vec3(holeRadius, 0, -holeBorderLength / 2) + holeDelta.xyz;
    vec3 UP = vec3(0.0, 0.2, 0.0);
    vec3 RIGHT = vec3(holeBorderWidth, 0.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, holeBorderLength);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}

float sdf_wall_4(in vec3 pos) {
    vec3 CORNER = vec3(-holeRadius, 0, -holeBorderLength / 2) + holeDelta.xyz;
    vec3 UP = vec3(0.0, 0.2, 0.0);
    vec3 RIGHT = vec3(-holeBorderWidth, 0.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, holeBorderLength);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}

float sdf_several(in vec3 pos) {
    vec3 CENTER = vec3(-12, -4, 7);
    float RADIUS = 3.0;
    float ENTIRE_LENGTH = 50.0;
    float deltaBetween = ENTIRE_LENGTH / (objectsAmount + 1);
    float deltaStart = -ENTIRE_LENGTH / 2 + deltaBetween;
    float result = 1000000.0;
    for (int i = 0; i < 4096; ++i) {
        if (i >= objectsAmount) break;
        result = min(result, sdf_sphere(pos, CENTER + deltaStart + deltaBetween * i, RADIUS));
    }
    return result;
}

// sdf for scene

float sdf(in vec3 pos) {
    return mmin6(sdf_floor(pos), sdf_wall_1(pos), sdf_wall_2(pos), sdf_wall_3(pos), sdf_wall_4(pos), sdf_several(pos));
}

vec3 sdf_normal(vec3 point) {
    float DELTA = 0.001;
    return vec3(
        (sdf(vec3(point[0]+DELTA, point[1], point[2])) - sdf(vec3(point[0]-DELTA, point[1], point[2]))) / 2.0 / DELTA,
        (sdf(vec3(point[0], point[1]+DELTA, point[2])) - sdf(vec3(point[0], point[1]-DELTA, point[2]))) / 2.0 / DELTA,
        (sdf(vec3(point[0], point[1], point[2]+DELTA)) - sdf(vec3(point[0], point[1], point[2]-DELTA))) / 2.0 / DELTA
    );
}



// normal - sdf or texture

vec3 get_normal(vec3 point) {
    float dist = sdf(point);
    return sdf_normal(point);
}



// colors for objects

vec3 col_wall(in vec3 pos) {
    return vec3(texture(colorTex, vec2(remainder(pos.x / 100.0), remainder(pos.z / 100.0))));
}

vec3 col_several(in vec3 pos) {
    return vec3(texture(fileTex, vec2(remainder(pos.z / 40.0), remainder(pos.x / 40.0))));
}

vec3 col_floor(in vec3 pos) {
    return vec3(texture(fileTex, vec2(remainder(pos.z / 40.0), remainder(pos.x / 40.0))));
}


// col for scene

vec3 col(in vec3 pos, in vec3 ray) {
    float MIN_STEP = 0.00001;
    float dist = sdf(pos);
    if (dist == sdf_floor(pos)) {
        return col_floor(pos);
    } else if (dist == sdf_several(pos)) {
        return col_several(pos);
    } else {
        return col_wall(pos);
    }
}





// lights

const int LIGHTS_DIRECTIONAL_AMOUNT = 1;
vec3[1] LIGHTS_DIRECTIONAL_DIRECTION = vec3[](
    vec3(0.0, -1.0, 0.3)
);
vec3[1] LIGHTS_DIRECTIONAL_COLOR = vec3[](
    vec3(0.5)
);



// raytracing

vec3 trace(vec3 position, in vec3 ray, out bool hit) {
    float SDF_STEP = 0.8;
    float MAX_STEP = 1000.0;
    float MIN_STEP = 0.00001;
    hit = true;
    vec3 ray_step = ray / length(ray);
    for (int i = 0; i < 500; ++i) {
        float step_size = sdf(position);

        if (step_size < 0.0) break;
        if (step_size < MIN_STEP) return position;
        if (step_size > MAX_STEP) break;

        position += step_size * ray_step * SDF_STEP;
    }
    hit = false;
    return vec3(0.0);
}

vec3 get_light_including_shadows(int directional_light_index, vec3 point)  // TODO
{
    bool shadow = false;
    trace(point - LIGHTS_DIRECTIONAL_DIRECTION[directional_light_index] * 0.01, -LIGHTS_DIRECTIONAL_DIRECTION[directional_light_index], shadow);
    if (shadow) return vec3(0.0);
    return LIGHTS_DIRECTIONAL_COLOR[directional_light_index];
}



// calculating Phong color

vec3 get_color(in vec3 point, in vec3 ray) {
    vec3 norm = get_normal(point);
    vec3 mirrored_ray = -mirror(ray, norm);
    // ambient
    vec3 result = ambientLight.xyz;
    // diffuse
    for (int i = 0; i < LIGHTS_DIRECTIONAL_AMOUNT; ++i) {
        vec3 light_direction = LIGHTS_DIRECTIONAL_DIRECTION[i];
        float cos_angle = dot(-light_direction, norm) / length(light_direction) / length(norm);
        if (cos_angle > 0.0) {
            result += cos_angle * diffuseVal * get_light_including_shadows(i, point);
        }
    }
    // specular
    for (int i = 0; i < LIGHTS_DIRECTIONAL_AMOUNT; ++i) {
        vec3 light_direction = LIGHTS_DIRECTIONAL_DIRECTION[i];
        if (dot(-light_direction, norm) > 0.0) {
            float cos_angle = abs(dot(-light_direction, mirrored_ray)) / length(light_direction) / length(mirrored_ray);
            result += pow(cos_angle, specPow) * specVal * get_light_including_shadows(i, point);
        }
    }
    // apply texture
    result *= col(point, mirrored_ray);
    // return result
    return result;
}

bool intersectParticle(vec3 rayOrigin, vec3 rayDir, vec3 particlePos, float particleSize, out float t, out vec4 particleColor) {
    vec3 oc = rayOrigin - particlePos;
    float a = dot(rayDir, rayDir);
    float b = 2.0 * dot(oc, rayDir);
    float c = dot(oc, oc) - particleSize * particleSize;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        return false;
    }

    float sd = sqrt(discriminant);
    t = (-b - sd) / (2.0 * a);
    if (t < 0.0) {
      t = (-b + sd) / (2.0 * a);
      if (t < 0.0) {
          return false;
      }
    }

    return true;
}

// Modified trace function to handle particle transparency
vec4 trace_transparent(vec3 position, vec3 ray, out bool hit, out float distance) {
    float SDF_STEP = 0.8;
    float MAX_STEP = 1000.0;
    float MIN_STEP = 0.0001;

    vec3 currentPos = position;
    vec3 accumulatedColor = vec3(0.0);
    float accumulatedAlpha = 0.0;
    float accumulatedSteps = 0.0;
    vec3 ray_step = ray / length(ray);
    distance = -1.0;

    for (int i = 0; i < 500; ++i) {
        // First check SDF
        float sdf_dist = sdf(currentPos);

        // Check particle collisions
        float closestParticleT = MAX_STEP;
        vec4 closestParticleColor = vec4(0.0);
        int closestParticleIndex = -1;

        for (int p = 0; p < min(particleCount, 500); p++) {
            float t;
            vec4 particleColor;
            if (intersectParticle(currentPos, ray_step, particles[p].position, particles[p].size, t, particleColor)) {
                if (t < closestParticleT) {
                    closestParticleT = t;
                    closestParticleColor = particles[p].color;
                    closestParticleIndex = p;
                }
            }
        }

        // Determine the closest hit (SDF or particle)
        if (sdf_dist < MIN_STEP) {
            // Hit scene geometry
            hit = true;
            if (distance < 0) distance = accumulatedSteps;
            vec3 sceneColor = get_color(currentPos, ray);
            return vec4(mix(accumulatedColor, sceneColor, 1.0 - accumulatedAlpha), 1.0);
        }
        else if (closestParticleT < sdf_dist && closestParticleT < MAX_STEP) {
            // Hit a particle
            vec3 hitPos = currentPos + ray_step * closestParticleT;
            vec4 particleColor = closestParticleColor;

            // Blend with accumulated color
            accumulatedColor = accumulatedColor + particleColor.rgb * particleColor.a * (1.0 - accumulatedAlpha);
            accumulatedAlpha = accumulatedAlpha + particleColor.a * (1.0 - accumulatedAlpha);

            // Continue tracing from just beyond the particle
            currentPos = hitPos + ray_step * 0.01;
            accumulatedSteps += 0.01 + closestParticleT;
            if (distance < 0) distance = accumulatedSteps;

            if (accumulatedAlpha > 0.99) {
                hit = true;
                return vec4(accumulatedColor, 1.0);
            }

            continue;
}
        else {
            // No hit, continue marching
            float step_size = min(sdf_dist, closestParticleT);
            accumulatedSteps += step_size;;

            if (accumulatedSteps > MAX_STEP) {
                hit = false;
                return vec4(accumulatedColor, accumulatedAlpha);
            }

            currentPos += step_size * ray_step * SDF_STEP;
        }
    }

    hit = false;
    return vec4(accumulatedColor, accumulatedAlpha);
}



vec3 apply_fog(vec3 color, vec3 position, vec3 ray, float distance)  // TODO
{
    const float FOG_FULL_DISTANCE = 1000;
    if (distance < 0) distance = 100000;

    float fog_divisions = min(128, fogDivisions);

    vec3 result_color = vec3(0.0, 0.0, 0.0);
    float stepf = distance / fog_divisions;
    for (int i = 0; i < min(128, fog_divisions); ++i) {
        vec3 fog_spot = position + (i + 0.5) * stepf * ray;
        for (int j = 0; j < LIGHTS_DIRECTIONAL_AMOUNT; ++j) {
            result_color = result_color + get_light_including_shadows(j, fog_spot) / (fog_divisions * 1.0); // * (1.0 + dot(LIGHTS_DIRECTIONAL_DIRECTION[j], ray));
        }
    }
    // get fog texture
    vec2 fogUV = gl_FragCoord.xy / iResolution.xy;
    vec4 fogData = texture(fogTex, fogUV);

    // calculate total
    float fog_density = fogGeneralDensity * fogData.a;
    return mix(color, result_color * fogData.rgb, fog_density);
}




void main()
{
    LIGHTS_DIRECTIONAL_DIRECTION[0] += vec3(cos(iTime * 4) / 4, 0, 0);

    vec2 fragCoord = gl_FragCoord.xy;

    // position
    vec2 mouse = vec2(0.0, 0.0);
    vec3 position = vec3(0.0, 0.0, 0.0);
    if (mouseControlType == 0) {
        mouse = vec2(0.333, 0.0);
    } else if (mouseControlType == 1) {
        mouse = iMouse.xy * 1.0f / iResolution.xy - vec2(0.0, 0.5);
        position = 30.0 * vec3(sin(mouse.x * 6.28), cos(mouse.x * 6.28) * sin(-mouse.y * 6.28), cos(mouse.x * 6.28) * cos(-mouse.y * 6.28));
    } else if (mouseControlType == 2) {
        mouse = iMouse.xy * 1.0f / iResolution.xy - vec2(0.0, 0.5);
    }
    // ray
    vec2 uv = fragCoord / iResolution.xy * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
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

    // Use the new transparent trace function
    vec3 result_color = vec3(0.0, 0.0, 0.0);
    bool hit = false;
    float distance = 0;
    vec4 color = trace_transparent(position, ray, hit, distance);
    if (hit) {
        result_color = vec3(color);
    } else {
        result_color = mix(vec3(color), vec3(0.1, 0.1, 0.3), 1 - color.a);
    }

    if (fogEnabled == 1)
        result_color = apply_fog(result_color, position, ray, distance);
    fragColor = vec4(result_color, 1.0);
}


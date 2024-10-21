#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(local_size_x = 16, local_size_y = 16) in;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform params
{
  uvec2 iResolution;
  uvec2 iMouse;
  double iTime;
};

layout(binding = 0) uniform sampler2D colorTex;
layout(binding = 1) uniform sampler2D fileTex;






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

float sdf_wall(in vec3 pos) {
    vec3 CENTER = vec3(0, 45, 0);
    vec3 NORM = vec3(0, 1, 0);
    return sdf_semispace(pos, CENTER, NORM);
}

float sdf_road(in vec3 pos) {
    vec3 CORNER = vec3(-200, 45, -40);
    vec3 UP = vec3(0, -5, 0);
    vec3 RIGHT = vec3(0, 0, 80);
    vec3 FAR = vec3(400, 0, 0);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}

float sdf_ball(in vec3 pos) {
    vec3 CENTER = vec3(5, -5, 15);
    float RADIUS = 3.0;
    return sdf_sphere(pos, CENTER, RADIUS);
}

float sdf_melon(in vec3 pos) {
    vec3 CENTER = vec3(-15, -5, 15);
    float RADIUS = 5.0;
    return sdf_sphere(pos, CENTER, RADIUS) + dot(sin((pos - CENTER) * 2.0), vec3(1.0, 1.0, 1.0)) / 20.0;
}

float sdf_box(in vec3 pos) {
    vec3 CORNER = vec3(0, 0, 10);
    vec3 UP = vec3(8.0, 8.0, 0.0);
    vec3 RIGHT = vec3(-8.0, 8.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, 8.0);
    return sdf_box(pos, CORNER, UP, RIGHT, FAR);
}


float sdf_cheese(in vec3 pos) {
    vec3 CORNER = vec3(-13.2, -2.7, 6);
    vec3 UP = vec3(3.0, 0.0, 1.5);
    vec3 RIGHT = vec3(0.0, 2.0, 0.0);
    vec3 FAR = vec3(-1, 0.0, 2.0);
    vec3 CENTER = vec3(-12, -4, 7);
    float RADIUS = 3.0;
    return max(sdf_sphere(pos, CENTER, RADIUS), sdf_box(pos, CORNER, UP, RIGHT, FAR));
}

// sdf for scene

float sdf(in vec3 pos) {
    return mmin6(sdf_wall(pos), sdf_road(pos), sdf_ball(pos), sdf_melon(pos), sdf_box(pos), sdf_cheese(pos));
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
    return vec3(1.0, 1.0, 1.0);
    //return vec3(texture(colorTex, vec2(remainder(pos.x / 100.0), remainder(pos.z / 100.0))));
}

vec3 col_road(in vec3 pos) {
    return vec3(1.0, 1.0, 0.5);
    //return vec3(texture(fileTex, vec2(remainder(pos.z / 40.0), remainder(pos.x / 40.0))));
}

vec3 col_box(in vec3 pos) {
    // TASK PART 2 - triplanar projection
    vec3 CORNER = vec3(0, 0, 10);
    vec3 UP = vec3(8.0, 8.0, 0.0);
    vec3 RIGHT = vec3(-8.0, 8.0, 0.0);
    vec3 FAR = vec3(0.0, 0.0, 8.0);
    vec3 CENTER = CORNER + (UP + RIGHT + FAR) / 2.0;

    vec3 norm = get_normal(pos);
    vec3 closest_plane_normal = vec3(
        (norm.x == maxvec3(norm)) ? 1.0 : 0.0,
        (norm.y == maxvec3(norm)) ? 1.0 : 0.0,
        (norm.z == maxvec3(norm)) ? 1.0 : 0.0
    );
    vec3 pro_base_1 = normalize(vec3(1, 1, 1));
    vec3 pro_base_2 = cross(pro_base_1, closest_plane_normal);
    vec3 v_to_pro = pos - CENTER;
    float texture_x = dot(v_to_pro, pro_base_1) / length(pro_base_1) / 4.0;
    float texture_y = dot(v_to_pro, pro_base_2) / length(pro_base_2) / 4.0;
    return vec3(0.5, 0.0, 0.0);
    //return vec3(texture(fileTex, vec2(texture_x, texture_y)));
}


// col for scene

vec3 col(in vec3 pos, in vec3 ray) {
//return get_normal(pos);
    float MIN_STEP = 0.00001;
    float dist = sdf(pos);
    if (dist == sdf_wall(pos)) {
        return col_wall(pos);
    } else if (dist == sdf_road(pos)) {
        return col_road(pos);
    } else if (dist == sdf_box(pos)) {
        return col_box(pos);
    } else {
        return vec3(1.0, 1.0, 1.0);
    }
}





// lights

const int LIGHTS_DIRECTIONAL_AMOUNT = 4;
vec3[LIGHTS_DIRECTIONAL_AMOUNT] LIGHTS_DIRECTIONAL_DIRECTION = vec3[](
    vec3(0.0, 1.0, 0.0),
    vec3(0.5, 1.0, 2.0),
    vec3(-1.0, 1.0, 0.0),
    vec3(3, 1.0, -2.0)
);
vec3[LIGHTS_DIRECTIONAL_AMOUNT] LIGHTS_DIRECTIONAL_COLOR = vec3[](
    vec3(0.5),
    vec3(1.0, 0.0, 0.0) * 0.5,
    vec3(0.3, 0.3, 0.85) * 0.5,
    vec3(0.0, 1.0, 0.0) * 0.5
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



// calculating Phong color

vec3 get_color(in vec3 point, in vec3 ray) {
    vec3 AMBIENT_LIGHT = vec3(0.4, 0.4, 0.4);
    float DIFFUSE_VAL = 0.3;
    float SPEC_POW = 20.0;
    float SPEC_VAL = 0.8;
    vec3 norm = get_normal(point);
    vec3 mirrored_ray = -mirror(ray, norm);
    // ambient
    vec3 result = AMBIENT_LIGHT;
    // diffuse
    for (int i = 0; i < LIGHTS_DIRECTIONAL_AMOUNT; ++i) {
        vec3 light_direction = LIGHTS_DIRECTIONAL_DIRECTION[i];
        vec3 light_color = LIGHTS_DIRECTIONAL_COLOR[i];
        float cos_angle = dot(-light_direction, norm) / length(light_direction) / length(norm);
        if (cos_angle > 0.0) {
            // check shadow
            bool shadow = false;
            trace(point-light_direction*0.01, -light_direction, shadow); // TODO separate sdfs? or how to cast ray
            if (!shadow) result += cos_angle * DIFFUSE_VAL * light_color;
        }
    }
    // specular
    for (int i = 0; i < LIGHTS_DIRECTIONAL_AMOUNT; ++i) {
        vec3 light_direction = LIGHTS_DIRECTIONAL_DIRECTION[i];
        vec3 light_color = LIGHTS_DIRECTIONAL_COLOR[i];
        if (dot(-light_direction, norm) > 0.0) {
            float cos_angle = abs(dot(-light_direction, mirrored_ray)) / length(light_direction) / length(mirrored_ray);
            // check shadow
            bool shadow = false;
            trace(point-light_direction*0.01, -light_direction, shadow); // TODO separate sdfs? or how to cast ray
            if (!shadow) result += pow(cos_angle, SPEC_POW) * SPEC_VAL * light_color;
        }
    }
    // apply texture
    result *= col(point, mirrored_ray);
    // return result
    return result;
}



// main logic

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;

    // position
    vec2 mouse = iMouse.xy * 1.0f / iResolution.xy - vec2(0.0, 0.5);
    vec3 position = 30.0 * vec3(sin(mouse.x * 6.28), cos(mouse.x * 6.28) * sin(-mouse.y * 6.28), cos(mouse.x * 6.28) * cos(-mouse.y * 6.28));
    //position *= 0.0;
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

    vec3 result_color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < 10; ++i) {
        bool hit = false;
        vec3 point = trace(position, ray, hit);
        float current_color_power = pow(2.0, float(-i) * 2.0);
        if (hit) {
            vec3 color = clamp_color(get_color(point, ray));
            result_color += color * current_color_power;
            position = point - ray * 0.01; // TODO separate sdfs? or how to cast ray
            ray = -mirror(ray, get_normal(point));
        }
    }
    fragColor = vec4(result_color, 1.0);
}

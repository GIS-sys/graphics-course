#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Camera {
    mat4 viewProj;
    vec3 cameraPos;
} cam;

void main() {
    fragColor = inColor;
    
    // Billboard effect - always face camera
    vec3 pos = inPosition;
    vec3 toCamera = normalize(cam.cameraPos - pos);
    vec3 right = cross(toCamera, vec3(0.0, 1.0, 0.0));
    vec3 up = cross(right, toCamera);
    
    // Expand quad based on size
    uint vertexId = gl_VertexIndex % 4;
    vec2 offsets[4] = vec2[](
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(-1.0, 1.0),
        vec2(1.0, 1.0)
    );
    
    vec2 offset = offsets[vertexId];
    pos += (right * offset.x + up * offset.y) * inSize * 0.5;
    
    gl_Position = cam.viewProj * vec4(pos, 1.0);
}


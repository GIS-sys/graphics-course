#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outMousePos;

layout(push_constant) uniform params
{
    uvec2 iResolution;
    uvec2 iMouse;
    float iTime;
    int objectsAmount;
    int mouseControlType;
};

void main()
{
    // Fullscreen triangle vertices
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2(3.0, -1.0),
        vec2(-1.0, 3.0)
    );
    
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    outUV = (pos + 1.0) * 0.5;
    outMousePos = vec3(iMouse.xy, 0.0);
}


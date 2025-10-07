#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    //outColor = fragColor;
    outColor = vec4(1.0, 0.0, 0.0, 0.5);
    
    //// Simple circular particle
    //vec2 coord = gl_PointCoord * 2.0 - 1.0;
    //float alpha = 1.0 - dot(coord, coord);
    //if (alpha <= 0.0) {
    //    discard;
    //}
    //outColor.a *= alpha;
}


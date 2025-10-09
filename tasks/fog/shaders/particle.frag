#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inMousePos;
layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform sampler2D inputTexture;

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

void main()
{
    // Get the current color from previous rendering
    vec4 color = texture(inputTexture, inUV);

    // float delta = 0.001;
    // color = (texture(inputTexture, inUV + vec2( delta,  delta)) +
    //          texture(inputTexture, inUV + vec2(-delta,  delta)) +
    //          texture(inputTexture, inUV + vec2( delta, -delta)) +
    //          texture(inputTexture, inUV + vec2(-delta, -delta))) / 4;
    
    // Calculate distance from current pixel to mouse position
    vec2 fragCoord = inUV * iResolution;
    float distanceToMouse = length(fragCoord - iMouse.xy);
    
    // If close to mouse position, invert the color
    float mouseRadius = 50.0; // Radius in pixels
    if (distanceToMouse < mouseRadius) {
        // Smooth transition at the edge
        float transition = smoothstep(0.0, mouseRadius, distanceToMouse);
        color.rgb = mix(vec3(1.0) - color.rgb, color.rgb, transition);
    }
    
    outFragColor = color;
}


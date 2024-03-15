#version 450

// Input data
layout(location = 0) in vec3 fragColor;

// Output data
layout(location = 0) out vec4 outColor;


void main()
{
    outColor = vec4(fragColor, 1.0);
}

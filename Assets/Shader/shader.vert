#version 450 		// Use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

/*
* set    : Means that the variable is a set of variables, across every single vertex.
*          Can have multiple bindings to the given set.
* binding: Means that the uniform buffer object will be bound to the given index.
*          Binding goes for each type of input. Binding 0 for "IN", "OUT" and "UNIFORM" are different.
* uniform: Means that the variable is a uniform variable, across every single vertex.
*
* Descriptor set is all the bindings, including everithing in an array at a binding index.
*/
layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 model;
} mvp;

layout(location = 0) out vec3 fragCol;

void
main()
{
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(pos, 1.0);
    fragCol = col;
}
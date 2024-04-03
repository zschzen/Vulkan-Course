#version 450 		// Use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;

/*
    * set    : Means that the variable is a set of variables (Descriptor Sets), across every single vertex.
    *          Can have multiple bindings to the given set.
    *          Implicitly, the set is 0.
    * binding: Means that the uniform buffer object will be bound to the given index.
    *          Binding goes for each type of input. Binding 0 for "IN", "OUT" and "UNIFORM" are different.
    * uniform: Means that the variable is a uniform variable, across every single vertex.
    *
    * Descriptor set is all the bindings, including everithing in an array at a binding index.
*/
layout(set = 0, binding = 0) uniform UBOViewProjection
{
    mat4 proj;
    mat4 view;
} ubo_vp;

/* Separate binding for each uniform buffer object, due to Dynamic Uniform Buffers. */
/* LEGACY: Left for reference about Dynamic Uniform Buffers. */
layout(set = 0, binding = 1) uniform UBOModel
{
    mat4 model;
} ubo_model;

/*
    * Push Constants are a way to pass data to the shader stage, without using a buffer.
    * They are limited in size, and are used for small amounts of data.
    * They are used to pass data to the
    * shader stage, without using a buffer.
    * They are limited in size, and are used for small amounts of data.
    * They are used to pass data to the shader stage, without using a buffer.
*/
layout(push_constant) uniform PushModel
{
    mat4 model;
} push_model;

/** Outputs */
layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTex;

void
main()
{
  gl_Position = ubo_vp.proj * ubo_vp.view * push_model.model * vec4(pos, 1.0);
	
  fragCol = col;
  fragTex = tex;
}

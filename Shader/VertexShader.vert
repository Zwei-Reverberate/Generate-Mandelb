#version 400 core

layout (location = 0) in vec3 Position;

uniform float u_nearPlane;
uniform float u_farPlane;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


out vec3 vertRayOrigin;
out vec3 vertRayDirection;

void main()
{
    mat4 u_inverseVP = inverse(view)*inverse(projection);
    vec4 farPlane = u_inverseVP * vec4(Position.xy, u_nearPlane, 1.0);
    vec4 nearPlane = u_inverseVP * vec4(Position.xy, u_farPlane, 1.0);

    farPlane /= farPlane.w;
    nearPlane /= nearPlane.w;

    vertRayOrigin = nearPlane.xyz;
    vertRayDirection = (farPlane.xyz - nearPlane.xyz);

    gl_Position = vec4(Position.xy, 0.0, 1.0);
}

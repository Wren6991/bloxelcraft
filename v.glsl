#version 110

uniform vec3 posoffset;

varying vec3 pos;
varying vec3 normal;
varying float light;

const vec3 lightdir = vec3(0.5774, 0.5774, 0.5774);

void main()
{
    gl_Position = gl_ProjectionMatrix * (gl_Vertex + vec4(posoffset, 0.0));
    pos = gl_Vertex.xyz;
    light = 0.5 + 0.5 * max(0.0, dot(gl_Normal, lightdir));
    normal = gl_Normal;
}
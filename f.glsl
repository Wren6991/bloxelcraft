#version 110

precision highp float;

uniform vec3 posoffset;
uniform sampler3D blocktexture;
uniform sampler2DArray facetextures;

varying vec3 pos;
varying vec3 normal;
varying float light;

void main()
{
    //gl_FragColor = vec4(vec3((pos.y + posoffset.y) * 0.05 + 0.2) * light, 1.0);
    float blockid = texture3D(blocktexture, (pos - normal * 0.01 + 0.0001).zyx * vec3(1.0 / 32.0)).r * 256;

    //gl_FragColor = vec4(texture3D(blocktexture, (pos - normal * 0.01 + 0.0001).zyx * vec3(1.0 / 32.0)).rrr * 100, 1.0);

	
    vec2 texcoord;
    if (normal.x * normal.x > dot(normal.yz, normal.yz))
    	texcoord = pos.yz;
    else if (normal.y * normal.y > dot(normal.xz, normal.xz))
	texcoord = pos.xz;
    else
        texcoord = pos.xy;

    gl_FragColor = vec4(texture2DArray(facetextures, vec3(texcoord, blockid)).rgb * light, 1.0);
}
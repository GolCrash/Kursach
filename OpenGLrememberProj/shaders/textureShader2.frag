varying vec3 Normal;
varying vec3 Position;

void main(void)
{
    vec3 c1 = vec3(0.992,0.416,0.008);
    vec3 c2 = vec3(1.0,0.0,0.0);
    vec3 c3 = vec3(0.992,0.416,0.008);
    
    vec3 c = mix(c1,c2,Position.z);
    vec3 l = mix(c3,c,Position.z);

    gl_FragColor = vec4(l, 0.9);  
}
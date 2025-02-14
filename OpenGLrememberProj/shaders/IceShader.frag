varying vec3 Normal;
varying vec3 Position;

uniform vec3 Ia;
uniform vec3 Id;
uniform vec3 Is;

uniform vec3 light_pos;

uniform vec3 ma;
uniform vec3 md;
uniform vec4 ms;

uniform vec3 camera;

void main(void)
{
	vec3 color_amb = Ia*ma;
	
	vec3 light_vector = normalize(light_pos-Position);
	vec3 color_dif = Id*md*dot( light_vector,Normal  );
	
	vec3 view_vector = normalize(camera - Position);
	vec3 r = reflect(light_vector,Normal);
	vec3 color_spec = Is*ms.xyz*pow(max(0.0,dot(-r,view_vector)),ms.w);
	
	vec3 c1 = vec3(0.118,0.565,1.0);
    vec3 c2 = vec3(0.0,0.0,1.0);
    vec3 c3 = vec3(0.118,0.565,1.0);

    vec3 c = mix(c1,c2,Position.z);
    vec3 l = mix(c3,c,Position.z);
	
	gl_FragColor = vec4(color_amb + color_dif + color_spec,0.5) + vec4(l,0);
	
	
}
uniform sampler2D fieldX;
uniform sampler2D fieldY;
uniform mat3 transformation;

attribute float age;
attribute float max_age;
attribute float brightness;

varying mat3 trans;

void main()
{
  vec2 velocity = vec2(texture2D(fieldX, gl_Vertex.xy).x, texture2D(fieldY, gl_Vertex.xy).y);
 float fac = clamp(0.0, 1000.0, 4000.0*length(velocity));
  float theta = mix(0, atan(velocity.y, velocity.x)-3.145926/2, fac);
  theta = 100*length(velocity);
  float cost = cos(theta), sint = sin(theta);
  trans = transpose(mat3( cost,-sint,  0.5*sint - 0.5*cost + 0.5,
                sint, cost, -0.5*sint - 0.5*cost + 0.5,
                 0.0,  0.0,  1.0));
	vec3 p = transformation * gl_Vertex.xyw;
  gl_Position = gl_ModelViewProjectionMatrix * vec4(p.x, p.y, 0, p.z);  
  float a = clamp(abs(max_age/2.0 - age) / (max_age/2.0), 0, 1);
  a = (1 - a * a * (3-2*a)) * brightness;
  gl_PointSize = 10 + 40*(1-a);
	gl_FrontColor = vec4(0.255, 0.4, 0.96, a);
}

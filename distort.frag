uniform sampler2D background;
uniform sampler2D fieldX;
uniform sampler2D fieldY;
uniform vec2 fieldSize;
uniform vec2 size;

const vec2 poisson[16]=vec2[16](
	vec2(0.007937789, 0.73124397),
	vec2(-0.10177308, -0.6509396),
	vec2(-0.9906806, -0.63400936),
	vec2(-0.5583586, -0.3614012),
	vec2(0.7163085, 0.22836149),
	vec2(-0.65210974, 0.37117887),
	vec2(-0.12714535, 0.112056136),
	vec2(0.48898065, -0.66669613),
	vec2(-0.9744036, 0.9155904),
	vec2(0.9274436, -0.9896486),
	vec2(0.9782181, 0.90990245),
	vec2(0.96427417, -0.25506377),
	vec2(-0.5021933, -0.9712455),
	vec2(0.3091557, -0.17652994),
	vec2(0.4665941, 0.96454906),
	vec2(-0.461774, 0.9360856)
);


const int rad = 5;

void main(void) {
	float tx = texture2D(fieldX, gl_TexCoord[0].st).x;
  float ty = texture2D(fieldY, gl_TexCoord[0].st).x;
	vec2 off = vec2(tx, ty);

	vec2 onePix = 1.0/size;
	vec2 o = onePix*12;
  vec2 o2 = o*5*(1+0*dot(off,off));
	for (int i=0; i < 16; ++i) {
		gl_FragColor += texture2D(background, gl_TexCoord[0].st + poisson[i]*o);
    gl_FragColor += texture2D(background, gl_TexCoord[0].st + poisson[i]*o2);
	}
  gl_FragColor /= 16*2;
#if 1
	float l = length(gl_FragColor);
  l -= length(off);
	vec4 inte = vec4(0.6, 0.1, 0.1, 0.9);
	vec4 exte = 0.1*vec4(0.12, 0.24, 0.17, 10.15);
  gl_FragColor = mix(inte, exte, l);
//  gl_FragColor *= clamp(1+length(off), 1.0, 1.5);
#endif
//gl_FragColor = texture2D(background, gl_TexCoord[0].st);
}

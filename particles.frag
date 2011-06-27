uniform sampler2D tex;
uniform sampler2D fieldX;
uniform sampler2D fieldY;

varying mat3 trans;

void main() {
  gl_FragColor = gl_Color;
  vec3 txcoord = trans * vec3(gl_PointCoord.xy, 1.0);
  gl_FragColor.a *= texture2D(tex, txcoord.xy).x;
}

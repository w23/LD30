uniform sampler2D us2_noise;
uniform mat4 um4_view;
vec2 uv2_res = vec2(1280.,720.);

const float _d=.01;
const float FAR=30.;

//float noise1(float f) { return texture2D(us2_noise,vec2(f/1024.,0.)).r; }
float noise1(vec2 f) { return texture2D(us2_noise,(vec2(.5)+f)/1024.,-100.).r; }
float noise1s(vec2 f) { vec2 F = floor(f), ff = fract(f); ff*=ff*(3.-2.*ff); return noise1(F+ff);}

float fnoise1(vec2 f) {
  float r=.5*noise1s(f);
  r+=.25*noise1s(f*2.);
  r+=.125*noise1s(f*4.);
  r+=.0625*noise1s(f*8.);
  r+=.03125*noise1s(f*16.);
  return r*r*r;
}

float height(vec3 o, vec3 c) {
  vec3 v=o-c;
  return fnoise1(v.zx*.9)+fnoise1(v.yx*.7);
}

vec3 M(vec3 o) {
  vec3 c = vec3(0.);
  vec3 v = o - c;
  float H = height(o,c);
  const vec3 grass=vec3(.31,.85,.44);
  const vec3 rock=vec3(.31,.25,.34);
  const vec3 water=vec3(.44,.68,.89);
  return mix(water,mix(grass,rock,fnoise1(v.yz*13.)+.37),step(.17,H));
}

float Wenv(vec3 o) {
  return length(o) - 10.;
}

const float R = .4;

float W(vec3 o) {
  return .9*(length(o) - 10. + R*(1. - height(o, vec3(0.))));
}

//float Wenv(vec3 o) { return W(o); }

vec3 N(vec3 o) {
  float w = W(o);
  vec2 e = vec2(.001,.0);
  return normalize(vec3(W(o+e.xyy)-w,W(o+e.yxy)-w,W(o+e.yyx)-w));
}

float Tenv(vec3 o, vec3 r, float Lmax) {
  float L, Lp = L = 0.;
  for (int i = 0; i < 64; ++i) {
    float d = Wenv(o+r*L);
    if (d < _d)
      break;
    Lp = L; L+=d+_d;
    if (L > Lmax)
      return L;
  }

  for (int i = 0; i < 4; ++i) {
    float dl = (L-Lp)*.5;
    float d = Wenv(o+r*(Lp+dl));
    if (d < _d) L -= dl; else Lp += dl;
  }

  return L;
}

float T(vec3 o, vec3 r, float Lmax) {
  float L, Lp = L = 0.;
  for (int i = 0; i < 64; ++i) {
    float d = W(o+r*L);
    if (d < _d)
      break;
    Lp = L; L+=d+_d;
    if (L > Lmax)
      return L;
  }

  for (int i = 0; i < 8; ++i) {
    float dl = (L-Lp)*.5;
    float d = W(o+r*(Lp+dl));
    if (d < _d) L -= dl; else Lp += dl;
  }

  return L;
}

float Tcoarse(vec3 o, vec3 r, float Lmax) {
  float L, Lp = L = 0.;
  for (int i = 0; i < 64; ++i) {
    float d = W(o+r*L);
    if (d < _d)
      break;
    L+=d+_d;
    if (L > Lmax)
      break;
  }
  return L;
}

vec3 enlight_dir(vec3 e, vec3 p, vec3 n, vec3 c, vec3 Ld, vec3 Lc, vec3 sp, float Lmax) {
  float umbra = smoothstep(Lmax*.9,Lmax,Tcoarse(p+n*_d,Ld,Lmax));
  return max(0.,dot(n,Ld))*Lc*c*umbra;
}

void main(){
  float aspect = uv2_res.x / uv2_res.y;
  vec2 p = gl_FragCoord.xy * 2. / uv2_res - 1.;;
  p.x *= aspect;

  mat4 inview = inverse(um4_view);

  vec3 O = (inview*vec4(0.,0.,0.,1.)).xyz;
  vec3 D = (inview*normalize(vec4(p,-2.,0.))).xyz;
  float L = Tenv(O, D, FAR);
  
  vec3 color = vec3(D);//*.5+1.);

  if (L < FAR) {
    float LL = T(O+D*L,D,5.);
    if (LL < 5.)
      L += LL;
    else
      L = FAR;
  }

  if (L < FAR) {
    vec3 P = O + D * L;
    color = enlight_dir(D,P,N(P),M(P),normalize(vec3(1.)),vec3(1.),vec3(0.),10.);
  }

  gl_FragColor=vec4(pow(color,vec3(.7)),0.);
}


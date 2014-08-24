uniform float t;
uniform sampler2D us2_noise;
uniform mat4 um4_view;
vec2 uv2_res = vec2(1280.,720.);

const float _d=.01;
const float FAR=30.;

const vec3 w1_pos = vec3(0.);
const float w1_R = 10.;
const float w1_r = .4;
const float w1_rw = .17;
const vec4 w1_m0h = vec4(0.,0.,w1_rw,w1_rw);
const vec4 w1_m0f = vec4(0.,0.,0.,0.);
const vec3 w1_m0d = vec3(.24,.48,.89);
const vec4 w1_m0s = vec4(1.,1.,1.,100.);

const vec4 w1_m1h = vec4(w1_rw,.5,.5,.6);
const vec4 w1_m1f = vec4(13.,.37,2.,.63);
const vec3 w1_m1d = vec3(.27, .87, .41);
const vec4 w1_m1s = vec4(0.);

const vec4 w1_m2h = vec4(.3,.6,.9,1.);
const vec4 w1_m2f = vec4(11.,1.5,23.,.5);
const vec3 w1_m2d = vec3(.31,.25,.34);
const vec4 w1_m2s = vec4(0.);


vec3 w2_pos = vec3(14.*sin(t*.1),0.,14.*cos(t*.1));
const float w2_R = 4.4;
const float w2_r = .4;
const float w2_rw = .17;
const vec4 w2_m0h = vec4(0.,0.,w2_rw,w2_rw);
const vec4 w2_m0f = vec4(0.,0.,0.,0.);
const vec3 w2_m0d = vec3(.24,.48,.89);
const vec4 w2_m0s = vec4(1.,1.,1.,100.);

const vec4 w2_m1h = vec4(w2_rw,.5,.5,.6);
const vec4 w2_m1f = vec4(13.,.37,2.,.63);
const vec3 w2_m1d = vec3(.27, .87, .41);
const vec4 w2_m1s = vec4(0.);

const vec4 w2_m2h = vec4(.3,.6,.9,1.);
const vec4 w2_m2f = vec4(11.,1.5,23.,.5);
const vec3 w2_m2d = vec3(.31,.25,.34);
const vec4 w2_m2s = vec4(0.);

float noise1(float f) { return texture2D(us2_noise,vec2(f/1024.,0.)).r; }
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
  float h = fnoise1(v.zx*.9)+fnoise1(v.yx*.7);
  return max(w1_rw*.99,h);
}

float W1_env(vec3 o) {return length(o-w1_pos) - w1_R;}
float W1(vec3 o) {return .9*(W1_env(o) + w1_r*(1. - height(o, w1_pos)));}

float W2_env(vec3 o) {return length(o-w2_pos) - w2_R;}
float W2(vec3 o) {return .9*(W2_env(o) + w2_r*(1. - height(o, w2_pos)));}

float Wenv(vec3 o) {
  return min(W1_env(o),W2_env(o));
}

float W(vec3 o) {
  float k = .5;
  float w1 = W1(o), w2 = W2(o);
  float h = clamp(.5+.5*(w2-w1)/k, .0, 1.);
  return mix(w2, w1, h) - k*h*(1.0-h);
}

vec3 mat_diffuse;
vec4 mat_specular;

void W1_m(vec3 o) {
  vec3 v = o - w1_pos;
  float H = height(o,w1_pos);

  float m0h = smoothstep(w1_m0h.x,w1_m0h.y,H) * (1.-smoothstep(w1_m0h.z,w1_m0h.w,H));
  float m1h = smoothstep(w1_m1h.x,w1_m1h.y,H) * (1.-smoothstep(w1_m1h.z,w1_m1h.w,H));
  float m2h = smoothstep(w1_m2h.x,w1_m2h.y,H) * (1.-smoothstep(w1_m2h.z,w1_m2h.w,H));

  m1h *= fnoise1(v.zx*w1_m1f.x) *w1_m1f.y + fnoise1(v.yx*w1_m1f.z) * w1_m1f.w;
  m2h *= fnoise1(v.zx*w1_m2f.x) *w1_m2f.y + fnoise1(v.yx*w1_m2f.z) * w1_m2f.w;

  float sum = m0h + m1h + m2h;

  float x = fnoise1(v.yx) * sum;

  float h01 = m0h;
  float h12 = m0h + m1h;

  vec3 d = mix(w1_m0d, w1_m1d, smoothstep(h01, h01, x));
  d = mix(d, w1_m2d, smoothstep(h12, h12, x));

  vec4 s = mix(w1_m0s, w1_m1s, smoothstep(h01, h01, x));
  s = mix(s, w1_m2s, smoothstep(h12, h12, x));

  mat_diffuse = d;
  mat_specular = s;
}

void W2_m(vec3 o) {
  vec3 v = o - w2_pos;
  float H = height(o,w2_pos);

  float m0h = smoothstep(w2_m0h.x,w2_m0h.y,H) * (1.-smoothstep(w2_m0h.z,w2_m0h.w,H));
  float m1h = smoothstep(w2_m1h.x,w2_m1h.y,H) * (1.-smoothstep(w2_m1h.z,w2_m1h.w,H));
  float m2h = smoothstep(w2_m2h.x,w2_m2h.y,H) * (1.-smoothstep(w2_m2h.z,w2_m2h.w,H));

  m1h *= fnoise1(v.zx*w2_m1f.x) *w2_m1f.y + fnoise1(v.yx*w2_m1f.z) * w2_m1f.w;
  m2h *= fnoise1(v.zx*w2_m2f.x) *w2_m2f.y + fnoise1(v.yx*w2_m2f.z) * w2_m2f.w;

  float sum = m0h + m1h + m2h;

  float x = fnoise1(v.yx) * sum;

  float h01 = m0h;
  float h12 = m0h + m1h;

  vec3 d = mix(w2_m0d, w2_m1d, smoothstep(h01, h01, x));
  d = mix(d, w2_m2d, smoothstep(h12, h12, x));

  vec4 s = mix(w2_m0s, w2_m1s, smoothstep(h01, h01, x));
  s = mix(s, w2_m2s, smoothstep(h12, h12, x));

  mat_diffuse = d;
  mat_specular = s;
}

void M(vec3 o) {
  if (W1(o) < W2(o))
    W1_m(o);
  else
    W2_m(o);
}

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
    float LL = T(O+D*L,D,FAR);
    if (LL < FAR)
      L += LL;
    else
      L = FAR;
  }

  if (L < FAR) {
    vec3 P = O + D * L;
    M(P);
    color = enlight_dir(D,P,N(P),mat_diffuse,normalize(vec3(1.)),vec3(1.),vec3(0.),10.);
  }

  gl_FragColor=vec4(pow(color,vec3(.7)),0.);
}


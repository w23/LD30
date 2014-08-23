#include <stddef.h> /* offsetof */

#include "kapusha/simpleton.h"
#include "kapusha/render.h"
#include "kapusha/ooo.h"

static KPrender_batch_o batch;
static KPrender_program_o program;

static const char shader_vertex[] =
"uniform mat4 um4_mvp;\n"
"attribute vec3 av3_vertex;\n"
"attribute vec3 av3_color;\n"
"varying vec2 vv2_tc, vv2_tc2;\n"
"void main() {\n"
"  gl_Position = um4_mvp * vec4(av3_vertex, 1.);\n"
"  vv2_tc = av3_color.xy * 1.6 - vec2(.8);\n"
"  vv2_tc2 = av3_color.zx;\n"
"}\n"
;

static const char shader_fragment[] =
"uniform sampler2D us2_tex;\n"
"uniform sampler2D us2_noise;\n"
"varying vec2 vv2_tc, vv2_tc2;\n"
"void main() {\n"
"  gl_FragColor = .5*(texture2D(us2_tex,vv2_tc)+texture2D(us2_noise,vv2_tc2));\n"
"}\n"
;

typedef struct {
  KPvec3f vertex;
  KPvec4b color;
} vertex_t;

static vertex_t vertices[8] = {
  {{ .5f, -.5f,  .5f}, {255,   0, 255, 0}},
  {{ .5f,  .5f,  .5f}, {255, 255, 255, 0}},
  {{-.5f, -.5f,  .5f}, {  0,   0, 255, 0}},
  {{-.5f,  .5f,  .5f}, {  0, 255, 255, 0}},
  {{-.5f, -.5f, -.5f}, {  0,   0,   0, 0}},
  {{-.5f,  .5f, -.5f}, {  0, 255,   0, 0}},
  {{ .5f, -.5f, -.5f}, {255,   0,   0, 0}},
  {{ .5f,  .5f, -.5f}, {255, 255,   0, 0}}
};

static KPu16 indices[36] = {
  0, 1, 3, 0, 3, 2,
  2, 3, 5, 2, 5, 4,
  4, 5, 7, 4, 7, 6,
  6, 7, 1, 6, 1, 0,
  1, 7, 5, 1, 5, 3,
  0, 2, 4, 0, 4, 6
};

enum {
  // Knuth MMIX constants
  // taken from http://en.wikipedia.org/wiki/Linear_congruential_generator
  rand_lcg32_increment = 1442695040888963407ull,
  rand_lcg32_multiplier = 6364136223846793005ull
};

static KPu64 prngstate = 31337;
static KPu32 rand() {
  prngstate = prngstate * rand_lcg32_multiplier + rand_lcg32_increment;
  return (KPu32)(prngstate >> 16);
}

KPrender_sampler_o rndtex;

KPrender_program_env_o env;
KPrender_cmd_fill_t fill;
KPrender_cmd_rasterize_t raster;

struct {
  KPreframe_t frame;
  KPmat4f proj;
  KPvec3f move;
} player;


void game_init(int argc, const char *argv[]) {
  KP_L("init");
  KPrender_buffer_o buffer = kpRenderBufferCreate();
  KPblob_desc_t data;
  data.data = vertices;
  data.size = sizeof(vertices);
  kpRenderBufferUpload(buffer, KPRenderBufferFlagNone, data);

  batch = kpRenderBatchCreate();
  KPrender_vertex_attrib_t attrib;
  attrib.buffer = buffer;
  attrib.components = 3;
  attrib.offset = offsetof(vertex_t, vertex);
  attrib.stride = sizeof(vertex_t);
  attrib.type = KPRenderVertexAttribF32;
  kpRenderBatchAttribSet(batch, kpRenderTag("VRTX"), &attrib);
  kpRelease(buffer);

  attrib.buffer = buffer;
  attrib.components = 3;
  attrib.offset = offsetof(vertex_t, color);
  attrib.stride = sizeof(vertex_t);
  attrib.type = KPRenderVertexAttribU8;
  kpRenderBatchAttribSet(batch, kpRenderTag("COLR"), &attrib);

  buffer = kpRenderBufferCreate();
  data.data = indices;
  data.size = sizeof(indices);
  kpRenderBufferUpload(buffer, KPRenderBufferFlagElement, data);

  KPrender_draw_params_t draw;
  draw.buffer = buffer;
  draw.index_type = KPRenderDrawIndexU16;
  draw.count = 36;
  draw.offset = 0;
  draw.primitive = KPRenderDrawPrimitiveTriangleList;
  kpRenderBatchDrawSet(batch, &draw);
  kpRelease(buffer);

  program = kpRenderProgramCreate();

  data.data = shader_vertex;
  data.size = sizeof(shader_vertex);
  kpRenderProgramModuleSet(program, KPRenderProgramModuleVertex, data);

  data.data = shader_fragment;
  data.size = sizeof(shader_fragment);
  kpRenderProgramModuleSet(program, KPRenderProgramModuleFragment, data);

  kpRenderProgramAttributeTag(program, "av3_vertex", kpRenderTag("VRTX"));
  kpRenderProgramAttributeTag(program, "av3_color", kpRenderTag("COLR"));

  kpRenderProgramArgumentTag(program, "um4_mvp", kpRenderTag("MMVP"));
  kpRenderProgramArgumentTag(program, "us2_tex", kpRenderTag("STEX"));
  kpRenderProgramArgumentTag(program, "us2_noise", kpRenderTag("NOIZ"));

  env = kpRenderProgramEnvCreate();

  fill.header.cmd = KPrender_Command_Fill;
  fill.color = kpVec4f(0, .5, 0, 0);

  raster.header.cmd = KPrender_Command_Rasterize;
  raster.batch = batch;
  raster.program = program;
  raster.env_count = 1;
  raster.env = &env;

  KPsurface_o surface = kpSurfaceCreate(256, 256, KPSurfaceFormatU8RGBA);
  KPu32 *pix = (KPu32*)surface->buffer;
  int x, y;
  for (y = 0; y < surface->height; ++y)
    for (x = 0; x < surface->width; ++x, ++pix) {
      KPu8 r = x + y;
      KPu8 g = x ^ y;
      KPu8 b = x * y;
      *pix = (0xff000000) | (b << 16) | (g << 8) | r;
    }

  KPrender_sampler_o sampler = kpRenderSamplerCreate();
  kpRenderSamplerUpload(sampler, surface);
  kpRelease(surface);

  surface = kpSurfaceCreate(1024, 1024, KPSurfaceFormatU8RGBA);
  int i;
  pix = (KPu32*)surface->buffer;
  for (i = 0; i < surface->width * surface->height; ++i, ++pix)
    *pix = rand();
  rndtex = kpRenderSamplerCreate();
  kpRenderSamplerUpload(rndtex, surface);
  kpRelease(surface);

  kpRenderProgramEnvSetSampler(env, kpRenderTag("STEX"), sampler);
  kpRelease(sampler);
  kpRenderProgramEnvSetSampler(env, kpRenderTag("NOIZ"), rndtex);

  kpReframeMakeIdentity(&player.frame);
}

void game_resize(int width, int height) {
  KPrender_destination_t dest;
  kpRenderDestinationDefaults(&dest);
  dest.viewport.tr.x = width;
  dest.viewport.tr.y = height;
  kpRenderSetDestination(&dest);

  player.proj = kpMat4fProjPerspective(1.f, 100.f, (KPf32)width/(KPf32)height, 90.f);
}

void game_update(KPtime_ms pts) {
  kpRenderExecuteCommand(&fill.header);

  static KPtime_ms lastpts;
  KPf32 dt;
  if (lastpts == 0) {
    dt = 0;
  } else {
    dt = (pts - lastpts) / 1000.f;
  }
  lastpts = pts;

  kpReframeTranslate(&player.frame, kpVec3fMulf(player.move, dt));

  int x,y,z;
  for (z = 0; z < 16; ++z) for(y = 0; y < 16; ++y) for(x = 0; x < 16; ++x) {
    KPvec3f offset = kpVec3f(x-8.f,y-8.f,z-8.f);
    KPdquatf q = kpDquatfRotationTranslation(
      kpVec3fNormalize(kpVec3f(x, y, z)), pts / 1000.f, kpVec3fMulf(offset, 3.f));
    KPmat4f m = kpMat4fMulm4(player.proj, kpMat4fdq(kpDquatfMuldq(player.frame.dq,q)));

    kpRenderProgramEnvSetMat4f(env, kpRenderTag("MMVP"), &m);
    kpRenderExecuteCommand(&raster.header);
  }
}

void game_mouse(int dx, int dy) {
  kpReframeRotateAroundSelfY(&player.frame, (KPf32)dx * .003f);
  kpReframeRotateAroundSelfX(&player.frame, (KPf32)dy * .003f);
  kpReframeSyncMatrix(&player.frame);
}

void game_key(int code, int down) {
  float d = down ? 1 : -1;
  d *= 4.f;
  switch (code) {
    case 1: player.move.z += d; break;
    case 2: player.move.z += -d; break;
    case 3: player.move.x += d; break;
    case 4: player.move.x += -d; break;
  }
}

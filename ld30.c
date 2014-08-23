#include <stddef.h> /* offsetof */

#include "kapusha/simpleton.h"
#include "kapusha/render.h"
#include "kapusha/ooo.h"

static KPrender_batch_o batch;

typedef struct {
  KPvec2f vertex;
} vertex_t;

static vertex_t vertices[4] = {{{-1,1}},{{-1,-1}},{{1,1}},{{1,-1}}};

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

KPrender_sampler_o noises;

KPrender_program_env_o env;
KPrender_cmd_fill_t fill;
KPrender_cmd_rasterize_t raster;

KPrender_cmd_rasterize_t paint;

typedef void *live_program;
extern live_program live_program_new(const char *vs, const char *fs,
  const char *attribs, const char *args);
extern KPrender_program_o live_program_current(live_program prg);

live_program painter;

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
  attrib.components = 2;
  attrib.offset = offsetof(vertex_t, vertex);
  attrib.stride = sizeof(vertex_t);
  attrib.type = KPRenderVertexAttribF32;
  kpRenderBatchAttribSet(batch, kpRenderTag("VRTX"), &attrib);
  kpRelease(buffer);

  KPrender_draw_params_t draw;
  draw.buffer = 0;
 // draw.index_type = KPRenderDrawIndexU16;
  draw.count = 4;
  draw.offset = 0;
  draw.primitive = KPRenderDrawPrimitiveTriangleStrip;
  kpRenderBatchDrawSet(batch, &draw);
  
  KPsurface_o surface = kpSurfaceCreate(1024, 1024, KPSurfaceFormatU8RGBA);
  int i;
  KPu32 *pix = (KPu32*)surface->buffer;
  for (i = 0; i < surface->width * surface->height; ++i, ++pix)
    *pix = rand();
  noises = kpRenderSamplerCreate();
  kpRenderSamplerUpload(noises, surface);
  kpRelease(surface);

  env = kpRenderProgramEnvCreate();
  kpRenderProgramEnvSetSampler(env, kpRenderTag("NOIZ"), noises);

  kpReframeMakeIdentity(&player.frame);

  fill.header.cmd = KPrender_Command_Fill;
  fill.color = kpVec4f(0, 0, 0, 0);

  raster.header.cmd = KPrender_Command_Rasterize;
  raster.batch = batch;
  raster.env_count = 1;
  raster.env = &env;

  painter = live_program_new("painter.vs", "painter.fs",
    "VRTX=v;", "NOIZ=us2_noise;MVIE=um4_view;SRES=uv2_res;");
}

void game_resize(int width, int height) {
  KPrender_destination_t dest;
  kpRenderDestinationDefaults(&dest);
  dest.viewport.tr.x = width;
  dest.viewport.tr.y = height;
  kpRenderSetDestination(&dest);

  player.proj = kpMat4fProjPerspective(1.f, 100.f, (KPf32)width/(KPf32)height, 90.f);
  //kpRenderProgramEnvSetVec2f(env,kpRenderTag("SRES"), kpVec2f(width, height));
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
  kpReframeSyncMatrix(&player.frame);

  kpRenderProgramEnvSetMat4f(env, kpRenderTag("MVIE"), &player.frame.mat);

  raster.program = live_program_current(painter);
  kpRenderExecuteCommand(&raster.header);
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

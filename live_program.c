#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> // ::close()
#include <stdio.h> // FILENAME_MAX
#include <errno.h>
#include <time.h>
#include <string.h>

#include <kapusha/render.h>

typedef struct {
  KPrender_program_o current;
  char *filevs, *filefs;
  char *attribs, *args;
  time_t timevs, timefs;
  char *vs, *fs;
} live_program_t;

typedef live_program_t *live_program;

live_program live_program_new(const char *vs, const char *fs,
  const char *attribs, const char *args)
{
  KPsize vslen = strlen(vs) + 1;
  KPsize fslen = strlen(fs) + 1;
  KPsize attribslen = strlen(attribs) + 1;
  KPsize argslen = strlen(args) + 1;
  KPsize header = sizeof(live_program_t);
  KPsize size = header  + vslen + fslen + attribslen + argslen;

  live_program_t *this = kpAlloc(size);
  this->filevs = ((char*)this) + header;
  this->filefs = ((char*)this) + header + vslen;
  this->attribs = ((char*)this) + header + vslen + fslen;
  this->args = ((char*)this) + header + vslen + fslen + attribslen;

  memcpy(this->filevs, vs, vslen);
  memcpy(this->filefs, fs, fslen);
  memcpy(this->attribs, attribs, attribslen);
  memcpy(this->args, args, argslen);

  this->vs = this->fs = 0;
  this->timevs = this->timefs = 0;
  this->current = 0;
  return this;
}

KPrender_program_o live_program_current(live_program this) {
  KPrender_program_o newprog = 0;
  struct stat st;

  stat(this->filevs, &st);
  if (st.st_mtime != this->timevs) {
    FILE *f = 0;
    while (f == 0) f = fopen(this->filevs, "r");
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    rewind(f);
    if (this->vs) kpFree(this->vs);
    this->vs = kpAlloc(len + 1);
    len = fread(this->vs, 1, len + 1, f);
    fclose(f);
    this->vs[len] = 0;

    newprog = kpRenderProgramCreate();
    KPblob_desc_t data;
    data.data = this->vs;
    data.size = len;
    kpRenderProgramModuleSet(newprog, KPRenderProgramModuleVertex, data);
    this->timevs = st.st_mtime;
  }

  stat(this->filefs, &st);
  if (st.st_mtime != this->timefs) {
    FILE *f = 0;
    while (f == 0) f = fopen(this->filefs, "r");
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    rewind(f);
    if (this->fs) kpFree(this->fs);
    this->fs = kpAlloc(len + 1);
    len = fread(this->fs, 1, len + 1, f);
    fclose(f);
    this->fs[len] = 0;

    if (!newprog) {
      newprog = kpRenderProgramCreate();
      KPblob_desc_t data;
      data.data = this->vs;
      data.size = strlen(this->vs);
      kpRenderProgramModuleSet(newprog, KPRenderProgramModuleVertex, data);
    }

    KPblob_desc_t data;
    data.data = this->fs;
    data.size = len;
    kpRenderProgramModuleSet(newprog, KPRenderProgramModuleFragment, data);
    this->timefs = st.st_mtime;
  } else if (newprog) {
    KPblob_desc_t data;
    data.data = this->fs;
    data.size = strlen(this->fs);
    kpRenderProgramModuleSet(newprog, KPRenderProgramModuleFragment, data);
  }

  if (newprog) {
    kpRelease(this->current);
    this->current = newprog;

    KPrender_tag_t tag;
    char buf[32];
    const char *s = this->attribs;
    while (*s != 0) {
      tag.tag.name[0] = s[0];
      tag.tag.name[1] = s[1];
      tag.tag.name[2] = s[2];
      tag.tag.name[3] = s[3];
      s += 5;
      const char *e = strchr(s, ';');
      memcpy(buf, s, e-s);
      buf[e-s] = 0;
      s = e + 1;
      kpRenderProgramAttributeTag(this->current, buf, tag);
    }
    
    s = this->args;
    while (*s != 0) {
      tag.tag.name[0] = s[0];
      tag.tag.name[1] = s[1];
      tag.tag.name[2] = s[2];
      tag.tag.name[3] = s[3];
      s += 5;
      const char *e = strchr(s, ';');
      memcpy(buf, s, e-s);
      buf[e-s] = 0;
      s = e + 1;
      kpRenderProgramArgumentTag(this->current, buf, tag);
    }
  }

  return this->current;
}

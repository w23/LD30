#include <SDL2/SDL.h>

extern void game_init(int, char**);
extern void game_resize(int w, int h);
extern void game_update(uint32_t time);
extern void game_key(int code, int down);
extern void game_mouse(int dx, int dy);

int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  SDL_Window *window = SDL_CreateWindow("ld30",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720,
    SDL_WINDOW_OPENGL);

  //| SDL_WINDOW_RESIZABLE);

//    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_OPENGL);

  SDL_GLContext *ctx = SDL_GL_CreateContext(window);

  int w, h;
  SDL_GetWindowSize(window, &w, &h);

  game_init(argc, argv);
  game_resize(w, h);

  int loop = 1;
  int grabbed = 0;
  while (loop == 1) {
    SDL_Event evt;
    while (SDL_PollEvent(&evt) != 0) {
      int keydown = 0;
      switch (evt.type) {
        case SDL_WINDOWEVENT_RESIZED:
          game_resize(evt.window.data1, evt.window.data2);
          break;
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
          loop = 0;
          break;
        case SDL_MOUSEBUTTONDOWN:
            if (!grabbed) {
              SDL_SetRelativeMouseMode(SDL_TRUE);
              SDL_SetWindowGrab(window, SDL_TRUE);
              grabbed = 1;
            }
          break;
        case SDL_MOUSEMOTION:
          if (grabbed)
            game_mouse(evt.motion.xrel, evt.motion.yrel);
          break;
        case SDL_KEYDOWN:
          if (evt.key.repeat)
            break;
          keydown = 1;
        case SDL_KEYUP:
          switch(evt.key.keysym.sym) {
            case SDLK_ESCAPE:
              if (!keydown) break;
              if (grabbed) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_SetWindowGrab(window, SDL_FALSE);
                grabbed = 0;
              } else {
                loop = 0;
              }
              break;
            case SDLK_w:
              game_key(1,keydown);
              break;
            case SDLK_s:
              game_key(2,keydown);
              break;
            case SDLK_a:
              game_key(3,keydown);
              break;
            case SDLK_d:
              game_key(4,keydown);
              break;
          }

          break;
        //default: printf("evt %d\n", evt.type);
      }
    }
    game_update(SDL_GetTicks());
    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(ctx);
  SDL_Quit();

  return 0;
}

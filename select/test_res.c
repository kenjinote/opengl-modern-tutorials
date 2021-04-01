/**
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */
#include "SDL.h"
#include "res_texture.c"
int main(int argc, char* argv[]) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Surface* screen = SDL_SetVideoMode(640, 480, 0, SDL_DOUBLEBUF);
  SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)res_texture.pixel_data,
                                               res_texture.width, res_texture.height, res_texture.bytes_per_pixel * 8,
                                               res_texture.width * res_texture.bytes_per_pixel, // pitch
					       0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
  if (surf == NULL) exit(1);
  SDL_FillRect(screen, NULL, (Uint32)-1);

  SDL_BlitSurface(surf, NULL, screen, NULL);
  SDL_Rect dst = {300,100,0,0};
  SDL_BlitSurface(surf, NULL, screen, &dst);
  SDL_Flip(screen);

  SDL_Event e;
  while (SDL_WaitEvent(&e)) {
    if (e.type == SDL_QUIT)
      break;
  }
  return 0;
}
/**
 * Local Variables:
 * compile-command: "gcc -Wall -g $(pkg-config sdl --cflags) test_res.c -o test_res $(pkg-config sdl --libs) && ./test_res"
 * End:
 */

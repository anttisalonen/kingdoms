#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include "SDL/SDL.h"
#include "SDL/SDL_endian.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    const Uint32 rmask = 0xff000000;
    const Uint32 gmask = 0x00ff0000;
    const Uint32 bmask = 0x0000ff00;
    const Uint32 amask = 0x000000ff;
#else
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif

/* doesn't update the screen. lock must be held.
 * snippet from: http://www.libsdl.org/intro.en/usingvideo.html */
void sdl_put_pixel(SDL_Surface* screen, int x, int y, const color& c);
color sdl_get_pixel(SDL_Surface* screen, int x, int y);
void sdl_change_pixel_color(SDL_Surface* screen, const color& src, const color& dst);
SDL_Surface* sdl_load_image(const char* filename);
int draw_text(SDL_Surface* screen, const TTF_Font* font, const char* str, int x, int y, Uint8 r, Uint8 g, Uint8 b);
int draw_line(SDL_Surface* screen, int start_x, int start_y, int end_x, int end_y, const color& col);

#endif

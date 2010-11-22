#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_ttf.h"

#include "color.h"

/* doesn't update the screen. lock must be held.
 * snippet from: http://www.libsdl.org/intro.en/usingvideo.html */
void sdl_put_pixel(SDL_Surface* screen, int x, int y, const color& c);
color sdl_get_pixel(SDL_Surface* screen, int x, int y);
void sdl_change_pixel_color(SDL_Surface* screen, const color& src, const color& dst);
SDL_Surface* sdl_load_image(const char* filename);
int draw_text(SDL_Surface* screen, const TTF_Font* font, const char* str, int x, int y, int r, int g, int b);

#endif

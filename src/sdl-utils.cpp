#include "sdl-utils.h"

#include <algorithm>

void sdl_put_pixel(SDL_Surface* screen, int x, int y, const color& c)
{
	Uint32 color = SDL_MapRGB(screen->format, c.r, c.g, c.b);

	if(x < 0 || y < 0 || screen->w <= x || screen->h <= y)
		return;
	switch (screen->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
				Uint8 *bufp;

				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*bufp = color;
			}
			break;

		case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *bufp;

				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				*bufp = color;
			}
			break;

		case 3: { /* Slow 24-bpp mode, usually not used */
				Uint8 *bufp;

				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				*(bufp+screen->format->Rshift/8) = c.r;
				*(bufp+screen->format->Gshift/8) = c.g;
				*(bufp+screen->format->Bshift/8) = c.b;
			}
			break;

		case 4: { /* Probably 32-bpp */
				Uint32 *bufp;

				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				*bufp = color;
			}
			break;
	}
}

color sdl_get_pixel(SDL_Surface* screen, int x, int y)
{
	Uint8 r, g, b;
	Uint32 pixel = 0;
	switch (screen->format->BytesPerPixel) {
		case 1: { /* Assuming 8-bpp */
				Uint8 *bufp;
				bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
				pixel = *bufp;
			}
			break;

		case 2: { /* Probably 15-bpp or 16-bpp */
				Uint16 *bufp;
				bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
				pixel = *bufp;
			}
			break;

		case 4: { /* Probably 32-bpp */
				Uint32 *bufp;
				bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
				pixel = *bufp;
			}
			break;
	}
	SDL_GetRGB(pixel, screen->format, &r, &g, &b);
	return color(r, g, b);
}

void sdl_change_pixel_color(SDL_Surface* screen, const color& src, const color& dst)
{
	if(screen->format->BytesPerPixel != 4)
		return;
	Uint32 src_color = SDL_MapRGB(screen->format, src.r, src.g, src.b);
	Uint32 dst_color = SDL_MapRGB(screen->format, dst.r, dst.g, dst.b);
	for(int i = 0; i < screen->h; i++) {
		for(int j = 0; j < screen->w; j++) {
			Uint32 *bufp = (Uint32*)screen->pixels + i * screen->pitch / 4 + j;
			if(*bufp == src_color)
				*bufp = dst_color;
		}
	}
}

SDL_Surface* sdl_load_image(const char* filename)
{
	SDL_Surface* img = IMG_Load(filename);
	if(!img) {
		fprintf(stderr, "Unable to load image '%s': %s\n", filename,
				IMG_GetError());
		return NULL;
	}
	else {
		SDL_Surface* opt = SDL_DisplayFormatAlpha(img);
		SDL_FreeSurface(img);
		if(!opt) {
			fprintf(stderr, "Unable to convert image '%s': %s\n",
					filename, SDL_GetError());
			return NULL;
		}
		else {
			return opt;
		}
	}
}

int draw_text(SDL_Surface* screen, const TTF_Font* font, const char* str, 
		int x, int y, 
		Uint8 r, Uint8 g, Uint8 b, bool centered)
{
	if(!str)
		return 0;
	SDL_Surface* text;
	SDL_Color color = {r, g, b};
	text = TTF_RenderUTF8_Blended((TTF_Font*)font, str, color);
	if(!text) {
		fprintf(stderr, "Could not render text: %s\n",
				TTF_GetError());
		return 1;
	}
	else {
		SDL_Rect dest;
		if(!centered)
			dest.x = x;
		else
			dest.x = x - text->w / 2;
		dest.y = y;
		SDL_BlitSurface(text, NULL, screen, &dest);
		SDL_FreeSurface(text);
		return 0;
	}
}

/* Bresenham's line algorithm */
int draw_line(SDL_Surface* screen, int x0, int y0, int x1, int y1, const color& col)
{
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if(steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if(x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int d_x = x1 - x0;
	int d_y = abs(y1 - y0);
	int error = d_x / 2;
	int ystep;
	int y = y0;
	if(y0 < y1)
		ystep = 1;
	else
		ystep = -1;
	for(int x = x0; x <= x1; x++) {
		if(steep)
			sdl_put_pixel(screen, y, x, col);
		else
			sdl_put_pixel(screen, x, y, col);
		error -= d_y;
		if(error < 0) {
			y += ystep;
			error += d_x;
		}
	}
	return 0;
}

int draw_plain_rectangle(SDL_Surface* screen, int x, int y, int w, int h, const color& col)
{
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	Uint32 bgcol = SDL_MapRGB(screen->format, col.r, col.g, col.b);
	SDL_FillRect(screen, &rect, bgcol);
	return 0;
}


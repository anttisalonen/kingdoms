#include <stdlib.h>

#include "SDL/SDL.h"

int main(int argc, char **argv)
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_Quit();
	return 0;
}



#include <stdio.h>
#include "sdl-utils.h"
#include "pompelmous.h"
#include "parse_rules.h"
#include "serialize.h"

void usage(char* pn)
{
	fprintf(stderr, "Usage: %s [-r <ruleset name>] <input filename> <output filename>\n\n", pn);
	fprintf(stderr, "\t-r ruleset:  use custom ruleset\n");
	fprintf(stderr, "\t-x:          disable wrapping the X coordinate\n");
	fprintf(stderr, "\tInput file:  an image file (e.g. png)\n");
	fprintf(stderr, "\tOutput file: map filename (without a file extension)\n");
}

static std::string ruleset_name = "default";
static const char* infile = NULL;
static const char* outfile = NULL;
static bool wrap_x = true;

void convert_image()
{
	resource_configuration resconf;
	resource_map rmap;
	unit_configuration_map uconfmap;
	advance_map amap;
	city_improv_map cimap;
	government_map govmap;
	std::vector<civilization*> civs;
	get_configuration(ruleset_name, &civs, &uconfmap, &amap, &cimap,
			&resconf, &govmap, &rmap);

	SDL_Surface* surf = IMG_Load(infile);
	if(!surf) {
		throw std::runtime_error("Could not load image");
	}
	int w = surf->w;
	int h = surf->h;
	printf("Map size: %dx%d\n", w, h);
	printf("Bytes per pixel: %d\n", surf->format->BytesPerPixel);
	int grass = resconf.get_grass_tile();
	int mountain = resconf.get_mountain_tile();
	int sea = resconf.get_sea_tile();
	map m(w, h, resconf, rmap);
	m.set_x_wrap(wrap_x);
	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			color c = sdl_get_pixel(surf, i, j);
			printf("Color %3d %3d %3d => ", c.r, c.g, c.b);
			if(c.r > c.g && c.r > c.b) {
				m.set_data(i, j, mountain);
				printf("Mountain\n");
			}
			else if(c.g > c.r && c.g > c.b) {
				m.set_data(i, j, grass);
				printf("Grass\n");
			}
			else {
				printf("Ocean\n");
			}
		}
	}

	// turn coasts to sea
	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			if(!m.resconf.is_water_tile(m.get_data(i, j))) {
				for(int k = -1; k <= 1; k++) {
					for(int l = -1; l <= 1; l++) {
						int neighbour = m.get_data(i + k, j + l);
						if(m.resconf.is_ocean_tile(neighbour)) {
							m.set_data(i + k, j + l, sea);
						}
					}
				}
			}
		}
	}
	
	save_map(outfile, ruleset_name, m);
}

int main(int argc, char** argv)
{
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			exit(2);
		}
		else if(!strcmp(argv[i], "-r")) {
			if(argc >= ++i) {
				ruleset_name = std::string(argv[i]);
			}
			else {
				fprintf(stderr, "Error: no parameter to -r.\n");
				usage(argv[0]);
				exit(2);
			}
		}
		else if(!strcmp(argv[i], "-x")) {
			wrap_x = false;
		}
		else {
			if(!infile)
				infile = argv[i];
			else if(!outfile)
				outfile = argv[i];
			else {
				fprintf(stderr, "Unknown option '%s'.\n", argv[i]);
				usage(argv[0]);
				exit(2);
			}
		}
	}
	if(!infile || !outfile) {
		usage(argv[0]);
		exit(1);
	}
	if(sdl_init_all())
		exit(1);
	try {
		convert_image();
	}
	catch (boost::archive::archive_exception& e) {
		printf("boost::archive::archive_exception: %s (code %d).\n",
				e.what(), e.code);
	}
	catch (std::exception& e) {
		printf("std::exception: %s\n", e.what());
	}
	catch(...) {
		printf("Unknown exception.\n");
	}
	TTF_Quit();
	SDL_Quit();

	return 0;
}


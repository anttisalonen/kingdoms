#include <stdio.h>
#include <string.h>
#include <fstream>

#include "sdl-utils.h"
#include "pompelmous.h"
#include "parse_rules.h"
#include "serialize.h"

#include <jsoncpp/json/json.h>


void usage(char* pn)
{
	fprintf(stderr, "Usage: %s [-r <ruleset name>] <color mapping file> <input image file> <output file>\n\n", pn);
	fprintf(stderr, "\t-r ruleset:         use custom ruleset\n");
	fprintf(stderr, "\t-x:                 disable wrapping the X coordinate\n");
	fprintf(stderr, "\tColor mapping file: mapping from color to terrain (see doc/mapping_example.json)\n");
	fprintf(stderr, "\tInput image file:   an image file (e.g. png)\n");
	fprintf(stderr, "\tOutput file:        map filename (without a file extension)\n");
}

static std::string ruleset_name = "default";
static const char* color_mapping_file = NULL;
static const char* infile = NULL;
static const char* outfile = NULL;
static bool wrap_x = true;


struct colormap {
	colormap(const color& c, int t) : col(c), type(t) { }
	color col;
	int type;
};

std::vector<colormap> loadColorMapping(const resource_configuration& resconf)
{
	Json::Reader reader;
	Json::Value root;

	std::ifstream input(color_mapping_file, std::ifstream::binary);
	bool parsingSuccessful = reader.parse(input, root, false);
	if (!parsingSuccessful) {
		throw std::runtime_error(reader.getFormatedErrorMessages());
	}

	std::vector<colormap> ret;

	auto members = root.getMemberNames();

	for(const auto& membername : members) {
		const auto& member = root[membername];
		std::vector<color> colors;
		if(!member[0u].isArray()) {
			int r = member[0u].asInt();
			int g = member[1u].asInt();
			int b = member[2u].asInt();
			colors.push_back(color(r, g, b));
		} else {
			for(const auto& col : member) {
				int r = col[0u].asInt();
				int g = col[1u].asInt();
				int b = col[2u].asInt();
				colors.push_back(color(r, g, b));
			}
		}

		bool found = false;
		for(size_t i = 0; i < num_terrain_types; i++) {
			if(resconf.resource_name[i] == membername) {
				for(const auto& col : colors) {
					ret.push_back(colormap(col, i));
				}
				found = true;
				break;
			}
		}

		if(!found) {
			printf("Warning: invalid terrain name \"%s\"\n", membername.c_str());
		}
	}

	return ret;
}


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
	int sea = resconf.get_sea_tile();

	std::vector<colormap> colormapping;
	colormapping = loadColorMapping(resconf);

	int count[num_terrain_types];
	memset(count, 0x00, sizeof(count));

	map m(w, h, resconf, rmap);
	m.set_x_wrap(wrap_x);
	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			color c = sdl_get_pixel(surf, i, j);
			int mindiff = INT_MAX;
			int type = -1;
			for(const auto& cm : colormapping) {
				int diff = abs(c.r - cm.col.r) +
					abs(c.g - cm.col.g) +
					abs(c.b - cm.col.b);
				if(diff < mindiff) {
					type = cm.type;
					mindiff = diff;
				}
			}
			m.set_data(i, j, type);
			count[type]++;
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

	// add random resources
	m.add_random_resources();

	save_map(outfile, ruleset_name, m);

	int total_land = 0;
	int total_tiles = w * h;
	for(unsigned int i = 0; i < num_terrain_types; i++) {
		if(!resconf.is_water_tile(i)) {
			total_land += count[i];
		}
	}

	printf("%-20s: %3d %%\n", "Land", 100 * total_land / total_tiles);
	for(unsigned int i = 0; i < num_terrain_types; i++) {
		if(count[i] && !resconf.is_water_tile(i)) {
			printf("%-20s: %3d %%\n", resconf.resource_name[i].c_str(),
					100 * count[i] / total_land);
		}
	}
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
			else if(!color_mapping_file)
				color_mapping_file = argv[i];
			else if(!outfile)
				outfile = argv[i];
			else {
				fprintf(stderr, "Unknown option '%s'.\n", argv[i]);
				usage(argv[0]);
				exit(2);
			}
		}
	}
	if(!infile || !outfile || !color_mapping_file) {
		usage(argv[0]);
		exit(1);
	}

	bool succ = false;

	if(sdl_init_all())
		exit(1);
	try {
		convert_image();
		succ = true;
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

	return succ ? 0 : 1;
}


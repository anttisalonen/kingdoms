#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include "pompelmous.h"

#define SAVE_FILE_EXTENSION	".game"
#define MAP_FILE_EXTENSION	".map"

std::string path_to_saved_games(const std::string& ruleset_name);
std::string path_to_saved_maps(const std::string& ruleset_name);
int save_game(const char* save_suffix, const std::string& ruleset_name, const pompelmous& g,
		unsigned int own_civ_id);
bool load_game(const char* filename, pompelmous& g, unsigned int& own_civ_id);

int save_map(const char* filename, const std::string& ruleset_name, const map& m);
bool load_map(const char* filename, map& m);

#endif


#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include "pompelmous.h"

#define SAVE_FILE_EXTENSION	".game"
#define MAP_FILE_EXTENSION	".map"

std::string path_to_saved_games();
int save_game(const char* save_suffix, const pompelmous& g);
bool load_game(const char* filename, pompelmous& g);

int save_map(const char* filename, const map& m);
bool load_map(const char* filename, map& m);

#endif


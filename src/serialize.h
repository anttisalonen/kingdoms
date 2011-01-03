#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include "pompelmous.h"

std::string path_to_saved_games();
int save_game(const char* save_suffix, const pompelmous& g);
bool load_game(const char* filename, pompelmous& g);

#endif


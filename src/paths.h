#ifndef PATHS_H
#define PATHS_H

#include <string>

std::string get_rules_path(const std::string& ruleset_name);
std::string get_preinstalled_maps_path(const std::string& ruleset_name);
std::string get_graphics_path(const std::string& file_basename);

#endif


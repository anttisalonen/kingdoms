#include <vector>
#include <string>

#include "civ.h"

std::vector<civilization*> parse_civs_config(const std::string& fp);
unit_configuration_map parse_unit_config(const std::string& fp);
advance_map parse_advance_config(const std::string& fp);
city_improv_map parse_city_improv_config(const std::string& fp);
resource_configuration parse_terrain_config(const std::string& fp);
government_map parse_government_config(const std::string& fp);
resource_map parse_resource_config(const std::string& fp);


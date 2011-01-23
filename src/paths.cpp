#include "paths.h"
#include "config.h"

std::string get_ruleset_path(const std::string& ruleset_name,
		const std::string& type)
{
	std::string prefix = std::string(KINGDOMS_RULESETS_DIR);
	prefix += ruleset_name;
	prefix += type;
	return prefix;
}

std::string get_rules_path(const std::string& ruleset_name)
{
	return get_ruleset_path(ruleset_name,
			KINGDOMS_RULESSUBDIR);
}

std::string get_preinstalled_maps_path(const std::string& ruleset_name)
{
	return get_ruleset_path(ruleset_name,
			KINGDOMS_MAPSSUBDIR);
}


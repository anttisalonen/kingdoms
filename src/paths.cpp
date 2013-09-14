#include "paths.h"

#define KINGDOMS_RULESSUBDIR	"/rules"
#define KINGDOMS_MAPSSUBDIR	"/maps"

static const char* get_share_dir()
{
	static const char* val = NULL;
	if(!val) {
		val = getenv("KINGDOMS_SHARE_DIR");
		if(!val)
			val = "share";
	}
	return val;
}

std::string get_ruleset_path(const std::string& ruleset_name,
		const std::string& type)
{
	std::string prefix = std::string(get_share_dir());
	prefix += "/rulesets/";
	prefix += ruleset_name;
	prefix += type;
	return prefix;
}

std::string get_graphics_path(const std::string& file_basename)
{
	std::string prefix = std::string(get_share_dir());
	prefix += "/gfx/";
	prefix += file_basename;
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


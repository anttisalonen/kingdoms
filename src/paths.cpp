#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>

#include "paths.h"

#define KINGDOMS_RULESSUBDIR	"/rules"
#define KINGDOMS_MAPSSUBDIR	"/maps"

static std::string get_prog_binary_path()
{
	// http://stackoverflow.com/questions/933850/how-to-find-the-location-of-the-executable-in-c
#ifdef __WIN32__
	// not tested - probably need to include some header...
	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, ARRAYSIZE(path));
#else
	char buf[256];
	memset(buf, 0x00, sizeof(buf));
	// Linux
	ssize_t ret = readlink("/proc/self/exe", buf, 255);
	if(ret != -1) {
		return std::string(dirname(buf));
	}

	// FreeBSD
	ret = readlink("/proc/curproc/file", buf, 255);
	if(ret != -1) {
		return std::string(dirname(buf));
	}

	// End up here on Unixes without /proc.
	// No argv[0] so let's fall back to $cwd/bin.
	return std::string("bin");
#endif
}

static const char* get_share_dir()
{
	static const char* val = NULL;
	if(!val) {
		val = getenv("KINGDOMS_SHARE_DIR");
		if(!val) {
			static std::string prog_binary_path = get_prog_binary_path();
			prog_binary_path += "/../share";
			// use share/kingdoms if it exists (case after make install)
			struct stat buf;
			int ret = stat((prog_binary_path + "/kingdoms").c_str(), &buf);
			if(ret == 0 && (buf.st_mode & S_IFDIR)) {
				// found
				prog_binary_path += "/kingdoms";
			}
			val = prog_binary_path.c_str();
		}
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


#include <fstream>
#ifdef __MINGW32__
#include <io.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "serialize.h"

int create_dir_if_not_exist(const std::string& s)
{
	struct stat buf;
	if(stat(s.c_str(), &buf)) {
		if(errno == ENOENT) {
#ifdef __MINGW32__
			if(mkdir(s.c_str())) {
#else
			if(mkdir(s.c_str(), S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH)) {
#endif
				fprintf(stderr, "Could not create %s: %s\n",
						s.c_str(), strerror(errno));
				return 1;
			}
		}
		else {
			fprintf(stderr, "Could not stat %s: %s\n",
					s.c_str(), strerror(errno));
			return 1;
		}
	}
	return 0;
}

std::string path_to_saved_games()
{
#ifdef __WIN32__
	std::string s = "./saves/";
	create_dir_if_not_exist(s);
	return s;
#else
	const char* home = getenv("HOME");
	if(home) {
		std::string s(home);
		s += "/.kingdoms";
		create_dir_if_not_exist(s);
		s += "/saves/";
		create_dir_if_not_exist(s);
		return s;
	}
	return std::string("");
#endif
}

int save_game(const char* save_suffix, const pompelmous& g)
{
	char filename[256];
	time_t curr_time = time(NULL);
	struct tm* gmt;
	gmt = localtime(&curr_time);
	snprintf(filename, 256, "%s%02d-%02d-%02d_%02d-%02d-%02d-r%d-%s%s",
			path_to_saved_games().c_str(),
			gmt->tm_year % 100, gmt->tm_mon + 1, gmt->tm_mday,
			gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
			g.get_round_number(), save_suffix,
			SAVE_FILE_EXTENSION);
	std::ofstream ofs(filename);
	boost::archive::text_oarchive oa(ofs);
	oa << g;
	return 0;
}

bool load_game(const char* filename, pompelmous& g)
{
	try {
		std::ifstream ifs(filename);
		boost::archive::text_iarchive ia(ifs);
		ia >> g;
		return true;
	}
	catch(boost::archive::archive_exception& e) {
		fprintf(stderr, "Could not load game %s: %s.\n",
				filename, e.what());
		return false;
	}
}

int save_map(const char* fn, const map& m)
{
	char filename[256];
	snprintf(filename, 256, "%s%s%s",
			path_to_saved_games().c_str(),
			fn, MAP_FILE_EXTENSION);
	std::ofstream ofs(filename);
	boost::archive::text_oarchive oa(ofs);
	oa << m;
	return 0;
}

bool load_map(const char* filename, map& m)
{
	try {
		std::ifstream ifs(filename);
		boost::archive::text_iarchive ia(ifs);
		ia >> m;
		return true;
	}
	catch(boost::archive::archive_exception& e) {
		fprintf(stderr, "Could not load map %s: %s.\n",
				filename, e.what());
		return false;
	}
}


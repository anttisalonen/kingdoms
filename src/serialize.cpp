#include <fstream>
#ifdef __MINGW32__
#include <io.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
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

std::string path_to_saved(const std::string& ruleset_name,
		const std::string& save_dir)
{
#ifdef __WIN32__
	std::string s = "./saves/";
	create_dir_if_not_exist(s);
	return s;
#else
	const char* home = getenv("HOME");
	if(home) {
		std::string s(home);
		s += "/.kingdoms/";
		create_dir_if_not_exist(s);
		s += ruleset_name;
		create_dir_if_not_exist(s);
		s += "/";
		s += save_dir;
		s += "/";
		create_dir_if_not_exist(s);
		return s;
	}
	return std::string("");
#endif
}

std::string path_to_saved_games(const std::string& ruleset_name)
{
	return path_to_saved(ruleset_name, "saves");
}

std::string path_to_saved_maps(const std::string& ruleset_name)
{
	return path_to_saved(ruleset_name, "maps");
}

int save_game(const char* save_suffix, const std::string& ruleset_name,
		const pompelmous& g,
		unsigned int own_civ_id)
{
	char filename[256];
	time_t curr_time = time(NULL);
	struct tm* gmt;
	gmt = localtime(&curr_time);
	snprintf(filename, 256, "%s%02d-%02d-%02d_%02d-%02d-%02d-r%d-%s%s",
			path_to_saved_games(ruleset_name).c_str(),
			gmt->tm_year % 100, gmt->tm_mon + 1, gmt->tm_mday,
			gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
			g.get_round_number(), save_suffix,
			SAVE_FILE_EXTENSION);
	std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
	out.push(boost::iostreams::bzip2_compressor());
	out.push(ofs);
	boost::archive::binary_oarchive oa(out);
	oa << own_civ_id;
	oa << g;
	return 0;
}

bool load_game(const char* filename, pompelmous& g,
		unsigned int& own_civ_id)
{
	try {
		std::ifstream ifs(filename);
		boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
		in.push(boost::iostreams::bzip2_decompressor());
		in.push(ifs);
		boost::archive::binary_iarchive ia(in);
		ia >> own_civ_id;
		ia >> g;
		return true;
	}
	catch(std::exception& e) {
		fprintf(stderr, "Could not load game %s: %s.\n",
				filename, e.what());
		return false;
	}
}

int save_map(const char* fn, const std::string& ruleset_name, const map& m)
{
	char filename[256];
	snprintf(filename, 256, "%s%s%s",
			path_to_saved_maps(ruleset_name).c_str(),
			fn, MAP_FILE_EXTENSION);
	std::ofstream ofs(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
	out.push(boost::iostreams::bzip2_compressor());
	out.push(ofs);
	boost::archive::binary_oarchive oa(out);
	oa << m;
	return 0;
}

bool load_map(const char* filename, map& m)
{
	try {
		std::ifstream ifs(filename);
		boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
		in.push(boost::iostreams::bzip2_decompressor());
		in.push(ifs);
		boost::archive::binary_iarchive ia(in);
		ia >> m;
		return true;
	}
	catch(std::exception& e) {
		fprintf(stderr, "Could not load map %s: %s.\n",
				filename, e.what());
		return false;
	}
}


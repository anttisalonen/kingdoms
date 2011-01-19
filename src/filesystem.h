#ifndef KINGDOMS_FILESYSTEM_H
#define KINGDOMS_FILESYSTEM_H

#include <vector>
#include <string>

#define BOOST_FILESYSTEM_VERSION	3
#include <boost/filesystem.hpp>

std::vector<boost::filesystem::path> get_files_in_directory(const std::string& search_path,
		const std::string& ext);

#endif


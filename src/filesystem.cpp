#include "filesystem.h"
#include <boost/algorithm/string.hpp>

bool compare_filenames(const boost::filesystem::path& p1,
		const boost::filesystem::path& p2)
{
	return p1.filename() < p2.filename();
}

std::vector<boost::filesystem::path> get_files_in_directory(const std::string& search_path, const std::string& ext)
{
	using namespace boost::filesystem;
	path svg_path = path(search_path);
	directory_iterator end_itr;

	std::vector<path> filenames;
	if(exists(svg_path)) {
		for (directory_iterator itr(svg_path);
				itr != end_itr;
				++itr) {
			if(is_regular_file(itr->status())) {
				if(ext.empty() || 
					boost::algorithm::to_lower_copy(itr->path().extension().string()) == ext)
					filenames.push_back(itr->path());
			}
		}
		std::sort(filenames.begin(), filenames.end(), compare_filenames);
	}
	return filenames;
}



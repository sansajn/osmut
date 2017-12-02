#pragma once
#include <future>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem;

class tile
{
public:
	virtual fs::path path() = 0;  // const
};

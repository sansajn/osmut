#pragma once
#include <string>
#include <boost/filesystem/path.hpp>
#include <mapnik/map.hpp>

namespace fs = boost::filesystem;

class tile_source
{
public:
	virtual std::string get(size_t zoom, size_t x, size_t y) const = 0;
};

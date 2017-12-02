#pragma once
#include <boost/filesystem/path.hpp>
#include <mapnik/map.hpp>

namespace fs = boost::filesystem;

mapnik::Map make_map(size_t zoom);

void render_tile_to_file(mapnik::Map const & map, unsigned offset_x, unsigned offset_y,
	int width, int height, fs::path const & file);

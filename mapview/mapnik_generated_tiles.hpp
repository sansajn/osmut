#pragma once
#include <mapnik/map.hpp>
#include "tile_source.hpp"

class mapnik_generated_tiles : public tile_source
{
public:
	mapnik_generated_tiles(fs::path const & cache_dir);
	std::shared_ptr<tile> get(size_t zoom, size_t x, size_t y) override;  // const

private:
	fs::path _cache_dir;
	mapnik::Map _map;
	size_t _used_zoom;
};

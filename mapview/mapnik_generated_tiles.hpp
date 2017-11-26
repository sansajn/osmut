#pragma once
#include "tile_source.hpp"

class mapnik_generated_tiles : public tile_source
{
public:
	mapnik_generated_tiles(fs::path const & cache_dir);
	std::string get(size_t zoom, size_t x, size_t y) const override;

private:
	fs::path _cache_dir;
	mutable mapnik::Map _map;
	mutable size_t _used_zoom;
};

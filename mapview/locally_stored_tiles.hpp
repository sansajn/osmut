#pragma once
#include "tile_source.hpp"

class locally_stored_tiles : public tile_source
{
public:
	locally_stored_tiles(fs::path const & tile_dir);
	std::shared_ptr<tile> get(size_t zoom, size_t x, size_t y) override;  // const

private:
	fs::path _tile_dir;
};

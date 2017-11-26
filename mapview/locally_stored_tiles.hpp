#pragma once
#include <string>
#include <boost/filesystem/path.hpp>
#include "tile_source.hpp"

class locally_stored_tiles : public tile_source
{
public:
	locally_stored_tiles(fs::path const & tile_dir);
	std::string get(size_t zoom, size_t x, size_t y) const override;

private:
	fs::path _tile_dir;
};

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

class locally_storred_tiles : public tile_source
{
public:
	locally_storred_tiles(fs::path const & tile_dir);
	std::string get(size_t zoom, size_t x, size_t y) const override;

private:
	fs::path _tile_dir;
};

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

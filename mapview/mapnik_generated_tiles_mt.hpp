#pragma once
#include <future>
#include <mapnik/map.hpp>
#include "tile_source.hpp"
#include "sync_queue.hpp"

class mapnik_tile_mt : public tile
{
public:
	mapnik_tile_mt(fs::path const & fname);
	mapnik_tile_mt(std::future<fs::path> && promised_tile);

	fs::path path() override;  // const

private:
	fs::path _p;
	std::future<fs::path> _promised_tile;
};

//! multithread implementacia generatoru dlazdic
class mapnik_generated_tiles_mt : public tile_source
{
public:
	mapnik_generated_tiles_mt(fs::path const & cache_dir, size_t max_render_threads = 0);
	~mapnik_generated_tiles_mt();

	std::shared_ptr<tile> get(size_t zoom, size_t x, size_t y);  // const

private:
	void free_maps();
	std::shared_ptr<tile> create_new_tile(size_t zoom, size_t x, size_t y, fs::path const & fname);

	fs::path const _cache_dir;
	size_t const _max_render_threads;
	size_t _used_zoom;
	sync_queue<mapnik::Map *> _maps;
};

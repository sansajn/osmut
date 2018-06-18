#pragma once
#include "pattern_renderer.hpp"

using pixfmt = agg::pixfmt_rgb24;

class road_symbolizer
{
public:
	using ren_base = agg::renderer_base<pixfmt>;

	road_symbolizer(ren_base & renb);
	void width(double w);
	void color(agg::rgba const & inner, agg::rgba const & outer);
	void render(agg::path_storage & path);

private:
	double _w;
	agg::rgba _inner, _outer;
	ren_base & _renb;  // rendering_buffer
};

class footway_symbolizer
{
public:
	using ren_base = agg::renderer_base<pixfmt>;

	footway_symbolizer(ren_base & renb);
	void render(agg::path_storage & path);

private:
	ren_base & _dst;  // rendering_buffer
	pattern_renderer<pixfmt> _ren;
};

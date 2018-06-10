#pragma once
#include <agg_renderer_base.h>
#include <agg_pattern_filters_rgba.h>
#include <agg_renderer_outline_image.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_path_storage.h>
#include <agg_pixfmt_rgb.h>

template <typename PixelFmt>
class pattern_renderer
{
public:
	using pixfmt_type = PixelFmt;
	using renderer_base_type = agg::renderer_base<PixelFmt>;
	using filter_type = agg::pattern_filter_bilinear_rgba8;
	using pattern_type = agg::line_image_pattern<filter_type>;
	using renderer_type = agg::renderer_outline_image<renderer_base_type, pattern_type>;
	using rasterizer_rype = agg::rasterizer_outline_aa<renderer_type>;

	pattern_renderer();
	pattern_renderer(pixfmt_type & pattern_pixels);
	void render(renderer_base_type & dst, agg::path_storage & p);
	void pattern(pixfmt_type & pattern_pixels);

private:
	pattern_type _patt;
};

using pattern_renderer_rgb24 = pattern_renderer<agg::pixfmt_rgb24>;


template <typename PixelFmt>
pattern_renderer<PixelFmt>::pattern_renderer()
	: _patt{filter_type{}}
{}

template <typename PixelFmt>
pattern_renderer<PixelFmt>::pattern_renderer(pixfmt_type & pattern_pixels)
	: _patt{filter_type{}}
{
	_patt.create(pattern_pixels);
}

template <typename PixelFmt>
void pattern_renderer<PixelFmt>::render(renderer_base_type & dst, agg::path_storage & p)
{
	renderer_type ren{dst, _patt};
	ren.scale_x(1.0);
	ren.start_x(0.0);

	rasterizer_rype ras{ren};
	ras.add_path(p);
}

template <typename PixelFmt>
void pattern_renderer<PixelFmt>::pattern(pixfmt_type & pattern_pixels)
{
	_patt.create(pattern_pixels);
}


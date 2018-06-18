#include <string>
#include <cassert>
#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_conv_stroke.h"
#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_contour.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_conv_marker.h"
#include "agg_arrowhead.h"
#include "agg_vcgen_markers_term.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgb.h"
#include <agg_path_storage.h>
#include <Magick++.h>
#include "symbolizer.hpp"

using std::string;

void road_symbolizer::render(agg::path_storage & path)
{
	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	// outer line
	agg::conv_stroke<agg::path_storage> outer{path};
	outer.width(_w);
	ras.add_path(outer);
	agg::render_scanlines_aa_solid(ras, sl, _renb, _outer);

	// inner line
	agg::conv_stroke<agg::path_storage> inner{path};
	inner.width(_w - 2.0);
	ras.add_path(inner);
	agg::render_scanlines_aa_solid(ras, sl, _renb, _inner);
}

road_symbolizer::road_symbolizer(ren_base & renb)
	: _renb{renb}
{}

void road_symbolizer::width(double w)
{
	_w = w;
}

void road_symbolizer::color(agg::rgba const & inner, agg::rgba const & outer)
{
	_inner = inner;
	_outer = outer;
}

footway_symbolizer::footway_symbolizer(ren_base & renb)
	: _dst{renb}
{
	// load pattern
	string const file_name = "3.ppm";

	Magick::Blob pixels;
	Magick::Image im{file_name};
	im.write(&pixels, "RGB");
	assert(pixels.length() == im.columns()*im.rows()*3);

	agg::rendering_buffer patt_buf;
	patt_buf.attach((uint8_t *)pixels.data(), im.columns(), im.rows(), im.columns()*3);
	pixfmt patt{patt_buf};

	_ren.pattern(patt);
}

void footway_symbolizer::render(agg::path_storage & path)
{
	_ren.render(_dst, path);
}

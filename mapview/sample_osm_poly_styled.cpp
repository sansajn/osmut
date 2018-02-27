// render polygonu s OSM mapy (pomocou agg)
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <boost/geometry.hpp>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <agg_pixfmt_rgb.h>
#include <agg_scanline_u.h>
#include <agg_path_storage.h>
#include <agg_renderer_base.h>
#include <agg_renderer_scanline.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>

int const FRAME_WIDTH = 1080;
int const FRAME_HEIGHT = FRAME_WIDTH;

using std::map;
using std::vector;
using std::string;
using std::cout;
using std::runtime_error;

namespace bg = boost::geometry;
using bg::get;

using agg::rgba8;
using agg::path_storage;
using renderer = agg::renderer_base<agg::pixfmt_rgb24>;

using node_map = map<size_t, osmium::Location>;
using polygon_ref = vector<size_t>;
using property_map = map<string, string>;
using point = bg::model::point<double, 2, bg::cs::cartesian>;
using box = bg::model::box<point>;
using ring = bg::model::ring<point>;

//! dava do suvyslosti polygon a jeho ucel (park, budova, ...)
class map_area
{
public:
	map_area(ring && shape, property_map && props);
	string const & property(string const & key) const;
	bool has_property(string const & key) const;
	ring const & shape() const;

	static string const key_not_found;

private:
	ring _shape;
	property_map _props;
};

class area_handler_impl : public osmium::handler::Handler
{
public:
	// statistics
	size_t way_count = 0;

	area_handler_impl(vector<map_area> & areas)
		: _areas{areas}, _envelope{bg::make_inverse<box>()}
	{}

	void apply(osmium::io::Reader & osm);
	box const & envelope() const;

	// handler api
	void node(osmium::Node & n);
	void way(osmium::Way & w);

private:
	vector<map_area> & _areas;
	node_map _nodes;
	vector<polygon_ref> _polys;
	vector<property_map> _props;
	box _envelope;
};

//! (x - x0) * frame_width/envelop_width
class to_frame_transform
{
public:
	to_frame_transform(double x0, double y0, double envelope_w, double envelope_h,
		double frame_w, double frame_h)
			: _x0{x0}, _y0{y0}, _envel_w{envelope_w}, _envel_h{envelope_h}
			, _frame_w{frame_w}, _frame_h{frame_h}
	{}

	point operator()(point const & p) const
	{
		return point{
			(bg::get<0>(p) - _x0) * _frame_w/_envel_w,
			(bg::get<1>(p) - _y0) * _frame_h/_envel_h};
	}

private:
	double _x0, _y0, _envel_w, _envel_h, _frame_w, _frame_h;
};

property_map to_property_map(osmium::TagList const & tags);

void render_polygon(renderer & ren, path_storage & path, rgba8 const & color);
void render_stroke_polygon(renderer & ren, path_storage & path, rgba8 const & fill_color,
	rgba8 const & stroke_color, double stroke_width = 1.0);

bool write_ppm(unsigned char const * buf, unsigned width, unsigned height,
	char const * file_name);

to_frame_transform make_to_frame_transform(box const & polygons_envelope,
	int frame_width, int frame_height);


int main(int argc, char * argv[])
{
	string input_osm = argc > 1 ? argv[1] : "data/u_uranie.osm";

	vector<map_area> areas;
	box map_envelope;

	// precitat cesty a ukladat tie uzatvorene (tzn. polygony)
	{
		osmium::io::Reader reader{input_osm};
		area_handler_impl osm{areas};
		osm.apply(reader);
		map_envelope = osm.envelope();
		cout << "polygons " << areas.size() << "/" << osm.way_count << "\n";
	}

	// vyrenderovat polygon pomocou agg
	uint8_t * buffer = new uint8_t[FRAME_WIDTH * FRAME_HEIGHT * 3];
	agg::rendering_buffer rbuf(buffer, FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH * 3);
	agg::pixfmt_rgb24 pixf{rbuf};
	renderer ren{pixf};
	ren.clear(agg::rgba8{255, 250, 230});

	// render areas
	to_frame_transform to_frame = make_to_frame_transform(map_envelope,
		FRAME_WIDTH, FRAME_HEIGHT);

	agg::path_storage path;  // create path to render

	for (map_area const & a : areas)
	{
		path.remove_all();

		ring const & shape = a.shape();

		point p = to_frame(shape[0]);
		path.move_to(get<0>(p), get<1>(p));

		for (size_t i = 1; i < shape.size(); ++i)
		{
			p = to_frame(shape[i]);
			path.line_to(get<0>(p), get<1>(p));
		}

		path.close_polygon();

		rgba8 color;
//		if (a.property("building") == "residential")
		if (a.has_property("building"))
			color = rgba8{217, 208, 201};
		else if (a.property("leisure") == "park")
			color = rgba8{200, 250, 204};
		else
			color = rgba8{120, 60, 0};

/*
		if (a.property("building") != "residential")
			continue;  // skip all not building=residential

		rgba8 color{120, 60, 0};
*/

		render_polygon(ren, path, color);
	}

	write_ppm(buffer, FRAME_WIDTH, FRAME_HEIGHT, "output/polys_styled.ppm");
	delete [] buffer;

	return 0;
}


void area_handler_impl::apply(osmium::io::Reader & osm)
{
	osmium::apply(osm, *this);

	assert(_polys.size() == _props.size());

	// create geometry
	for (size_t poly_idx = 0; poly_idx < _polys.size(); ++poly_idx)
	{
		polygon_ref const & poly = _polys[poly_idx];

		ring shape;
		for (size_t vid : poly)
		{
			osmium::Location const & loc = _nodes[vid];
			if (loc.valid())
			{
				point p{loc.lon(), loc.lat()};
				bg::append(shape, p);
				bg::expand(_envelope, p);
			}
		}

		_areas.emplace_back(move(shape), move(_props[poly_idx]));
	}

	osm.close();
}

box const & area_handler_impl::envelope() const
{
	return _envelope;
}

void area_handler_impl::node(osmium::Node & n)
{
	_nodes[n.id()] = n.location();
}

void area_handler_impl::way(osmium::Way & w)
{
	++way_count;

	if (w.nodes().is_closed())
	{
		polygon_ref p;
		for (osmium::NodeRef const & n : w.nodes())
			p.push_back(n.ref());

		_polys.push_back(p);
		_props.push_back(to_property_map(w.tags()));
	}
}


property_map to_property_map(osmium::TagList const & tags)
{
	property_map props;
	for (osmium::Tag const & t : tags)
		props[t.key()] = t.value();
	return props;
}

string const map_area::key_not_found;

map_area::map_area(ring && shape, property_map && props)
	: _shape{move(shape)}, _props{move(props)}  // TODO: ocihni jak na move like konstruktor
{}

bool map_area::has_property(string const & key) const
{
	return _props.find(key) != _props.end();
}

string const & map_area::property(string const & key) const
{
	auto it = _props.find(key);
	if (it != _props.end())
		return it->second;
	else
		return key_not_found;
}

ring const & map_area::shape() const
{
	return _shape;
}

void render_polygon(renderer & ren, path_storage & path, rgba8 const & color)
{
	agg::rasterizer_scanline_aa<> ras;

	agg::conv_curve<agg::path_storage> curve{path};
	ras.add_path(curve);

	agg::renderer_scanline_aa_solid<
		agg::renderer_base<agg::pixfmt_rgb24> > ren_sl{ren};
	ren_sl.color(color);

	agg::scanline_u8 sl;
	agg::render_scanlines(ras, sl, ren_sl);
}

void render_stroke_polygon(renderer & ren, path_storage & path, rgba8 const & fill_color,
	rgba8 const & stroke_color, double stroke_width)
{
	agg::conv_curve<agg::path_storage> curve{path};
	agg::conv_stroke<agg::conv_curve<agg::path_storage>> stroke{curve};
	stroke.width(stroke_width);

	agg::renderer_scanline_aa_solid<
		agg::renderer_base<agg::pixfmt_rgb24> > ren_sl{ren};
	ren_sl.color(fill_color);

	agg::scanline_u8 sl;
	agg::rasterizer_scanline_aa<> ras;
	ras.add_path(curve);
	agg::render_scanlines(ras, sl, ren_sl);

	ras.add_path(stroke);
	ren_sl.color(stroke_color);
	agg::render_scanlines(ras, sl, ren_sl);
}

to_frame_transform make_to_frame_transform(box const & polygons_envelope,
	int frame_width, int frame_height)
{
	double x0 = get<0>(polygons_envelope.min_corner());
	double y0 = get<1>(polygons_envelope.min_corner());
	double envel_width = get<0>(polygons_envelope.max_corner()) - get<0>(polygons_envelope.min_corner());
	double envel_height = get<1>(polygons_envelope.max_corner()) - get<1>(polygons_envelope.min_corner());

	return to_frame_transform{x0, y0, envel_width, envel_height, (double)frame_width,
		(double)frame_height};
}

bool write_ppm(const unsigned char* buf, unsigned width, unsigned height,
	const char * file_name)
{
	FILE* fd = fopen(file_name, "wb");
	if(fd)
	{
		fprintf(fd, "P6 %d %d 255 ", width, height);
		fwrite(buf, 1, width * height * 3, fd);
		fclose(fd);
		return true;
	}
	return false;
}

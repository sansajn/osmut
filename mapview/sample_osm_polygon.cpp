// render polygonu s OSM mapy (pomocou agg)
#include <map>
#include <vector>
#include <string>
#include <iostream>
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

int const frame_width = 800;
int const frame_height = 800;

using std::map;
using std::vector;
using std::string;
using std::cout;

namespace bg = boost::geometry;
using bg::get;

using node_map = map<size_t, osmium::Location>;
using polygon_ref = vector<size_t>;
using point = bg::model::point<double, 2, bg::cs::cartesian>;
using box = bg::model::box<point>;
using polygon = bg::model::polygon<point>;


class polygon_handler_impl : public osmium::handler::Handler
{
public:
	size_t way_count = 0;

	polygon_handler_impl(node_map & nodes, vector<polygon_ref> & polys)
		: _nodes{nodes}, _polys{polys}
	{}

	void node(osmium::Node & n);
	void way(osmium::Way & w);

private:
	node_map & _nodes;
	vector<polygon_ref> & _polys;
};

bool write_ppm(unsigned char const * buf, unsigned width, unsigned height,
	char const * file_name);

//! (x - x0) * frame_width/envelop_width
class to_frame_transform
{
public:
	to_frame_transform(double x0, double y0, double envelope_w, double envelope_h,
		double frame_w, double frame_h)
			: _x0{x0}, _y0{y0}, _envel_w{envelope_w}, _envel_h{envelope_h}
			, _frame_w{frame_w}, _frame_h{frame_h}
	{}

	point get(point const & p) const
	{
		return point{
			(bg::get<0>(p) - _x0) * _frame_w/_envel_w,
			(bg::get<1>(p) - _y0) * _frame_h/_envel_h};
	}

private:
	double _x0, _y0, _envel_w, _envel_h, _frame_w, _frame_h;
};




int main(int argc, char * argv[])
{
	string input_osm = argc > 1 ? argv[1] : "data/u_uranie.osm";

	vector<polygon> polys;
	box polys_envelope = bg::make_inverse<box>();

	// precitat cesty a ukladat tie uzatvorene (tzn. polygony)
	{
		node_map nodes;
		vector<polygon_ref> poly_refs;

		osmium::io::Reader osm_reader{input_osm};
		polygon_handler_impl polygon_collector{nodes, poly_refs};
		osmium::apply(osm_reader, polygon_collector);
		osm_reader.close();

		// create geometry
		for (polygon_ref const & pref : poly_refs)
		{
			polygon poly;
			for (size_t vid : pref)
			{
				osmium::Location const & loc = nodes[vid];
				if (loc.valid())
				{
					point p{loc.lon(), loc.lat()};
					bg::append(poly.outer(), p);
					bg::expand(polys_envelope, p);
				}
			}
			polys.push_back(poly);
		}

		cout << "polygons " << polys.size() << "/" << polygon_collector.way_count << "\n";
	}

	// vyrenderovat polygon pomocou agg
	unsigned char * buffer = new unsigned char[frame_width * frame_height * 3];
	agg::rendering_buffer rbuf(buffer, frame_width, frame_height, frame_width * 3);

	agg::pixfmt_rgb24 pixf{rbuf};
	agg::renderer_base<agg::pixfmt_rgb24> ren_base{pixf};
	ren_base.clear(agg::rgba8{255, 250, 230});

	agg::scanline_u8 sl;
	agg::rasterizer_scanline_aa<> ras;
	agg::renderer_scanline_aa_solid<
		agg::renderer_base<agg::pixfmt_rgb24> > ren_sl{ren_base};

	// create path to render
	agg::path_storage path;

	double x0 = get<0>(polys_envelope.min_corner());
	double y0 = get<1>(polys_envelope.min_corner());
	double envel_width = get<0>(polys_envelope.max_corner()) - get<0>(polys_envelope.min_corner());
	double envel_height = get<1>(polys_envelope.max_corner()) - get<1>(polys_envelope.min_corner());

	to_frame_transform to_frame{x0, y0, envel_width, envel_height, frame_width,
		frame_height};

	for (polygon const & poly : polys)
	{
		point p = to_frame.get(poly.outer()[0]);
		path.move_to(get<0>(p), get<1>(p));

		for (size_t i = 1; i < poly.outer().size(); ++i)
		{
			p = to_frame.get(poly.outer()[i]);
			path.line_to(get<0>(p), get<1>(p));
		}

		path.close_polygon();
	}

	agg::conv_curve<agg::path_storage> curve{path};
	ras.add_path(curve);

	ren_sl.color(agg::rgba8(120, 60, 0));
	agg::render_scanlines(ras, sl, ren_sl);

	write_ppm(buffer, frame_width, frame_height, "output/polys.ppm");
	delete [] buffer;

	return 0;
}

void polygon_handler_impl::node(osmium::Node & n)
{
	_nodes[n.id()] = n.location();
}

void polygon_handler_impl::way(osmium::Way & w)
{
	++way_count;

	if (w.nodes().is_closed())
	{
		polygon_ref p;
		for (osmium::NodeRef const & n : w.nodes())
			p.push_back(n.ref());

		_polys.push_back(p);
	}
}

bool write_ppm(const unsigned char* buf,
					unsigned width,
					unsigned height,
					const char* file_name)
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

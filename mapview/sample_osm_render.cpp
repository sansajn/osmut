// render do suboru s osm dat
#include <string>
#include <utility>
#include <chrono>
#include <iostream>
#include <boost/format.hpp>
#include <glm/glm.hpp>
#include <mapnik/map.hpp>
#include <mapnik/layer.hpp>
#include <mapnik/rule.hpp>
#include <mapnik/feature_type_style.hpp>
#include <mapnik/symbolizer.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/color_factory.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_any.hpp>
#include <mapnik/image_util.hpp>

using std::string;
using std::move;
using std::cout;
using glm::dvec2;
using namespace mapnik;

unsigned const WIDTH = 1920;
unsigned const HEIGHT = WIDTH;

dvec2 const albert_loc = {50.10563479999999714, 14.45019899999999957};


int main(int argv, char * arv[])
{
	datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");

	Map m{WIDTH, HEIGHT};
	m.set_background(parse_color("white"));
//	m.set_srs(MAPNIK_LONGLAT_PROJ);  // set map projection
	m.set_srs(MAPNIK_GMERC_PROJ);  // web/google mercator projection aka. EPSG 3857

	// styles
	feature_type_style poly_style;
	{
		rule r;
		{
			polygon_symbolizer poly_sym;
			put(poly_sym, keys::fill, parse_color("darkgreen"));
			r.append(move(poly_sym));
		}
		poly_style.add_rule(move(r));
	}
	m.insert_style("polygons", move(poly_style));

	feature_type_style poi_style;
	{
		rule r;
		{
			point_symbolizer poi_sym;
			r.append(move(poi_sym));
		}
		poi_style.add_rule(move(r));
	}
	m.insert_style("pois", move(poi_style));


	// layers
	{
		parameters p;
		p["type"] = "sqlite";
		p["file"] = "data/holesovice.db";
		p["table"] = "multipolygons";
		p["encoding"] = "utf8";

		layer lyr{"layer_0"};
		lyr.set_datasource(datasource_cache::instance().create(p));
		lyr.add_style("polygons");
//		lyr.set_srs(srs_lcc);

		m.add_layer(lyr);
	}

	m.zoom_all();

	// render
	string ofile_name = "output/holecovice.png";
	image_rgba8 im{WIDTH, HEIGHT};
	agg_renderer<image_rgba8> ren{m, im};
	ren.apply();
	save_to_file(im, ofile_name);

	cout << ofile_name << " rendered\n";

	return 0;
}

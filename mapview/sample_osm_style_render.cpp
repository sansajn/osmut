// render do suboru s osm dat
#include <string>
#include <utility>
#include <vector>
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
#include <mapnik/load_map.hpp>

using std::string;
using std::move;
using std::vector;
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
	load_map(m, "data/holesovice_style.xml");

	// datovy layer si musim vytvorit
	{
		parameters p;
		p["type"] = "sqlite";
		p["file"] = "data/holesovice.db";
		p["table"] = "multipolygons";
		p["encoding"] = "utf8";

		layer lay{"Layer_0"};
		lay.set_datasource(datasource_cache::instance().create(p));
		lay.add_style("polygons");

		m.add_layer(lay);
	}

	{
		parameters p;
		p["type"] = "sqlite";
		p["file"] = "data/holesovice.db";
		p["table"] = "lines";
		p["encoding"] = "utf8";

		layer lay{"Layer_1"};
		lay.set_datasource(datasource_cache::instance().create(p));
		lay.add_style("lines");

		m.add_layer(lay);
	}

	m.zoom_all();

	// render
	string ofile_name = "output/hello_style.png";
	image_rgba8 im{WIDTH, HEIGHT};
	agg_renderer<image_rgba8> ren{m, im};
	ren.apply();
	save_to_file(im, ofile_name);

	cout << ofile_name << " rendered\n";

	return 0;
}


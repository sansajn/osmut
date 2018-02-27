// render do suboru s osm dat
#include <string>
#include <iostream>
#include <mapnik/map.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/load_map.hpp>

using std::string;
using std::cout;
using namespace mapnik;

unsigned const WIDTH = 1920;
unsigned const HEIGHT = WIDTH;


int main(int argc, char * argv[])
{
	string style;

	if (argc > 1)
		style = argv[1];
	else
	{
		cout << "style_render STYLE\n"
			<< "error: style missing\n";
		return 1;
	}

	datasource_cache::instance().register_datasources("/usr/lib/mapnik/3.0/input/");

	Map m{WIDTH, HEIGHT};
	load_map(m, style);

	m.zoom_all();

	// render
	string ofile_name = "output/out.png";
	image_rgba8 im{WIDTH, HEIGHT};
	agg_renderer<image_rgba8> ren{m, im};
	ren.apply();
	save_to_file(im, ofile_name);

	cout << ofile_name << " rendered\n";

	return 0;
}

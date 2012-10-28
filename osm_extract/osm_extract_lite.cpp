/* $ ./osm_extract ../data/petrzalka.osm 17.0998 48.1166 17.1096 48.1231 petrzalka-extract.osm */
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "geometry.h"
#include "osm_range.h"
using std::set;
using std::vector;
using std::string;
using std::ofstream;
using std::cout;
using namespace irr::io;


void unrecoverable_error(boost::format const & msg);

inline node_range make_node_r(IrrXMLReader & xml, node & buf)
{
	return node_range(node_iterator(&xml, &buf), node_iterator());
}


int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(
			boost::format("not enought parameters, 6 needed get %1%") % argc);

	string osm_fname = argv[1];

	IrrXMLReader * xml = createIrrXMLReader(osm_fname.c_str());

	ofstream fout("test.txt");

	rect<signed_coordinate> bounds{signed_coordinate{481140000, 170000000},
		signed_coordinate{490000000, 180000000}};

	// nodes
	node nodebuf;
	for (auto r = make_node_r(*xml, nodebuf); r; ++r)
	{
		if (bounds.contains(signed_coordinate{
			int32_t(r->lat*1e7), int32_t(r->lon*1e7)}))
		{
			cout << boost::format("%1%, %2%\n")	% r->lat % r->lon;
		}

		float lat = r->lat;
		float lon = r->lon;

		fout << lat << " " << lon << "\n";
	}

	fout.close();

	return 0;
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


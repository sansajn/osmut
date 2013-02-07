/* vylistuje špecificku všetky way-properties */
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include "osm_range.h"

using std::string;
using std::cout;
using std::ostream;
using osmut::xml_reader;


void print(way const & w);
void unrecoverable_error(boost::format const & msg);


struct gps_coordinate
{
	float lat;
	float lon;

	gps_coordinate() {}

	gps_coordinate(float latitude, float longitude)
		: lat(latitude), lon(longitude)
	{}
};


int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		unrecoverable_error(
			boost::format(
				"not enought parameters, 3 needed (<input>, <key>, <value>)"));
	}

	string osm_fname = argv[1];
	xml_reader osm(osm_fname.c_str());

	cout << "open file '" << osm_fname << "'\n";

	cout << "reading nodes ..." << std::flush;

	// skip nodes
	int nnodes = 0;
	for (auto r = make_node_range(osm); r; ++r)
		nnodes += 1;

	cout << "reading ways ..." << std::flush;

	string key = argv[2];
	string value = argv[3];

	int nways = 0;
	for (auto r = make_way_range(osm); r; ++r)
	{
		way w = *r;
		if (w.tags)
		{
			for (auto & t : *w.tags)
			{
				if (t.first == key && t.second == value)
				{
					print(w);
				  	cout << "\n";
				}
			}
		}
		nways += 1;
	}

	cout << "\n\n";
	cout << "nodes:" << nnodes << ", ways:" << nways << "\n";

	return 0;
}

void print(way const & w)
{
	cout << "way:" <<  w.id << "\n"
		<< "  nodes:" << w.node_ids.size() << "\n";

	for (auto & t : *w.tags)
		cout << "  k:" << t.first << ", v:" << t.second << "\n";
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


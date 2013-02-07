/* vypíše informácie o osm súbore */
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "osm_range.h"

using std::cout;
using std::string;
using osmut::xml_reader;

struct stats
{
	int nodes;
	int ways;
};

void unrecoverable_error(boost::format const & msg);

int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(
			boost::format("not enought parameters, 2 needed gets %1%") % argc);

	string osm_fname = argv[1];

	xml_reader osm(osm_fname.c_str());

	stats st = {0};

	cout << "reading nodes ..." << std::flush;

	for (auto r = make_node_range(osm); r; ++r)
		st.nodes += 1;

	cout << "\n";
	cout << "reading ways ..." << std::flush;

	for (auto r = make_way_range(osm); r; ++r)
		st.ways += 1;

	cout << "\n";
	cout << "results\n"
		<< "nodes: " << st.nodes << "\n"
		<< "ways : " << st.ways << "\n";

	return 0;
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


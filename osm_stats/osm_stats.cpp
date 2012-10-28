/* vypíše informácie o osm súbore */
#include <iostream>
#include <string>
#include <boost/format.hpp>
#include "osm_range.h"
using std::cout;
using std::string;
using namespace irr::io;

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

	IrrXMLReader * xml = createIrrXMLReader(osm_fname.c_str());
	if (!xml)
		unrecoverable_error(
			boost::format("can not open input file '%1%'") % osm_fname);

	stats st = {0};

	cout << "reading nodes ..." << std::flush;

/*
	node nodebuf;
	for (auto r = make_node_r(*xml, nodebuf); r; ++r)
		st.nodes += 1;
*/

	node nodebuf;
	for (node_iterator node_it(xml, &nodebuf); node_it != node_iterator(); 
		++node_it)
	{
		st.nodes += 1;
	}

	cout << "\n";
	cout << "reading ways ..." << std::flush;

	way waybuf;
	for (auto r = make_way_r(*xml, waybuf); r; ++r)
		st.ways += 1;

	cout << "\n";
	cout << "results\n"
		<< "nodes: " << st.nodes << "\n"
		<< "ways : " << st.ways << "\n";

	delete xml;

	return 0;
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


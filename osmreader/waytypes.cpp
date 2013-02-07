/* vylistuje všetky way-properties */
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include "osm_range.h"

using std::map;
using std::set;
using std::cout;
using std::string;
using osmut::xml_reader;


void dump(map<string, set<string>> const & c);
int count_keys(map<string, set<string>> const & c);
void unrecoverable_error(boost::format const & msg);


int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(
			boost::format("not enought parameters, 1 needed"));

	string osm_fname = argv[1];
	xml_reader osm(osm_fname.c_str());

	cout << "open file '" << osm_fname << "'\n";

	for (auto r = make_node_range(osm); r; ++r)
		continue;

	cout << "reading ways ..." << std::flush;

	map<string, set<string>> way_types;

	int nways = 0;
	for (auto r = make_way_range(osm); r; ++r)
	{
		way w = *r;
		if (w.tags)
			for (auto & t : *w.tags)
				way_types[t.first].insert(t.second);
		nways += 1;
	}

	cout << "\n\n{stats}\n";
	cout << "features:" << way_types.size() << "\n";
	cout << "keys:" << count_keys(way_types) << "\n";
	cout << "ways:" << nways << "\n";

	cout << "\n";

	dump(way_types);

	return 0;
}

void dump(map<string, set<string>> const & c)
{
	for (auto & f : c)
	{
		string const & feature = f.first;
		for (auto & v : f.second)
			cout << feature << ":" << v << "\n";
	}
}

int count_keys(map<string, set<string>> const & c)
{
	int count = 0;
	for (auto & vals : c)
		count += vals.second.size();
	return count;
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


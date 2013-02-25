/* vylistuje špecificku (na zaklade 'key' a 'value') way-properties
	$ listways <osm-input> <key> <value> */
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include "osm_range.h"
#include "range.h"

using std::map;
using std::vector;
using std::pair;
using std::string;
using std::cout;
using std::ostream;
using osmut::xml_reader;


typedef pair<string, string> filter_t;
typedef vector<filter_t> filters_t;

class way_filter
{
public:
	way_filter(filters_t const & f)
		: _filters(f)
	{}

	bool operator()(way const & w) const;

private:
	bool test_filter(way const & w, filter_t const & f) const;

	filters_t const & _filters;
};


void save_osm_map(string const & osm_outfname, map<int, node> const & nodes,
	vector<way> const & ways);

filter_t parse_filter(char * argv);
void unrecoverable_error(boost::format const & msg);


int main(int argc, char * argv[])
{
	if (argc < 3)
	{
		unrecoverable_error(boost::format(
			"not enought parameters, 3 needed (<input>, [<filter-pair> ...], <output>)"));
	}

	vector<filter_t> filters;
	for (int i = 2; i < argc-1; ++i)
		filters.push_back(parse_filter(argv[i]));


	string osm_infname = argv[1];
	xml_reader osm(osm_infname.c_str());

	cout << "open file '" << osm_infname << "'\n";

	cout << "reading nodes ..." << std::flush;

	// save nodes for future use
	map<int, node> nodes;
	for (auto r = make_node_range(osm); r; ++r)
		nodes[r->id] = *r;


	cout << "reading ways ..." << std::flush;

	vector<way> ways;
	for (auto r = filtered_range(make_way_range(osm), way_filter(filters)); r; ++r)
		ways.push_back(*r);

	cout << "\n\n";
	cout << "nodes:" << nodes.size() << ", ways:" << ways.size() << "\n";


	string osm_outfname = argv[argc-1];
	save_osm_map(osm_outfname, nodes, ways);

	return 0;
}


bool way_filter::operator()(way const & w) const
{
	if (!w.tags)
		return false;

	for (auto & f : _filters)
		if (!test_filter(w, f))
			return false;

	return true;
}

bool way_filter::test_filter(way const & w, filter_t const & f) const
{
	auto it_key = w.tags->find(f.first);
	return it_key != w.tags->end() && it_key->second == f.second;
}

filter_t parse_filter(char * argv)
{
	string argument(argv);
	string::size_type sep_pos = argument.find(':');
	if (sep_pos != string::npos)
		return make_pair(string(argument.begin(), argument.begin()+sep_pos),
			string(argument.begin()+sep_pos, argument.end()));
	else
		return make_pair(string(), string());
}

void save_osm_map(string const & osm_outfname, map<int, node> const & nodes,
	vector<way> const & ways)
{
}

void unrecoverable_error(boost::format const & msg)
{
	std::cout << msg << "\n";
	exit(1);
}


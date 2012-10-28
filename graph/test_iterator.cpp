/* Zakladný test funkčnosti node a wai iterátorov, generuje pozície do
'nodes.py'.
\node čítanie koordinátov je stratové, knižnica nedáva presné hodnoty, ani
atof funkcia nie je presnejšia. Hodnoty treba načítať ako int (čím sa zvíši 
rýchlosť aj presnosť. */

#include <set>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <irrXML.h>
#include "iterator.h"

using std::set;
using std::vector;
using std::string;
using std::cout;
using std::ofstream;
using namespace irr::io;


void dump_to_python(string const & fname, vector<node> const & nodes);


int main(int argc, char * argv[])
{
	string xml_fname("test.xml");
	if (argc > 1)
		xml_fname = argv[1];

	IrrXMLReader * xml = createIrrXMLReader(xml_fname.c_str());

	assert(xml && "reader instance empty");

	int n_nodes = 0;
	int collisions = 0;
	set<int> ids_collision_detector;

	cout << "nodes:\n";

	vector<node> nodes;

	node n;
	for (node_iterator it(xml, &n); it != node_iterator(); ++it)
	{
		n_nodes += 1;
		auto ret = ids_collision_detector.insert(it->id);
		if (!ret.second)
		{
			collisions += 1;
			cout << "id collision detected id:" << it->id << "\n";
		}
		nodes.push_back(*it);
	}

	cout << "-----\n"
		<< "collisions: " << collisions << "/" << n_nodes << "\n";


	n_nodes = 0;
	collisions = 0;
	ids_collision_detector.clear();

	cout << "\nways:\n";

	way w;
	for (way_iterator it(xml, &w); it != way_iterator(); ++it)
	{
		n_nodes += 1;
		auto ret = ids_collision_detector.insert(it->id);
		if (!ret.second)
		{
			collisions += 1;
			cout << "id collision detected id:" << it->id << "\n";
		}
	}

	cout << "-----\n"
		<< "collisions: " << collisions << "/" << n_nodes << "\n";


	dump_to_python("nodes.py", nodes);

	
	cout << "\ndone!\n";

	return 0;
}

void dump_to_python(string const & fname, vector<node> const & nodes)
{
	ofstream fout(fname.c_str());
	if (!fout.is_open())
		return;

	fout.precision(10);

	fout << "nodes = [\n";

	for (auto & n : nodes)
		fout << "\t(" << n.lat << "," << n.lon << "),\n";

	fout << "]\n";

	fout.close();
}


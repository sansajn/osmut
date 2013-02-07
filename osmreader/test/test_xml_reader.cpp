// test pre attribute_iterator
#include <vector>
#include <utility>
#include <string>
#include <iostream>
#include <cassert>
#include "../xml_reader.h"

using std::cout;
using std::vector;
using std::string;
using std::pair;
using std::make_pair;
using namespace osmut;

void process_node(xml_reader & reader);

int main(int argc, char * argv[])
{
	LIBXML_TEST_VERSION

	xml_reader reader(argv[1]);
	while (reader.read())
		process_node(reader);

	xmlCleanupParser();
	return 0;
}

void process_node(xml_reader & reader)
{
	int type = reader.node_type();
	char const * name = reader.node_name();
	int n_attribs = reader.attribute_count();

	vector<pair<string, string>> attrs;
	if (type == 1 && n_attribs)  // 1=Element
	{
		for (auto r = reader.attributes(); r; ++r)
			attrs.push_back(
				make_pair(string((*r).first), string((*r).second)));
	}

	assert(name && "uzol nema ziadne meno");

	cout << "type" << ":" << type << ", name:" << name 
		<< ", attribs:" << n_attribs;

	if (!attrs.empty())
	{
		cout << " [";
		for (auto & a : attrs)
			cout << a.first << ":" << a.second << ", ";
		cout << "]";
	}

	cout << "\n";
}

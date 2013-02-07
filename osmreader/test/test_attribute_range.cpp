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

void process_node(xmlTextReader * reader);

int main(int argc, char * argv[])
{
	LIBXML_TEST_VERSION

	xmlTextReader * reader = xmlReaderForFile(argv[1], NULL, 0);
	
	assert(reader && "can't create xml reader");

	int ret = xmlTextReaderRead(reader);
	while (ret == 1)
	{
		process_node(reader);
		ret = xmlTextReaderRead(reader);
	}

	xmlFreeTextReader(reader);

	if (ret != 0)
		cout << "failed to parse '" << argv[1] << "\n";

	xmlCleanupParser();
	return 0;
}

void process_node(xmlTextReader * reader)
{
	int type = xmlTextReaderNodeType(reader);
	xmlChar const * name = xmlTextReaderConstName(reader);
	bool has_attribs = xmlTextReaderHasAttributes(reader) == 1;
	bool empty_elem = xmlTextReaderIsEmptyElement(reader) == 1;
	int n_attribs = xmlTextReaderAttributeCount(reader);

	vector<pair<string, string>> attrs;
	if (type == 1 && n_attribs)  // 1=Element
	{
		for (auto r = attribute_range(reader); r; ++r)
			attrs.push_back(
				make_pair(string((*r).first), string((*r).second)));
	}

	assert(name && "uzol nema ziadne meno");

	cout << "type" << ":" << type << ", name:" << name 
		<< ", has-attribs:" << (has_attribs ? 1 : 0) 
		<< ", empty:" << (empty_elem ? 1 : 0) 
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

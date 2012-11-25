#include <string>
#include <iostream>
#include "osm_parser.h"
using std::string;
using std::cout;
using osmut::parser;

void parse(parser & p);
void unrecoverable_error(boost::format const & msg);

int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(boost::format("input file argument missing"));

	parser p(argv[1]);

	parse(p);

	if (p.good())
		cout << "done!\n";
	else
		unrecoverable_error(p.error_message());

	return 0;
}

void parse(parser & p)
{
	while (p.read())
	{
		parser::e_node type = p.node_type();
		if (type == parser::start_node || type == parser::empty_node)
		{
			cout << "node:" << p.node_name() << "\n";
			for (int i = 0; i < p.attribute_count(); ++i)
			{
				cout << "  k:" << p.attribute_name(i) << ", v:"
					<< p.attribute_value(i) << "\n";
			}
		}
	}
}

void unrecoverable_error( boost::format const & msg )
{
	cout << "\nunrecoverable error: " << msg << std::endl;
	exit(1);
}


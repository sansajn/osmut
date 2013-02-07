#include "osm_parser.h"
#include <iostream>
using std::max;
using std::cout;
using std::string;


namespace osmut 
{

parser::parser(char const * file_name)
	: _osm(file_name)
{
	_error = false;
	ch = ' ';
	next_ch = ' ';
	_curnode.type = undefined_node;

	// tebug
	tabs = 0;

	if (!_osm.is_open())
		error(boost::format("can't open '%1%' file") % file_name);
	
	get_sym();
	if (sym == pi_opener)
		eat_pi_section();
}

bool parser::read()
{
	node();
	get_sym();
	return good() && !_osm.eof() && sym != eof;
}

void parser::node()
{
	if (sym == tag_opener)
		start_tag();
	else if (sym == end_tag_opener)
		end_tag();
	else
		error("start-tag opener '<', or end-tag opener '</' expected");
}

void parser::eat_pi_section()
{
	do {
		get_sym();
	} while (sym != pi_closer);

	get_sym();
}

void parser::start_tag()
{
#ifdef PRINT_OSM_TREE
	tabs += 1;
	cout << offset() << ":start-tag:\n";
#endif

	// name
	get_sym();
	if (sym != name)
		error("start-tag name expected");

	_curnode.name = name_buf;
	
	_curnode.attribs.clear();
	
	// (name=valaue)*
	get_sym();
	while (sym == name)
	{
		get_sym();
		if (sym != equall)
			error("symbol '=' expected");

		get_sym();
		if (sym != text)
			error("value expected");

		_curnode.attribs.push_back(make_pair(name_buf, text_buf));

#ifdef PRINT_OSM_TREE
		tabs += 1;
		cout << offset() 
			<< boost::format("%1%=%2%") % name_buf % text_buf << "\n";
		tabs -= 1;
#endif

		get_sym();
	}

	// end_tag
	if (sym == tag_closer)
		_curnode.type = start_node;
	else if (sym == empty_tag_closer)
		_curnode.type = empty_node;
	else
		error("tag closer ('>' or '/>') expected.");

#ifdef PRINT_OSM_TREE
	if (sym == empty_tag_closer)
	{
		cout << offset() << ":tag-end (empty):\n";
		tabs -= 1;
	}
#endif
}

void parser::end_tag()
{
#ifdef PRINT_OSM_TREE
	cout << offset() << ":end-tag:\n";
	tabs -= 1;
#endif

	// name
	get_sym();
	if (sym != name)
		error("tag name expected");

	// >
	get_sym();
	if (sym != tag_closer)
		error("tag closer expected");

	_curnode.name = name_buf;
	_curnode.type = end_node;
}

void parser::get_sym()
{
	get_nospace_char();

	if (isalpha(ch))
	{
		sym = name;
		read_name();
	}
	else if (ch == '"')
	{
		sym = text;
		read_text();
	}
	else if (ch == '=')
	{
		sym = equall;
	}
	else if (ch == '\0')
	{
		sym = eof;
	}
	else
		get_complex_sym();
}

void parser::get_complex_sym()
{
	switch (ch)
	{
		case '<':
			if (next_ch != '/' && next_ch != '?')
			{
				sym = tag_opener;
			}
			else if (next_ch == '/')
			{
				get_char();
				sym = end_tag_opener;
			}
			else if (next_ch == '?')
			{
				get_char();
				sym = pi_opener;
			}
			break;  // <

		case '>':
			get_char();
			sym = tag_closer;
			break;

		case '/':
			if (next_ch == '>')
			{
				get_char();
				sym = empty_tag_closer;
			}
			break;
		
		case '?':
			if (next_ch == '>')
			{
				get_char();
				sym = pi_closer;
			}
			break;
	}  // switch
}

void parser::read_name()
{
	name_buf.clear();
	name_buf += ch;

	while (isalpha(next_ch))
	{
		get_char();
		name_buf += ch;
	}
}

void parser::read_text()
{
	text_buf.clear();

	get_char();
	while (ch != '"')
	{
		text_buf += ch;
		get_char();
	}
}

void parser::get_nospace_char()
{
	do 
	{
		get_char();
	} 
	while (isspace(ch) && ch != '\0');
}

void parser::get_char()
{
	ch = next_ch;
	if (ch == '\0')
		return;

	if (!_osm.eof())
		next_ch = _osm.get();
	else
	{
		next_ch = '\0';
//		error("unexpected eof");
	}	
}

void parser::error(char const * msg)
{
	_error = true;
	_errmsg = boost::format("%1%") % msg;

	error(_errmsg);
}

void parser::error(boost::format const & msg)
{
	_error = true;
	cout << "\nparser error: " << msg << "\n";

	cout << "\tch:" << ch << "\n"
		<< "\tnext_ch:" << next_ch << "\n"
		<< "\tsym=" << sym << "\n";

	string buf;
	_osm.getline(buf);
	cout << "\trest of line:" << buf;

	cout << std::endl;

	exit(1);
}

void parser::unrecoverable_error( boost::format const & msg )
{
	_error = true;
	cout << "\nerror: " << msg << std::endl;
	exit(1);
}

string parser::offset()
{
	string s;
	for (int i = 0; i < max(tabs, 0); ++i)
		s += ' ';
	return s;
}


};  // osmut

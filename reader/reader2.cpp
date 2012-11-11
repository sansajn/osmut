/*

get_char()


get_sym()
	while get_char()
		if ch == '<'  // zaciatok tagu
			get_char()
			if ch == '\'
				sym = end_tag
				read_name()
			else
				sym = start_tag
				read_name()
			break;
		else if ch == '>'  // konic tagu
			continue
		else if ch == '='
			sym = equal
		else if ch == '\'
		

<, >, =, \, "
		


name = alpha_ >> *char_

key
value
start_tag
end_tag
node
osm

parse()
	get_char()
	osm()

osm()
	if ch == '<'
		start_tag()
		while ch != "</"
			node()
		end_tag()
	else
		error 'expected <'

node()
	osm()

start_tag()
	key()
	while ch != '>'
		key()
		if ch != '='
			error 'expected ='
		value()
		
end_tag()
	key()
	if ch != '>'
		error 'epected >'  // tag ending, expected >

key()
	if ch == alpha
		while ch != alpha_num
			get_char()
	else
		error 'expcted alpha'  // key name must start with alphabet

value()
	if ch == '"'
		do
			get_char()
			if ch == '\\'
				get_char()
		while ch != '"'
	else
		error 'epected "'  // argument value must be enclosed by "

*/

#include <algorithm>
#include <cctype>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

using std::max;
using std::isalpha;
using std::cout;
using std::string;
using std::getline;
using std::ifstream;

char ch;
char next_ch;
string key_buf;
string value_buf;
ifstream fin;

int tabs;

void init();
void parse();
void osm();
void node();
void start_tag();
void end_tag();
void key();
void value();
void get_char();
void get_raw_char();

string offset();
void parser_error( boost::format const & msg );
void unrecoverable_error( boost::format const & msg );


int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(boost::format("input file argument missing"));

	fin.open(argv[1]);
	if (!fin.is_open())
		unrecoverable_error(boost::format("can't open '%1%' file") % argv[1]);

	parse();

	fin.close();

	return 0;
}

void init()
{
	get_char();
	get_char();
	key_buf = "";
	value_buf = "";
	tabs = -1;
}

void parse()
{
	init();
	osm();
	cout << "parsing done!\n";
}

void osm()
{
	tabs += 1;
	cout << offset() << ":osm:\n";

	if (ch == '<')
	{
		start_tag();

		while (ch != '<' || next_ch != '/')
		{
			node();
			if (ch == '\0')  // eof
				return;
		}

		if (ch == '<')
			get_char();  // eat /
		else
			parser_error(boost::format("expected <"));

		end_tag();
	} 
	else
		parser_error(
			boost::format("tag must start with '<' not '%1%'") % ch);

	tabs -= 1;
}

void node()
{
	tabs += 1;
	cout << offset() << ":node:\n";

	osm();

	tabs -= 1;
}

void start_tag()
{
	tabs += 1;
	cout << offset() << ":start-tag:\n";

	key();
	while (ch != '>' && ch != '/' && next_ch != '>')
	{
		key();
		if (ch != '=')
			parser_error(boost::format("expected ="));
		value();
	}

	if (ch == '/' && next_ch == '>')
		end_tag();
	else
		get_char();
}

void end_tag()
{
	cout << offset() << ":end-tag:\n";

	if (ch == '/' && next_ch == '>')
		get_char();
	else
	{
		key();
		if (ch != '>')
			parser_error(boost::format("tag ending, expected '>'"));
	}

	get_char();

	tabs -= 1;
}

void key()
{
	tabs += 1;
	cout << offset() << ":key:";

	get_char();
	if (isalpha(ch))
	{		
		key_buf = "";
		while (isalnum(ch))
		{
			key_buf += ch;
			get_raw_char();
		}
	}
	else
		parser_error(
			boost::format("key name must start with an alphabet"));

	cout << boost::format(" (key=%1%)\n") % key_buf;

	tabs -= 1;
}

void value()
{
	tabs += 1;
	cout << offset() << ":value:";

	get_char();
	if (ch == '"')
	{
		value_buf = "";

		get_raw_char();
		while (ch != '"')
		{
			if (ch == '\\')
				get_raw_char();
			value_buf += ch;
			get_raw_char();
		}

		if (!isspace(next_ch))
			get_char();  // character after "
	}
	else
		parser_error(
			boost::format("argument value must be enclosed in \""));

	cout << boost::format(" (value=\"%1%\")\n") % value_buf;

	tabs -= 1;
}


void get_char()  //!< vynecháva medzery
{
	do 
	{	
		get_raw_char();
	}
	while (fin && isspace(ch));

	if (!fin)
	{
		next_ch = 0;
		if (isspace(ch))
			ch = 0;
	}
}

void get_raw_char()  //! prečíta nasledujúci znak (vrátane medzier)
{
	ch = next_ch;
	fin.get(next_ch);
}


string offset()
{
	string s;
	for (int i = 0; i < tabs; ++i)
		s += ' ';
	return s;
}

void unrecoverable_error( boost::format const & msg )
{
	cout << "\nunrecoverable error: " << msg << std::endl;
	exit(1);
}

void parser_error( boost::format const & msg )
{
	cout << "\nparser error: " << msg << "\n";

	cout << "\tch:" << ch << "\n"
		<< "\tnext_ch:" << next_ch << "\n";

	if (fin)
	{
		string buf;
		getline(fin, buf);
		cout << "\trest of line:" << buf;
	}

	cout << std::endl;

	exit(1);
}

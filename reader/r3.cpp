/* recognized symbols: </, />, <?, ?>, text, name
slower then libxml (slovakia.osm 2:26 vs 2:03) */

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/utility.hpp>


//#define PRINT_OSM_TREE

using std::max;
using std::isalpha;
using std::string;
using std::ifstream;
using std::ios;
using std::cout;


template <int BufferSize = 4096>
class file_content : boost::noncopyable
{
public:
	file_content(char const * file_name) 
		: _fin(file_name, ios::in|ios::binary), _bufidx(0) 
	{
		alloc();
		fill();
	}

	~file_content() 
	{
		free();
		_fin.close();
	}

	bool is_open() const {return _fin.is_open();}

	char get() 
	{
		if (_bufidx == size())
		{
			fill();
			_bufidx = 0;
		}
		return _buf[_bufidx++];
	}

	void getline(string & s) 
	{
		char ch = get();
		while (!eof() && ch != '\n')
		{
			s += ch;
			ch = get();
		}
	}

	bool eof() const 
	{
		return _fin && (_bufidx == size());
	}

private:
	void alloc() {_buf = new char[BufferSize];}
	void free() {delete [] _buf; _buf = nullptr;}
	void fill() {_fin.read(_buf, BufferSize);}
	int size() const {return _fin.gcount();}

	ifstream _fin;
	char * _buf;
	int _bufidx;
};


enum e_symbol
{
	tag_open = 1,  // tag_opener
	tag_close,  // tag_closer
	etag_open, 
	empty_tag_close,
	pi_open, 
	pi_close, 
	equall, 
	text, 
	name
};

e_symbol sym;
char ch;
char next_ch;
string name_buf;
string text_buf;

file_content<4096> * ifile;

int tabs;

void init();
void parse();
void osm();
void node();
void start_tag();
void end_tag();

void get_sym();
void get_complex_sym();
void read_name();
void read_text();
void get_nospace_char();
void get_char();

bool not_one_of(int x, char const * p);


string offset();
void parser_error(std::string const & msg);
void parser_error(boost::format const & msg);
void unrecoverable_error(boost::format const & msg);


int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error(boost::format("input file argument missing"));

	ifile = new file_content<4096>(argv[1]);
	if (!ifile->is_open())
		unrecoverable_error(boost::format("can't open '%1%' file") % argv[1]);

	parse();

	delete ifile;

	return 0;
}

void init()
{
	tabs = 0;
	ch = ' ';
	next_ch = ' ';
}

void parse()
{
	init();

	get_sym();

	if (sym == pi_open)  // ignore pi section
	{
		do {
			get_sym();
		} while (sym != pi_close);

		get_sym();
	}

	node();

	cout << "done!\n";
}

void osm()
{
	if (sym == tag_open)
		start_tag();
	else
		parser_error("tag open expected");

	if (sym == empty_tag_close)
		return;

	get_sym();

	while (sym == tag_open)
	{
		node();
		get_sym();
	}

	if (sym == etag_open)
		end_tag();
	else
		parser_error("new node, or end-tag opener '</' expected");
}

void node()
{
#ifdef PRINT_OSM_TREE
	tabs += 1;
	cout << offset() << ":node:\n";
#endif

	osm();

#ifdef PRINT_OSM_TREE
	tabs -= 1;
#endif
}

void start_tag()
{
#ifdef PRINT_OSM_TREE
	tabs += 1;
	cout << offset() << ":start-tag:\n";
#endif

	// name
	get_sym();
	if (sym != name)
		parser_error("tag name expected");
	
	// (name=valaue)*
	get_sym();
	while (sym == name)
	{
		get_sym();
		if (sym != equall)
			parser_error("symbol '=' expected");

		get_sym();
		if (sym != text)
			parser_error("value expected");

#ifdef PRINT_OSM_TREE
		tabs += 1;
		cout << offset() 
			<< boost::format("%1%=%2%") % name_buf % text_buf << "\n";
		tabs -= 1;
#endif

		get_sym();
	}

	// end_tag
	if (sym != tag_close && sym != empty_tag_close)
		parser_error("tag close ('>' or '/>') expected.");

#ifdef PRINT_OSM_TREE
	if (sym == empty_tag_close)
	{
		cout << offset() << ":tag-end (empty):\n";
		tabs -= 1;
	}
#endif
}

void end_tag()
{
#ifdef PRINT_OSM_TREE
	cout << offset() << ":end-tag:\n";
	tabs -= 1;
#endif

	// name
	get_sym();
	if (sym != name)
		parser_error("tag name expected");

	// >
	get_sym();
	if (sym != tag_close)
		parser_error("tag close expected");
}


void get_sym()
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
	else
		get_complex_sym();
}

void get_complex_sym()
{
	switch (ch)
	{
		case '<':
			if (not_one_of(next_ch, "/?"))
			{
				sym = tag_open;
			}
			else if (next_ch == '/')
			{
				get_char();
				sym = etag_open;
			}
			else if (next_ch == '?')
			{
				get_char();
				sym = pi_open;
			}
			break;  // <

		case '/':
			if (next_ch == '>')
			{
				get_char();
				sym = empty_tag_close;
			}
			break;

		case '>':
			get_char();
			sym = tag_close;
			break;

		case '?':
			if (next_ch == '>')
			{
				get_char();
				sym = pi_close;
			}
			break;
	}  // switch
}

void read_name()
{
	name_buf.clear();
	name_buf += ch;

	while (isalpha(next_ch))
	{
		get_char();
		name_buf += ch;
	}
}

void read_text()
{
	text_buf.clear();

	get_char();
	while (ch != '"')
	{
		text_buf += ch;
		get_char();
	}
}

bool not_one_of(int x, char const * p)
{
	for (; *p; ++p)
	{
		if (x == *p)
			return false;
	}
	return true;
}

void get_nospace_char()
{
	do 
	{
		get_char();
	} 
	while (isspace(ch));
}

void get_char()
{
	ch = next_ch;
	next_ch = ifile->get();
}

void parser_error(std::string const & msg)
{
	parser_error(boost::format("%1%") % msg);
}

void parser_error( boost::format const & msg )
{
	cout << "\nparser error: " << msg << "\n";

	cout << "\tch:" << ch << "\n"
		<< "\tnext_ch:" << next_ch << "\n"
		<< "\tsym=" << sym << "\n";

	string buf;
	ifile->getline(buf);
	cout << "\trest of line:" << buf;

	cout << std::endl;

	exit(1);
}

void unrecoverable_error( boost::format const & msg )
{
	cout << "\nunrecoverable error: " << msg << std::endl;
	exit(1);
}

string offset()
{
	string s;
	for (int i = 0; i < max(tabs, 0); ++i)
		s += ' ';
	return s;
}


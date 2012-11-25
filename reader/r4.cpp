// r4

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
		if (!eof() && _bufidx == size())
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

	bool eof() const {return !_fin && (_bufidx == size());}

private:
	void alloc() {_buf = new char[BufferSize];}
	void free() {delete [] _buf; _buf = nullptr;}	
	void fill() {_fin.read(_buf, BufferSize);}
	int size() const {return _fin.gcount();}

	ifstream _fin;
	char * _buf;
	int _bufidx;
};

class parser
{
public:
	enum e_symbol
	{
		tag_opener = 1,
		tag_closer,
		end_tag_opener, 
		empty_tag_closer,
		pi_opener, 
		pi_closer, 
		equall, 
		text, 
		name,
		eof
	};

	parser(char const * file_name);
	~parser() {}

	bool read() 
	{
		node();
		get_sym();
		return good() && !_osm.eof() && sym != eof;
	}

	void node() 
	{
		if (sym == tag_opener)
			start_tag();
		else if (sym == end_tag_opener)
			end_tag();
		else
			error("start-tag opener '<', or end-tag opener '</' expected");
	}

	void eat_pi_section()
	{
		do {
			get_sym();
		} while (sym != pi_closer);

		get_sym();
	}

	// error handling
	bool good() const {return !_error;}
	boost::format const & error_message() const {return _errmsg;}

private:
	void __parse();
	void __node();
	void start_tag();
	void end_tag();

	void get_sym();
	void get_complex_sym();
	void read_name();
	void read_text();
	void get_nospace_char();
	void get_char();

	void error(char const * msg);
	void error(boost::format const & msg);
	string offset();

private:
	e_symbol sym;
	char ch;
	char next_ch;
	string name_buf;
	string text_buf;

	file_content<4096> _osm;

	// error handling
	bool _error;
	boost::format _errmsg;

	int tabs;
};


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
		continue;
}

parser::parser(char const * file_name)
	: _osm(file_name)
{
	_error = false;
	ch = ' ';
	next_ch = ' ';

	// tebug
	tabs = 0;

	if (!_osm.is_open())
		error(boost::format("can't open '%1%' file") % file_name);
	
	get_sym();
	if (sym == pi_opener)
		eat_pi_section();
}

void parser::__parse()
{
	get_sym();

	if (sym == pi_opener)  // ignore pi section
	{
		do {
			get_sym();
		} while (sym != pi_closer);

		get_sym();
	}

	__node();
}

void parser::__node()
{
	if (sym == tag_opener)
		start_tag();
	else
		error("tag opener ('<') expected");

	if (sym == empty_tag_closer)
		return;

	get_sym();

	while (sym == tag_opener)
	{
		node();
		get_sym();
	}

	if (sym == end_tag_opener)
		end_tag();
	else
		error("new node, or end-tag opener '</' expected");
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

#ifdef PRINT_OSM_TREE
		tabs += 1;
		cout << offset() 
			<< boost::format("%1%=%2%") % name_buf % text_buf << "\n";
		tabs -= 1;
#endif

		get_sym();
	}

	// end_tag
	if (sym != tag_closer && sym != empty_tag_closer)
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

void parser::error( boost::format const & msg )
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

void unrecoverable_error( boost::format const & msg )
{
	cout << "\nunrecoverable error: " << msg << std::endl;
	exit(1);
}

string parser::offset()
{
	string s;
	for (int i = 0; i < max(tabs, 0); ++i)
		s += ' ';
	return s;
}


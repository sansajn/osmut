// r5

/* recognized symbols: </, />, <?, ?>, text, name
\note pr tomto type parseru som stratil možnosť overiť validnosť vstupu 
pri parsovaní.

\note po pridaní node-u sa rýchlosť parseru znížila 4x (teraz je 2x pomalší 
ako libxml). Je to spôsobene vektorom (porozmýšlaj nad lazy attrib reading). */

#include <algorithm>
#include <utility>
#include <vector>
#include <cctype>
#include <fstream>
#include <string>
#include <iostream>
#include <boost/format.hpp>
#include <boost/utility.hpp>


//#define PRINT_OSM_TREE

using std::max;
using std::pair;
using std::vector;
using std::isalpha;
using std::string;
using std::ifstream;
using std::ios;
using std::cout;


template <int BufferSize = 4096>
class file_content : boost::noncopyable
{
public:
	file_content(char const * file_name);
	~file_content();
	bool is_open() const {return _fin.is_open();}
	char get();
	void getline(string & s);
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

template <int BufferSize>
file_content<BufferSize>::file_content(char const * file_name) 
	: _fin(file_name, ios::in|ios::binary), _bufidx(0) 
{
	alloc();
	fill();
}

template <int BufferSize>
file_content<BufferSize>::~file_content() 
{
	free();
	_fin.close();
}

template <int BufferSize>
char file_content<BufferSize>::get() 
{
	if (!eof() && _bufidx == size())
	{
		fill();
		_bufidx = 0;
	}
	return _buf[_bufidx++];
}

template <int BufferSize>
void file_content<BufferSize>::getline(string & s) 
{
	char ch = get();
	while (!eof() && ch != '\n')
	{
		s += ch;
		ch = get();
	}
}


class parser
{
public:
	typedef pair<string, string> keyval_t;

	enum e_node {start_node, end_node, empty_node};

	parser(char const * file_name);
	~parser() {}
	bool read();

	string const & node_name() const {return _curnode.name;}
	e_node node_type() const {return _curnode.type;}
	int attribute_count() const {return _curnode.attribs.size();}

	string const & attribute_name(int idx) const {
		return _curnode.attribs[idx].first;
	}
	
	string const & attribute_value(int idx) const {
		return _curnode.attribs[idx].second;
	}

	keyval_t const & attribute(int idx) const {
		return _curnode.attribs[idx];
	}

	// error handling
	bool good() const {return !_error;}
	boost::format const & error_message() const {return _errmsg;}

private:
	void node();
	void start_tag();
	void end_tag();

	void eat_pi_section();

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

	struct node_data
	{
		string name;
		e_node type;
		vector<keyval_t> attribs;  // spôsobuje 4-nasobné spomalenie parsingu
	};

	node_data _curnode;

	// low-level-parser
	e_symbol sym;
	char ch;
	char next_ch;
	string name_buf;
	string text_buf;

	file_content<4096> _osm;

	// error handling
	bool _error;
	boost::format _errmsg;

	// debug
	int tabs;
};  // parser




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
		if (p.node_type() == parser::start_node)
		{
			cout << "node:" << p.node_name() << "\n";
			for (int i = 0; i < p.attribute_count(); ++i)
			{
				cout << "  k:" << p.attribute_name() << ", v:"
					<< p.attribute_value() << "\n";
			}
		}
		continue;
	}
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


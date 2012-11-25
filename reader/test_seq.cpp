// sekvenčné čítanie
#include <fstream>
#include <iostream>
using std::ifstream;
using std::cout;
using std::endl;

void unrecoverable_error(char const * s);

int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error("input file parameter missing");

	ifstream fin(argv[1]);
	if (!fin.is_open())
		unrecoverable_error("can't open input file");

	while (fin)
	{
		volatile char ch;
		fin.get(ch);
	}

	fin.close();

	return 0;
}

void unrecoverable_error(char const * s)
{
	cout << s << endl;
	exit(1);
}

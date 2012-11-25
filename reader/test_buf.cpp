// baferovane čítanie
#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
using std::ifstream;
using std::cout;
using std::endl;
using std::ios;

void unrecoverable_error(char const * s);

int buf_size = 10*1024;
char * buf_alloc();
void buf_free(char *& buf);

int main(int argc, char * argv[])
{
	if (argc < 2)
		unrecoverable_error("input file parameter missing");

	ifstream fin(argv[1], ios::in|ios::binary);
	if (!fin.is_open())
		unrecoverable_error("can't open input file");

	if (argc > 2)
		buf_size = boost::lexical_cast<int>(argv[2]);

	cout << "buffer size:" << buf_size << " bytes\n";
	cout << "reading...";

	int reads = 1;

	char * buf = buf_alloc();

	fin.read(buf, buf_size);

	while (fin.gcount())
	{
		for (int i = 0; i < fin.gcount(); ++i)
			volatile char ch = buf[i];		

		fin.read(buf, buf_size);

		if (fin.gcount())
			++reads;
	}

	fin.close();

	buf_free(buf);

	cout << "\ndone!\n";
	cout << "number of reads: " << reads;

	return 0;
}


char * buf_alloc()
{
	return new char[buf_size];
}

void buf_free(char *& buf)
{
	delete [] buf;
	buf = nullptr;
}

void unrecoverable_error(char const * s)
{
	cout << s << endl;
	exit(1);
}


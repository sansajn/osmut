#include <iostream>
#include <fstream>
using std::cout;
using std::ifstream;


int main(int argc, char * argv[])
{
	ifstream fin(argv[1]);
	char buf[1024];
	fin.read(buf, 1024);
	if (fin.eof())
		cout << "eof reached\n";
	else
		cout << "eof not yet reached\n";
	cout << "red characters: " << fin.gcount() << "\n";

	if (!fin)
		cout << "stream bool operator() id also false\n";

	return 0;
}

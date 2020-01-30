#include "assembler.h"
#include <sstream>
#include <string.h>
#include <iomanip>

/*
 *
 */
int main(int argc, char** argv) {
	Assembler a;
	ifstream infile;
	//ofstream out;
	ofstream outfile(argv[2]);
	//outfile << "my text here!" << std::endl;
	//infile.open("input.txt");
	infile.open(argv[1]);
	if (!infile.is_open()) {
		cout << "error opening";
	}
	else {
		//out.open(argv[2]);
		a.FirstRun(infile);
		infile.clear();
		infile.seekg(0, infile.beg);
		//a.SecondRun(infile, out);
		a.SecondRun(infile, outfile);
		outfile.close();
		//out.close();
	}
	
	return 0;
}
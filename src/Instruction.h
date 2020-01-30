#pragma once
#include <iostream>

#include <regex>
using namespace std;

class Instruction {
public:
	Instruction(string hex, regex assembly);
	~Instruction();
	regex getAssembly_code();
	string getHexa_code();
private:
	string hexa_code;
	regex assembly_code;
};
#pragma once
#include <iostream>
using namespace std;
class SymbolTableEntry {
public:
	SymbolTableEntry(string name,string section,int value, char vis,string flags="");
	~SymbolTableEntry();
	SymbolTableEntry* next;
	string name;
	string section;
	string flags;
	int val;
	char visibility;
	int num;
	int section_size;
	static int id;
	

};
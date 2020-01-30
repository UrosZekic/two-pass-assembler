#pragma once
#include <iostream>
using namespace std;
class RelocationTableEntry {
public:
	RelocationTableEntry(int offset,string type,int num,string section);
	~RelocationTableEntry();
	int offset;
	string type;
	int num;
	string section;
	RelocationTableEntry* next;
};
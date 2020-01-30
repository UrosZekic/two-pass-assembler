#include "SymbolTableEntry.h"
int SymbolTableEntry::id = 0;

SymbolTableEntry::SymbolTableEntry(string name, string section, int value, char vis,string flags)
{
	num = id++;
	this->name = name;
	this->section = section;
	this->val = value;
	this->visibility = vis;
	this->flags = flags;
	this->section_size = 0;
	this->next = nullptr;

}
SymbolTableEntry::~SymbolTableEntry(){
}

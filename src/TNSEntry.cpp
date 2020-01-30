#include "TNSEntry.h"

TNSEntry::TNSEntry(string name, vector<string> expression)
{
	this->name = name;
	this->expression = expression;
	this->is_calc = false;
	this->next = nullptr;
}

TNSEntry::~TNSEntry()
{
}

#pragma once
#include <iostream>
#include <vector>
using namespace std;
class TNSEntry {
public:
	TNSEntry(string name, vector<string>expression);
	~TNSEntry();
	string name;
	vector<string> expression;
	TNSEntry* next;
	bool is_calc;
};
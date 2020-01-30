#pragma once
#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_
#include <iostream>
#include <fstream>
#include <regex>
#include "SymbolTableEntry.h"
#include "Instruction.h"
#include "RelocationTableEntry.h"
#include "TNSEntry.h"

using namespace std;
class Assembler {
public:
	Assembler();
	~Assembler();
	void FirstRun(ifstream& input);
	void SecondRun(ifstream& input, ofstream& output);
	vector<string> tokenize(const string line);
	int instr_size(vector<string>& tokens);
	
	int loc_cnt;
	string curr_section;
	regex regdir;
	regex regind;
	regex spec_regdir;
	regex spec_regind;
	regex immed_dec;
	regex immed_hex;
	regex memdir_dec;
	regex memdir_hex;
	regex instruction;
	regex label;
	regex symbol;
	regex directive;
	regex section;
	regex new_section;
	regex globl;
	regex two_op_instr;
	regex one_op_instr;
	regex zero_op_instr;
	regex jmp_instr;
	regex symb_expression;
	void FirstRunLabel(string label,char vis);
	void FirstRunDirective(vector<string> tokens);
	bool calculate_expression(vector<string> tokens);
	bool insert_into_TNS(vector<string> tokens);
	void FirstRunSection(vector<string> tokens);
	void FirstRunGlobl(vector<string> tokens);
	void iterate_through_TNS();
	void code_instr(vector<string> tokens, ofstream& output);
	string code_operands(vector<string> tokens);
	string code_jump_instr_pc_rel(int val,string section,char vis,int num,vector<string> tokens,int op_num);
	void code_directive(vector<string> tokens, ofstream& output);
	void turn_symbols_global(vector<string>tokens);
	void calc_byte_word(vector<string>tokens, ofstream& output);
	static SymbolTableEntry* symbolTable;
	static SymbolTableEntry* last;
	static RelocationTableEntry* relocationTable;
	static RelocationTableEntry* relocationLast;
	static TNSEntry* TNS;
	static TNSEntry* TNSlast;
	vector<Instruction> allInstructions;
	void createInstructions();
	void add_relocation_entry(int offset, string type, int num);
	vector<string> all_sections();
	SymbolTableEntry* find_symbol(string name);
};




#endif /* ASSEMBLER_H_ */
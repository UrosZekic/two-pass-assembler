#include "assembler.h"
#include <vector>
#include <string.h>
#include <cstdlib>
#include <sstream>
#include  <iomanip>

SymbolTableEntry* Assembler::symbolTable = nullptr;
SymbolTableEntry* Assembler::last = nullptr;
RelocationTableEntry* Assembler::relocationTable = nullptr;
RelocationTableEntry* Assembler::relocationLast = nullptr;
TNSEntry* Assembler::TNS = nullptr;
TNSEntry* Assembler::TNSlast = nullptr;
Assembler::Assembler() {
	regdir = regex("^r([0-7])(h|l)?$"); 
	spec_regdir = regex("^(pc|sp|psw)(h|l)?");
	regind = regex("^r([0-7])\\[");
	spec_regind = regex("^(pc|sp|psw)\\[");
	immed_dec = regex("^\\-?([0-9]+)$");
	immed_hex = regex("^(0x[0-9abcdef]+)|(0x[0-9ABCDEF]+)$");
	memdir_dec = regex("^\\*([0-9]+)$");
	//memdir_hex = regex("^\\*(0x[0-9abcdef]+)|(0x[0-9ABCDEF]+)$");
	memdir_hex = regex("^\\*(0x[0-9a-fA-F]+)$");
	instruction = regex("^(int|add|sub|mul|div|cmp|and|or|not|test|call|mov|shr|shl|halt|ret|iret|pop|push|xchg|jmp|jeq|jne|jgt)+(b|w)?$");
	symbol = regex("^(\\&|\\$)?([a-zA-Z_][a-zA-Z0-9]*)$");
	label = regex("^([a-zA-Z][a-zA-Z0-9]*):$");
	directive = regex("^\\.(word|byte|align|skip|equ)$");
	section = regex("^\\.(text|data|bss)$");
	new_section = regex("^\\.section$");
	globl = regex("^\\.(global|extern)$");
	two_op_instr = regex("^(add|sub|mul|div|cmp|and|or|test|mov|shr|shl|xchg)?(b|w)?$");
	one_op_instr = regex("^(int|not|push|pop)(b|w)?");
	zero_op_instr = regex("^(ret|iret|halt)");
	jmp_instr = regex("^(call|jmp|jeq|jne|jgt)$");
	symb_expression = regex("([a-zA-Z][a-zA-Z0-9]*|\\-?([0-9]+)|(0x[0-9abcdef]+)|(0x[0-9ABCDEF]+))(\\+|\\-)([a-zA-Z][a-zA-Z0-9]*|\\-?([0-9]+)|(0x[0-9abcdef]+)|(0x[0-9ABCDEF]+))");
	loc_cnt = 0;
	curr_section = "no_section";
	createInstructions();
}
Assembler::~Assembler() {
}
void Assembler::createInstructions() {
	allInstructions.push_back(Instruction("0x08", regex("^(halt)$")));
	allInstructions.push_back(Instruction("0xc8", regex("^(iret)$")));
	allInstructions.push_back(Instruction("0xc0", regex("^(ret)$")));
	allInstructions.push_back(Instruction("0x1c", regex("^(int)$")));
	allInstructions.push_back(Instruction("0x14", regex("^(xchg)(b|w)?$")));
	allInstructions.push_back(Instruction("0x24", regex("^(mov)(b|w)?$")));
	allInstructions.push_back(Instruction("0x2c", regex("^(add)(b|w)?$")));
	allInstructions.push_back(Instruction("0x34", regex("^(sub)(b|w)?$")));
	allInstructions.push_back(Instruction("0x3c", regex("^(mul)(b|w)?$")));
	allInstructions.push_back(Instruction("0x44", regex("^(div)(b|w)?$")));
	allInstructions.push_back(Instruction("0x4c", regex("^(cmp)(b|w)?$")));
	allInstructions.push_back(Instruction("0x54", regex("^(not)(b|w)?$")));
	allInstructions.push_back(Instruction("0x5c", regex("^(and)(b|w)?$")));
	allInstructions.push_back(Instruction("0x64", regex("^(or)(b|w)?$")));
	allInstructions.push_back(Instruction("0x6c", regex("^(xor)(b|w)?$")));
	allInstructions.push_back(Instruction("0x74", regex("^(test)(b|w)?$")));
	allInstructions.push_back(Instruction("0x7c", regex("^(shl)(b|w)?$")));
	allInstructions.push_back(Instruction("0x84", regex("^(shr)(b|w)?$")));
	allInstructions.push_back(Instruction("0x8c", regex("^(push)(b|w)?$")));
	allInstructions.push_back(Instruction("0x94", regex("^(pop)(b|w)?$")));
	allInstructions.push_back(Instruction("0x9c", regex("^(jmp)(b|w)?$")));
	allInstructions.push_back(Instruction("0xa4", regex("^(jeq)(b|w)?$")));
	allInstructions.push_back(Instruction("0xac", regex("^(jne)(b|w)?$")));
	allInstructions.push_back(Instruction("0xb4", regex("^(jgt)(b|w)?$")));
	allInstructions.push_back(Instruction("0xbc", regex("^(call)(b|w)?$")));
}

vector<string> Assembler::tokenize(const string line)
{
	vector<string> tokens;
	std::size_t prev = 0, pos;
	while ((pos = line.find_first_of(" '\t''\r\n',", prev)) != std::string::npos)
	{
		if (pos > prev)
			tokens.push_back(line.substr(prev, pos - prev));
		prev = pos + 1;
	}
	if (prev < line.length())
		tokens.push_back(line.substr(prev, std::string::npos));
	return tokens;
}

int Assembler::instr_size(vector<string>& tokens)
{	
	int ret = 1;
	// ako ima b ili w onda mozda moze velicina ali fali bajt opisa
	int check = tokens[0].find_last_of("'b'");//bilo je i b i w
	for (int i = 1; i < tokens.size(); i++) {
		if (regex_match(tokens[i], regdir) or regex_match(tokens[i],spec_regdir)) {
			ret += 1;
			continue;
		}
		if (regex_search(tokens[i], regind) or regex_search(tokens[i],spec_regind)) {// search umesto match
			// 1 bajt za registar, izracunati koliki je pomeraj
			//pretpostavka- regind bez pomeraja se pise [0]
			ret += 1;
			int left_edge = tokens[i].find_first_of("[");
			int right_edge = tokens[i].length();
			string pomeraj = tokens[i].substr(left_edge + 1, right_edge - 2);
			//iz nekog razloga ostavlja ] u pomeraju
			pomeraj.pop_back();
			signed int skok;//ne znam bolje ime, i signed dodat da li treba tako?
			if (regex_match(pomeraj, symbol)) 
			{	
				ret += 2;
				continue;
			}
			if (regex_match(pomeraj, immed_dec)) {
				skok = stoi(pomeraj);
				if (skok == 0) ret += 0;//bez pomeraja
				else if (skok<127 and skok>-128) ret += 1;//1b pomeraj
				else ret += 2;// 2b pomeraj
				continue;
			}
			if (regex_match(pomeraj, immed_hex)) {
				string val = pomeraj.substr(2);
				if (val == "0" or val == "00" or val == "000" or val == "0000")
				{
					ret += 0;
					continue;
				}
				if (val.length() <= 2) {
					ret += 1;
					continue;
				}
				if (val.length() > 2 and val.length() < 5) {
					ret += 2;//provera da li je veci do 2b
					continue;
				}
				else { 
					cout << "ERROR PREVELIKI BROJ"; 
					std::exit(-1);
				}
			}
			
			
			}
			if (regex_match(tokens[i], immed_dec)) {
				if (i == 1 and tokens[0] != "int" and tokens[0] != "cmp" and tokens[0] != "push")
				{
					cout << "error ne moze immed da bude dst" << endl;
					std::exit(-1);
				}
				if (tokens[0] == "xchg" and i == 2)
				{
					cout << "error ne moze sa xchg immed" << endl;
					std::exit(-1);
				}
				if (tokens.size() > 2 and i == 1) cout << "DST ne moze biti immed" << endl;
				ret += 1;//bajt opisa
				if (check == tokens[0].length() - 1 and tokens[0].length() > 3 and tokens[0][check] == 'b')
				{
					int val = stoi(tokens[i]);
					if (val > 127 or val < -128)
					{
						cout << "operand je preveliki error" << endl;
						std::exit(-1);
					}
					ret += 1;
				}
				else ret += 2;
				continue;
			}
			if (regex_match(tokens[i], immed_hex)) {
				if (i == 1 and tokens[0] != "int" and tokens[0] != "cmp" and tokens[0] != "push")
				{
					cout << "error ne moze immed da bude dst" << endl;
					std::exit(-1);
				}
				if (tokens[0] == "xchg" and i == 2)
				{
					cout << "error ne moze sa xchg immed" << endl;
					std::exit(-1);
				}
				if (tokens.size() > 2 and i == 1) cout << "DST ne moze biti immed" << endl;
				ret += 1;
				if (check == tokens[0].length() - 1 and tokens[0].length() > 3 and tokens[0][check] == 'b')
				{
					if (tokens[i].size() > 4) {
						cout << "operand je preveliki error" << endl; 
						std::exit(-1);
					}
					ret += 1;
				}
				else ret += 2;
				continue;
			}
			if (regex_match(tokens[i], memdir_hex)) {
				ret += 1;//bajt opisa
				ret += 2;
				continue;
			}
			if (regex_match(tokens[i], memdir_dec)) {
				ret += 1;//bajt opisa
				ret += 2;
				continue;
			}
			if (regex_match(tokens[i], symbol)) {
				if (tokens[i].find('&') != -1 and i == 1 and tokens[0] != "int" and tokens[0] != "cmp" and tokens[0] != "push")
				{
					cout << " error ne moze immed";std::exit(-1);
				}
				if (tokens[0] == "xchg" and i == 2)
				{
					cout << "error ne moze sa xchg immed" << endl; std::exit(-1);
				}
				ret += 1;//bajt opisa
				ret += 2;
				continue;
			}
			cout << "operand nije prepoznat error" << endl;
			std::exit(-1);
		}
		return ret;

	}

void Assembler::FirstRunLabel(string label,char vis)
{	
	for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
		if (temp->name == label)
		{
			cout << "error ne mogu 2 simbola istog imena" << endl;
			std::exit(-1);
		}
	}
	last->next = new SymbolTableEntry(label, curr_section, loc_cnt, vis);
	last = last->next;
}
void Assembler::FirstRunDirective(vector<string> tokens)
{	
	if ((tokens[0] == ".byte" or tokens[0] == ".word")) {
		if (curr_section == "no_section") {
			cout << "ne mogu byte ili word direktive tu" << endl;
			std::exit(-1);
		}
		
	}
	if (tokens[0] == ".byte" or tokens[0] == ".word") {
		for (int i = 2; i < tokens.size(); i++) {
			if (tokens[i] == "+" or tokens[i] == "-") {
				string expr = tokens[i - 1] + tokens[i] + tokens[i + 1];
				std::vector<string>::iterator it=tokens.begin();
				tokens.insert(it+(i-1), expr);
				it = tokens.begin();
				tokens.erase(it + (i+2));
				it = tokens.begin();
				tokens.erase(it + (i+1));
				it = tokens.begin();
				tokens.erase(it + (i));
			}
		}
	}
	if (tokens[0] == ".equ" and regex_match(tokens[2],symb_expression)) {
		string first = tokens[2].substr(0, 1);
		string op = tokens[2].substr(1, 1);
		string second = tokens[2].substr(2);
		tokens.pop_back();
		tokens.push_back(first);
		tokens.push_back(op);
		tokens.push_back(second);
	}
	
	if (tokens[0] == ".skip") {
		int to_skip;
		if (tokens[1].find_last_of('x') != -1)
			to_skip = stoi(tokens[1], 0, 16);
		else
			to_skip = stoi(tokens[1]);
		loc_cnt += to_skip;
	}
		
	if (tokens[0] == ".byte")
		loc_cnt += tokens.size()-1;
	if (tokens[0] == ".word")
		loc_cnt += (tokens.size()-1)*2;
	if (tokens[0] == ".align") {
		int alignment;
		if (tokens[1].find_last_of('x') != -1)
			alignment = stoi(tokens[1], 0, 16);
		else
			alignment = stoi(tokens[1]);
		if (alignment % 2 != 0) {
			cout << "ERROR mora biti stepen dvojke";
			std::exit(-1);
		}
		while (loc_cnt % alignment != 0)
			loc_cnt++;
	}
	if (tokens[0] == ".equ") {
		SymbolTableEntry* temp = find_symbol(tokens[1]);
		if (temp != nullptr) {
			if (temp->section != "abs") {
				cout << "ne mogu 2 simbola istog imena-error";
				std::exit(-1);
			}
			if (temp->section == "abs") {
				SymbolTableEntry* prev = symbolTable;
				for (; prev->next != temp; prev = prev->next);
				prev->next = temp->next;
				delete temp;
			}
		}
		// neizracunljivi izrazi
		bool is_expr = false;

		if (tokens.size() > 3)
			is_expr = true;

		if (!is_expr) {
			if (regex_match(tokens[2], immed_dec) or regex_match(tokens[2], immed_hex))
			{
				/*unsigned char convert = std::stoi(tokens[2], nullptr, 16);
				char convert2 = *reinterpret_cast<char*>(&convert);
				int val = static_cast<int>(convert2);*/
				int val;
				if (tokens[2].find_last_of('x') != -1)
					val = stoi(tokens[2], 0, 16);
				else
					val = stoi(tokens[2]);
				last->next = new SymbolTableEntry(tokens[1], "abs", val, 'l');
				last = last->next;
			}
			if (regex_match(tokens[2], symbol)) {
				int val;
				bool found = false;
				for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
					if (temp->name == tokens[2])
					{
						val = temp->val;
						found = true;
					}
				}
				if (found) { // simbol postoji u tabeli
					last->next = new SymbolTableEntry(tokens[1], "abs", val, 'l');// sekcija koja li treba?
					last = last->next;
				}
				else { // nema simbola,TNS
					insert_into_TNS(tokens);
				}
			}
		}
		else {
			calculate_expression(tokens);
		}
	
	}
}
bool Assembler::calculate_expression(vector<string> tokens) {
	if (tokens.size() < 4) {// ovo je za .equ a,b npr.
		bool found = false;
		SymbolTableEntry* temp = find_symbol(tokens[2]);
		if (temp != nullptr)
			found = true;
		if (found) {
			last->next = new SymbolTableEntry(tokens[1], "abs", temp->val, 'l');//sekcija?
			last = last->next;
			return true;
		}
		else return found;
	}
	int i = 0;
	for (; i < tokens.size(); i++) { // onaj pre i posle su mi operandi
		if (tokens[i] == "+" or tokens[i] == "-")
			break;
	}
	int op1, op2;
	string section1 = "";
	string section2="";
	if (regex_match(tokens[i - 1], immed_dec) or regex_match(tokens[i - 1], immed_hex))
	{
		if (tokens[i-1].find_last_of('x') != -1)
			op1 = stoi(tokens[i-1], 0, 16);
		else
			op1 = stoi(tokens[i-1]);
	}
	else {
		bool found = false;
		SymbolTableEntry* temp = symbolTable;
		for (; temp != nullptr; temp = temp->next) {
			if (temp->name == tokens[i - 1]) {
				found = true;
				section1 = temp->section;
				break;
			}
		}
		if (found) op1 = temp->val;
		else {
			// nema simbola,TNS
			bool ret =insert_into_TNS(tokens);
			return ret;
		}
	}
	if (regex_match(tokens[i + 1], immed_dec) or regex_match(tokens[i + 1], immed_hex))
	{
		if (tokens[i+1].find_last_of('x') != -1)
			op2 = stoi(tokens[i+1], 0, 16);
		else
			op2 = stoi(tokens[i+1]);
	}
	else {
		bool found = false;
		SymbolTableEntry* temp = symbolTable;
		for (; temp != nullptr; temp = temp->next) {
			if (temp->name == tokens[i + 1]) {
				found = true;
				section2 = temp->section;
				break;
			}
		}
		if (found) op2 = temp->val;
		else {
			// nema simbola,TNS
			bool ret=insert_into_TNS(tokens);
			return ret;
		}
	}
	if (section1 != "" and section2 != "" and section1 != "abs" and section2 != "abs" and section1 != section2) {
		cout << "ne mogu simboli iz razl.sekcija";
		std::exit(-1);
	}
	if (section1 != "" and section2 != "" and section1 != "abs" and section2 != "abs" and section1 == section2 and tokens[i] == "+") {
		cout << "ne moze sabiranje iz iste sekcije";
		std::exit(-1);
	}
	int sim_val;
	if (tokens[i] == "+") sim_val = op1 + op2;
	else sim_val = op1 - op2;
	last->next = new SymbolTableEntry(tokens[1], "abs", sim_val, 'l');
	last = last->next;
	return true;
}
bool Assembler::insert_into_TNS(vector<string> tokens) {// true-unet false-vec postoji
	for (TNSEntry* temp = TNS; temp != nullptr; temp = temp->next) {
		if (temp->name == tokens[1])
		{
			temp->expression = tokens;
			return true;
		}
	}
	
	if (TNS == nullptr) {
		TNS = new TNSEntry(tokens[1], tokens);
		TNSlast = TNS;
		return true;
	}
	else {
		TNSlast->next = new TNSEntry(tokens[1], tokens);
		TNSlast = TNSlast->next;
		return true;
	}
}
void Assembler::FirstRunSection(vector<string> tokens) {
	for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
		if (temp->name == curr_section)
			temp->section_size = loc_cnt;
	}
	loc_cnt = 0;
	if (tokens[0] == ".text" or tokens[0] == ".data" or tokens[0] == ".bss") {
		SymbolTableEntry* temp = find_symbol(tokens[0].substr(1));// da li moze da se ponovo otpocne sekcija
		//curr_section = tokens[0].substr(1);
		curr_section = tokens[0];
		string fl = "";
		if (tokens[0] == ".text")
			fl = "rwx";
		if (tokens[0] == ".data")
			fl = "rw";
		if (tokens[0] == ".bss")
			fl = "r";
		last->next = new SymbolTableEntry(curr_section, curr_section, 0, 'l',fl);
		last = last->next;
	}
	else {
		if (tokens[1] == "text" or tokens[1] == ".text" or tokens[1] == "data" or tokens[1] == ".data" or tokens[1] == "bss" or tokens[1] == ".bss") {
			curr_section = tokens[0];
			string fl = "";
			if (tokens[0] == "text" or tokens[1] == ".text")
				fl = "rwx";
 			if (tokens[0] == "data" or tokens[1] == ".data")
				fl = "rw";
			if (tokens[0] == "bss" or tokens[1] == ".bss")
				fl = "r";
			last->next = new SymbolTableEntry(curr_section, curr_section, 0, 'l', fl);
			last = last->next;
			return;
		}
		curr_section = tokens[1];
		if (tokens.size() > 2) {
			last->next = new SymbolTableEntry(tokens[1], curr_section, 0, 'l', tokens[2]);
			last = last->next;
		}
		else {
			//podrazumevani flegovi rwx
			last->next = new SymbolTableEntry(tokens[1], curr_section, 0, 'l',"rwx");
			last = last->next;
		}
	}
}
void Assembler::FirstRunGlobl(vector<string> tokens) {
	if (tokens[0] == ".global") return;
	if (tokens[0] == ".extern") {
		for (int i = 1; i < tokens.size(); i++) {
			last->next = new SymbolTableEntry(tokens[i], "und", 0, 'g');
			last = last->next;
		}
	}
}
void Assembler::iterate_through_TNS() {
	int size_of_table = 0;
	int num_solved = 0;
	int solved_in_this_iteration = 0;
	for (TNSEntry* temp = TNS; temp != nullptr; temp = temp->next)
		size_of_table++;
	for (TNSEntry* temp = TNS; temp != nullptr;) {
		if (temp->is_calc) {
			if (temp == TNSlast) temp = TNS;
			else temp = temp->next;
			continue;
		}
		bool check = calculate_expression(temp->expression);
		if (check) {
			temp->is_calc = true;
			num_solved++;
			solved_in_this_iteration++;
		}
		if (temp == TNSlast and solved_in_this_iteration == 0) {
			cout << "nemoguce izracunati" << endl;
			std::exit(-1);
		}
		if (temp == TNSlast) 
		{ temp = TNS; 
		solved_in_this_iteration = 0; }
		else temp = temp->next;
		if (num_solved == size_of_table)
			break;//sve smo razresili, da li obrisati tabelu?
		
	}
}
void Assembler::FirstRun(ifstream& input) {
	string line;
	Assembler::symbolTable = new SymbolTableEntry("und","und",0,'l');
	last = symbolTable;
	while (getline(input, line))
	{
		vector<string> tokens= tokenize(line);
		
		
		if (tokens.size() == 0)
			continue;
		if (tokens[0]==".end")
		{
			
			break;
		}
		/*if (!regex_match(tokens[0], label) and !regex_match(tokens[0], globl) and tokens[0]!=".section" and !(regex_match(tokens[0],section)) and !regex_match(tokens[0], instruction) and !regex_match(tokens[0], directive)) {
			cout << "NO MATCH-ERROR";
			break;
		}*/
		
		if (regex_match(tokens[0], label)) {
			
			tokens[0].pop_back();//problem?
			FirstRunLabel(tokens[0], 'l');
			tokens.erase(tokens.begin());
			if (tokens.size() == 0)
				continue;
		}
		if (regex_match(tokens[0], globl)) {
			for (int i = 1; i < tokens.size(); i++) {
				if (regex_match(tokens[i], instruction) or regex_match(tokens[i], directive) or regex_match(tokens[i], globl) or regex_match(tokens[i], section) or tokens[i] == ".section")
					cout << "1 direktiva/naredba u 1 liniji" << endl;
				
			}
			FirstRunGlobl(tokens);
			continue;
		}
		if (regex_match(tokens[0], section) or tokens[0]==".section") {
			for (int i = 1; i < tokens.size(); i++) {
				if (regex_match(tokens[i], instruction) or regex_match(tokens[i], directive) or regex_match(tokens[i], globl) or regex_match(tokens[i], section) or tokens[i] == ".section")
					cout << "1 direktiva/naredba u 1 liniji" << endl;
			}
			FirstRunSection(tokens);
			continue;
		}
		
		if (regex_match(tokens[0], instruction)) {
			if (curr_section == "no_section") {
				cout << "ne mogu instr tu" << endl;
				std::exit(-1);
				}
			SymbolTableEntry* temp = find_symbol(curr_section);
			if (temp->flags.find_last_of('x') == -1) {
				cout << "ne mogu instr tu" << endl;
				std::exit(-1);
			}
			for (int i = 1; i < tokens.size(); i++) {
				if (regex_match(tokens[i], instruction) or regex_match(tokens[i], directive) or regex_match(tokens[i], globl) or regex_match(tokens[i], section) or tokens[i] == ".section")
					cout << "1 direktiva/naredba u 1 liniji" << endl;
			}
			if (tokens.size() > 3) {
				cout << "previse parametara"; std::exit(-1);
			}
			if (regex_match(tokens[0], two_op_instr) and tokens.size() < 3) {
				cout << "premalo parametara"; std::exit(-1);
			}
			if (regex_match(tokens[0], one_op_instr) and tokens.size() > 2) {
				cout << "previse parametara"; std::exit(-1);
			}
			if (regex_match(tokens[0], jmp_instr) and tokens.size() > 2) {
				cout << "previse parametara";
				std::exit(-1);
			}
			if (regex_match(tokens[0], zero_op_instr) and tokens.size() > 1) {
				cout << "previse parametara"; std::exit(-1);
			}
			loc_cnt += instr_size(tokens);	
			continue;
		}
		if (regex_match(tokens[0], directive)) {
			for (int i = 1; i < tokens.size(); i++) {
				if (regex_match(tokens[i], instruction) or regex_match(tokens[i], directive) or regex_match(tokens[i], globl) or regex_match(tokens[i], section) or tokens[i] == ".section")
					cout << "1 direktiva/naredba u 1 liniji" << endl;	
			}
			if (tokens.size() < 2) {
				cout << "premalo parametara"; std::exit(-1);
			}
			
			if ((tokens[0] == ".skip" or tokens[0] == ".align") and regex_match(tokens[1], symbol)) {
				cout << "ne smeju simboli sa skip i align"; std::exit(-1);
			}
			if (tokens[0] == ".equ" and tokens.size() < 3) {
				cout << "premalo parametara"; std::exit(-1);
			}
			if (tokens[0] == ".equ" and tokens.size() > 5) {
				cout << "previse parametara"; std::exit(-1);
			}
			FirstRunDirective(tokens);
			continue;
		}
		
		

	}
	cout << "prvi prolaz gotov" << endl;
	iterate_through_TNS();
}
SymbolTableEntry* Assembler::find_symbol(string name) {
	SymbolTableEntry* ret = symbolTable;
	for (; ret != nullptr; ret = ret->next) {
		if (ret->name == name)
			break;
	}
	return ret;
}
void Assembler::add_relocation_entry(int offset, string type, int num) {
	if (relocationTable == nullptr) {
		relocationTable = new RelocationTableEntry(offset, type, num,curr_section);
		relocationLast = relocationTable;
	}
	else {
		relocationLast->next = new RelocationTableEntry(offset, type, num,curr_section);
		relocationLast = relocationLast->next;
	}
}
void Assembler::code_instr(vector<string> tokens, ofstream& output) {
	for (int i = 0; i < allInstructions.size(); i++) {
		if (regex_match(tokens[0], allInstructions[i].getAssembly_code())) {
			string instr = allInstructions[i].getHexa_code();
			int last_char = tokens[0].size() - 1;
			//setovanje onog bita za vel.operanada
			if (tokens[0][last_char] == 'b' and instr[3] == '4' and tokens[0].length()>3)
				instr[3] = '0';
			if (tokens[0][last_char] == 'w' and instr[3] == 'c' and tokens[0].length() > 3)
				instr[3] = '8';
			output << instr << " ";
			break;
		}
	}
	output << code_operands(tokens) << endl;
}
string Assembler::code_operands(vector<string> tokens) {
	string operands="";
	for (int i = 1; i < tokens.size(); i++) {
		if (regex_match(tokens[i], regdir) or regex_match(tokens[i],spec_regdir)) {
			int code;
			int reg_num;
			if (regex_match(tokens[i], regdir))
				reg_num = tokens[i][1]-'0';
			else {
				if (tokens[i] == "sp")
					reg_num = 6;
				if (tokens[i] == "pc")
					reg_num = 7;
				if (tokens[i] == "psw")
					reg_num = 15;
			}
			if (tokens[i][tokens[i].length() - 1] == 'h')
			{
				code = 0x21;
				//reg_num = tokens[i][tokens[i].length() - 2] -'0';
			}
			else 
			{ 
				code = 0x20;
				//reg_num = tokens[i][tokens[i].length() - 1] - '0';
			}
			
			reg_num = reg_num << 1;
			code = code | reg_num;
			//da bi dobili hexa string
			stringstream stream;
			stream << "0x" << hex << code;
			operands += stream.str() + " ";
			continue;
			
			
		}
		if (regex_match(tokens[i], immed_hex)) {
			operands += "0x00 ";//immed code is 00
			string val = tokens[i].substr(2);
			int check = tokens[0].find_last_of("'b'");//bilo je i b i w
			if (check == tokens[0].length() - 1 and tokens[0].length() > 3 and tokens[0][check] == 'b') {
				if (val.length() == 1) operands += "0x0" + val;
				else operands += "0x" + val;
			}
			else {
				if (val.length() == 1) 
					operands += "0x0" + val + " 0x00 ";
				if (val.length() == 2 and (val[0] - '0' < 8)) 
					operands += "0x"+ val + " 0x00 ";
				if (val.length() == 2 and (val[0] - '0' >= 8)) 
					operands += "0x" + val + " 0xff ";
				/*if (val.length() == 3 and (val[0] - '0' < 8)) 
					operands += "0x" + val.substr(1)+" 0x0"+val.substr(0,1)+ " ";
				if (val.length() == 3 and (val[0] - '0' >= 8)) 
					operands += "0x" + val.substr(1) + " 0xf" + val.substr(0, 1)+ " ";*/
				if (val.length()==3)
					operands += "0x" + val.substr(1) + " 0x0" + val.substr(0, 1) + " ";
				if (val.length() == 4) 
					operands += "0x" + val.substr(2) + " 0x" + val.substr(0, 2);
			}
			continue;
		}
		if (regex_match(tokens[i], immed_dec)) {
			operands += "0x00 ";//immed code is 00
			int dec = stoi(tokens[i]);
			stringstream stream;
			char fill;
			if (dec < 0) fill = 'f';
			else fill = '0';
			int check = tokens[0].find_last_of("'b'");//bilo je i b i w
			if (check == tokens[0].length() - 1 and tokens[0].length() > 3 and tokens[0][check] == 'b') {
				stream << "0x" << setfill(fill) << setw(2) << hex << dec;
				operands += stream.str() + " ";
			}
			else {
				stream << setfill(fill) << setw(4) << hex << dec;
				if (dec >= 0) {
					operands += "0x" + stream.str().substr(2) + " ";
					operands += "0x" + stream.str().substr(0, 2) + " ";
				}
				else
					operands += "0x" + stream.str().substr(6) + " 0x" + stream.str().substr(4, 2);
			}
			continue;
		}
		if (regex_match(tokens[i], memdir_hex)) {
			operands += "0xa0 ";//memdir code is A0
			operands += "0x" + tokens[i].substr(5) + " ";
			operands += tokens[i].substr(1, 4);
			continue;
		}
		if (regex_match(tokens[i], memdir_dec)) {
			int mem = stoi(tokens[i].substr(1));// sta sa negativnim brojevima
			operands += "0xa0 ";//memdir code is A0
			stringstream stream;
			stream << setfill('0') << setw(4) << hex << mem;
			operands += "0x" + stream.str().substr(2) + " 0x" + stream.str().substr(0, 2) + " ";
			continue;
		}
		if (regex_search(tokens[i], regind) or regex_search(tokens[i],spec_regind)) { // search umesto match
			int reg_num;
			if (regex_search(tokens[i], regind))
				reg_num = tokens[i][1] - '0';
			else {
				if (tokens[i].find("sp")!= std::string::npos)
					reg_num = 6;
				if (tokens[i].find("pc") != std::string::npos) 
					reg_num = 7;
				if (tokens[i].find("psw") != std::string::npos)
					reg_num = 15;
			}
			
			reg_num = reg_num << 1;
			int code;
			
			int left_edge = tokens[i].find_first_of("[");
			int right_edge = tokens[i].length();
			string pomeraj = tokens[i].substr(left_edge + 1, right_edge - 2);
			//iz nekog razloga ostavlja ] u pomeraju
			pomeraj.pop_back();
			signed int skok;//ne znam bolje ime
			if (regex_match(pomeraj, symbol))
			{
				int num;
				int skok;
				char vis;
				string sec;
				/*for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
					if (temp->name == pomeraj)
					{	// nadajmo se da je simbol u tabeli,ako nije....
						 skok= temp->val;
						
						vis = temp->visibility;
						sec = temp->section;
						num = temp->num;
						break;
					}
				}*/
				SymbolTableEntry* temp = find_symbol(pomeraj);
				if (temp == nullptr) {
					cout << "nedefinisan simbol" << endl;
					std::exit(-1);
				}
				else {
					skok = temp->val;
					vis = temp->visibility;
					sec = temp->section;
					num = temp->num;
				}
				//2b skok
				code = 0x80;
				code = code | reg_num;
				string reloc_type = "R_386_16";
				/*if (tokens[i].find("pc") != std::string::npos or reg_num == 14)// posle shiftovanja 7 postalo 14
					reloc_type = "R_386_PC16";
				else
					reloc_type = "R_386_16";*/
				stringstream stream;
				stream << "0x" << hex << code;
				operands += stream.str() + " ";//op descr byte
				int ofs;
				if (tokens.size() == 2 or i == 2) ofs = loc_cnt - 2;
				else ofs = loc_cnt - instr_size(tokens) + 2; // promenio +1 u +2
				if (vis == 'g') {
					add_relocation_entry(ofs, reloc_type, num);
					operands += "0x00 0x00 ";
				}
				else {
					SymbolTableEntry* tempsect = find_symbol(sec);
					num = tempsect->num;
					add_relocation_entry(ofs, reloc_type, num);
					stream.str(std::string());
					stream << setfill('0') << setw(4) << hex << skok;
					operands += "0x" + stream.str().substr(2,2) + " 0x" + stream.str().substr(0, 2)+ " ";
				}
				continue;

			}
			if (regex_match(pomeraj, immed_dec)) {
				skok = stoi(pomeraj);
			}
			if (regex_match(pomeraj, immed_hex)) {
				string s = pomeraj.substr(2);
				if (s.size() > 2) {
					char* p;
					short pom = strtoul(s.c_str(), &p, 16);//short signed char
					skok = pom;
				}
				else {
					char* p;
					signed char pom = strtoul(s.c_str(), &p, 16);//short signed char
					skok = pom;
				}
			}
			if (skok == 0) {//bez pomeraja
				code = 0x40;
				code = code | reg_num;
				stringstream stream;
				stream << "0x" << hex << code;
				operands += stream.str() + " ";//op descr byte
			}
			
			else if (skok<=127 and skok>=-128) {//1b pomeraj
				code = 0x60;
				code = code | reg_num;
				stringstream stream;
				stream << "0x"  << hex << code;
				operands += stream.str() + " ";//op descr byte
				stream.str(std::string());
				if (skok > 0) {
					stream << "0x" << setfill('0') << setw(2) << hex << skok;
					operands += stream.str() + " ";
				}
				else {
					stream << setfill('f') << setw(2) << hex << skok;
					operands += "0x" + stream.str().substr(6) + " ";
				}
			}
			else { // 2b pomeraj
				code = 0x80;
				code = code | reg_num;
				stringstream stream;
				stream << "0x" << hex << code;
				operands += stream.str() + " ";//op descr byte
				stream.str(std::string());
				if (skok > 0) {
					stream << "0x" << setfill('0') << setw(4) << hex << skok;
					operands += "0x"+stream.str().substr(4) + " " + stream.str().substr(0,4)+ " ";
				}
				else {
					stream << setfill('f') << setw(4) << hex << skok;
					operands += "0x" + stream.str().substr(6) + " 0x"+stream.str().substr(4,2)+ " ";
				}
				
			}
			continue;
		}
		if (regex_match(tokens[i], symbol)) {
			int value;
			char vis;
			string sec;
			int num;
			string tok_name;
			if (tokens[i].find('$') != -1 or tokens[i].find('&') != -1)
				tok_name = tokens[i].substr(1);
			else
				tok_name = tokens[i];
			SymbolTableEntry* temp = find_symbol(tok_name);
			if (temp != nullptr) {
				value = temp->val;
				vis = temp->visibility;
				sec = temp->section;
				num = temp->num;
			}
			else
			{
				cout << "greska,nedefinisan simbol" << endl;
				std::exit(-1);
			}
			stringstream stream;
			if (tokens[i].find('$')!=-1)
			{
				operands += code_jump_instr_pc_rel(value,sec,vis,num,tokens,i);
				//return operands;
				continue;
			}
			else {
				if (tokens[i].find('&') != -1)
					operands += "0x00 ";//immed code
				else
					operands += "0xa0 ";// memdir

				if (vis == 'g') {
					int ofs;
					if (tokens.size() == 2 or i == 2) ofs = loc_cnt - 2;
					else ofs = loc_cnt - instr_size(tokens) + 2; // promenio +1 u +2
					add_relocation_entry(ofs, "R_386_16", num);
					operands += "0x00 0x00 ";
				}
				else {
					int ofs;
					if (tokens.size() == 2 or i == 2) ofs = loc_cnt - 2;
					else ofs = loc_cnt - instr_size(tokens) + 2;//+1 u +2
					for (SymbolTableEntry* section = symbolTable; section != nullptr; section = section->next) {
						if (section->name == sec) {
							num = section->num;
							break;
						}
					}
					if (sec != "abs") {
						add_relocation_entry(ofs, "R_386_16", num);
					}
					stringstream stream;
					stream << "0x" << setfill('0') << setw(4) << hex << value;
					operands += "0x" + stream.str().substr(4) + " " + stream.str().substr(0, 4) + " ";
				}
			}
		}
	}
 	return operands;
}
string Assembler::code_jump_instr_pc_rel(int val, string section, char vis, int num,vector<string> tokens,int op_num) {
	string ret = "0x8e "; // kod za pc rel simbol
	if (section == curr_section and vis=='l') {
		//int to_write = loc_cnt - val;
		int to_write = val-loc_cnt;
		stringstream stream;
		if (to_write > 0) { // u istoj sekciji ne treba relok
			stream << setfill('0') << setw(4) << hex << to_write;
			ret += "0x" + stream.str().substr(2) + " 0x" + stream.str().substr(0, 2) + " ";
		}
		else {
			stream << hex << to_write;
			ret += "0x" + stream.str().substr(6) + " 0x" + stream.str().substr(4, 2)+ " ";
		}
		return ret;
	}
	int ofset;
	if (op_num==1) 
		ofset = loc_cnt - instr_size(tokens) + 2; //1b opis instr,1b opis operanda
	else {
		ofset = loc_cnt - 2;
	}
	if (vis == 'g') {
		int to_write = 0 - (loc_cnt - ofset);
		add_relocation_entry(ofset, "R_386_PC16", num);
		stringstream stream;
		stream << hex << to_write;
		ret += "0x"+ stream.str().substr(6)+" 0x"+stream.str().substr(4,2) + " ";//treba srediti ispis,(4,2) od 4. citaj 2 chara
	} 
	else {
		int to_sub = loc_cnt - ofset;
		int to_write = val - to_sub;
		stringstream stream;
		if (to_write<0){
		stream << hex << to_write;
		ret += "0x" + stream.str().substr(6) + " 0x" + stream.str().substr(4, 2) + " ";//treba srediti ispis
		}
		else {
			stream <<  setfill('0') << setw(4) << hex << to_write;
			ret += "0x" + stream.str().substr(2) + " 0x" +stream.str().substr(0,2) + " ";// 4 ispise kao 0x04 0x00 , to je valjda ok
		
		}
		int sec_num;
		for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
			if (temp->name == section) {
				sec_num = temp->num;
				break;
			}
		}
		add_relocation_entry(ofset, "R_386_PC16", sec_num);
	}
	return ret;
}
void Assembler::calc_byte_word(vector<string>tokens, ofstream& output) {
	int i = 1;
	for (; i < tokens.size(); i++) {
		if (regex_match(tokens[i], symb_expression))
			break;
	}
	string first_op = tokens[i].substr(0, 1);
	string operation = tokens[i].substr(1, 1);
	string second_op = tokens[i].substr(2);
	int op1, op2;
	string section1 = "";
	string section2 = "";
	if (regex_match(first_op, immed_dec) or regex_match(first_op, immed_hex))
	{
		if (first_op.find_last_of('x') != -1)
			op1 = stoi(first_op, 0, 16);
		else
			op1 = stoi(first_op);
	}
	else {
		bool found = false;
		SymbolTableEntry* temp = find_symbol(first_op);
		if (temp != nullptr) {
			found = true;
			section1 = temp->section;
		}
		if (found) op1 = temp->val;
		else {
			cout << "nedefinisan simbol";
			std::exit(-1);
		}
	}
	if (regex_match(second_op, immed_dec) or regex_match(second_op, immed_hex))
	{
		if (second_op.find_last_of('x') != -1)
			op2 = stoi(second_op, 0, 16);
		else
			op2 = stoi(second_op);
	}
	else {
		bool found = false;
		SymbolTableEntry* temp = find_symbol(second_op);
		if (temp != nullptr) {
			found = true;
			section2 = temp->section;
		}
		
		if (found) op2 = temp->val;
		else {
			cout << "nedefinisan simbol";
			std::exit(-1);
		}
	}
	if (section1 != "" and section2 != "" and section1 != "abs" and section2 != "abs" and section1 != section2) {
		cout << "ne mogu simboli iz razl.sekcija"; std::exit(-1);
	}
	if (section1 != "" and section2 != "" and section1 == section2 and tokens[i].find_last_of('+') != -1) {
		cout << "ne moze sabiranje iz iste sekcije"; std::exit(-1);
	}
	if (section1 != "" and section1 != "abs" and (section2=="" or section2=="abs")) {
		SymbolTableEntry* temp = find_symbol(first_op);
		int num;
		if (temp->visibility == 'g')
			num = temp->num;
		else {
			SymbolTableEntry* sect = find_symbol(temp->section);
			num = sect->num;
		}
		add_relocation_entry(loc_cnt, "R_386_16", num);
	}
	if (section2 != "" and section2 != "abs" and (section1=="" or section1=="abs")) {
		SymbolTableEntry* temp = find_symbol(second_op);
		int num;
		if (temp->visibility == 'g')
			num = temp->num;
		else {
			SymbolTableEntry* sect = find_symbol(temp->section);
			num = sect->num;
		}
		add_relocation_entry(loc_cnt, "R_386_16", num);
	}
	int to_write;
	if (tokens[i].find_last_of('+')!=-1) 
		to_write = op1 + op2;
	else 
		to_write = op1 - op2;
	if (tokens[0] == ".byte") {
		if (to_write >= 0) {
			stringstream ss;
			ss <<"0x" << setfill('0') << setw(2) << hex << to_write;
			output << ss.str() << endl;
		}
		else {
			stringstream ss;
			ss << setfill('f') << setw(2) << hex << to_write;
			output << "0x" << ss.str().substr(6) << endl;
		}
	}
	else {
		if (to_write >= 0) {
			stringstream ss;
			ss << "0x" << setfill('0') << setw(4) << hex << to_write;
			output << "0x"<< ss.str().substr(4) << " " << ss.str().substr(0,4) << endl;
		}
		else {
			stringstream ss;
			ss << setfill('f') << setw(4) << hex << to_write;
			output << "0x" << ss.str().substr(6) << " 0x" << ss.str().substr(4, 2) << endl;
		}
		
	}
	
}
void Assembler::code_directive(vector<string> tokens, ofstream& output) {
	if (tokens[0] == ".byte" or tokens[0] == ".word") {
		for (int i = 2; i < tokens.size(); i++) {
			if (tokens[i] == "+" or tokens[i] == "-") {
				string expr = tokens[i - 1] + tokens[i] + tokens[i + 1];
				std::vector<string>::iterator it = tokens.begin();
				tokens.insert(it + (i - 1), expr);
				it = tokens.begin();
				tokens.erase(it + (i + 2));
				it = tokens.begin();
				tokens.erase(it + (i + 1));
				it = tokens.begin();
				tokens.erase(it + (i));
			}
		}
	}
	if (tokens[0] == ".skip") {
		int to_be_skipped;
		if (tokens[1].find_last_of('x') != -1) {
			to_be_skipped = stoi(tokens[1], 0, 16);
		}
		else
			to_be_skipped = stoi(tokens[1]);
		//int to_be_skipped = stoi(tokens[1]);
		for (int i = 0; i < to_be_skipped; i++)
		{
			output << "0x00 ";
			loc_cnt++;
		}
		output << endl;
	}
	if (tokens[0] == ".byte") {
		int num_of_bytes = tokens.size() - 1;
		SymbolTableEntry* temp = find_symbol(curr_section);
		if (temp->flags.find_last_of('w') == -1) {
			for (int i = 0; i < num_of_bytes; i++) {
				output << "0x00 " << endl;
				loc_cnt++;
			}
			return;
		}
		for (int i = 0; i < num_of_bytes; i++)
		{
			SymbolTableEntry* section = find_symbol(curr_section);
			if (section->flags.find_last_of("w") == -1) {
				output << "0x00" << endl;
				loc_cnt++;
				continue;
			}
			if (regex_match(tokens[i + 1], symb_expression)) {
				calc_byte_word(tokens, output);
			}
			if (regex_match(tokens[i + 1], symbol)) {
				/*for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next)
				{
					if (temp->name == tokens[i + 1])
					{
						if (temp->val > 127 or temp->val < -128) cout << "ERROR-zadao si nesto sto ne moze stati u 1b" << endl;
						stringstream stream;
						stream << "0x" << setfill('0') << setw(2) << hex << temp->val;
						output << stream.str() << endl;
						int num;
						if (temp->visibility == 'g')
							num = temp->num;
						else {
							for (SymbolTableEntry* sec = symbolTable; sec != nullptr; sec = sec->next) {
								if (sec->name == temp->section)
									num = sec->num;
								break;
							}
						}
						add_relocation_entry(loc_cnt, "R_386_16", num);
						break;
					}
				}*/
				SymbolTableEntry* temp = find_symbol(tokens[i + 1]);
				if (temp == nullptr) {
					cout << "error nedefinisan simbol" << endl;
					std::exit(-1);
				}
				else {
					int num;
					if (temp->visibility == 'g')
						num = temp->num;
					else {
						SymbolTableEntry* section = find_symbol(temp->section);
						num = section->num;
						}
					if (temp->section != "abs") {
						add_relocation_entry(loc_cnt, "R_386_16", num);
					}
					if (temp->visibility == 'g')
						output << "0x00 " << endl;
					else {
						if (temp->val > 127 or temp->val < -128) {
							cout << "ERROR-zadao si nesto sto ne moze stati u 1b" << endl; std::exit(-1);
						}
						stringstream stream;
						stream << "0x" << setfill('0') << setw(2) << hex << temp->val;
						output << stream.str() << endl;
					}
				}
			}
			if (regex_match(tokens[i+1],immed_hex)) {
				stringstream stream;
				string val = tokens[i+1].substr(2);
				
				if (val.length() > 2)
				{
					cout << "ERROR preveliki broj" << endl; std::exit(-1);
				}
				
				else {
					if (val.length() == 1) output << "0x0" + val + " ";
					if (val.length() == 2 ) output<< "0x" + val + " ";	
				}
			}
			if (regex_match(tokens[i + 1], immed_dec)) {
				stringstream stream;
				int byte = stoi(tokens[i + 1]);
				if (byte > 127 or byte < -128) {
					cout << "ERROR-zadao si nesto sto ne moze stati u 1b" << endl; std::exit(-1);
				}
				if (byte >= 0) {
					stream << setfill('0') << setw(2) << hex << byte;
					output << "0x" << stream.str() + " ";
				}
				else {
					stream << setfill('f') << setw(2) << hex << byte;
					output << "0x" << stream.str().substr(6) + " ";
				}
			}
			loc_cnt++;
		}
		output << endl;
	}
	if (tokens[0] == ".word") {
		int num_of_words = tokens.size() - 1;
		SymbolTableEntry* temp = find_symbol(curr_section);
		if (temp->flags.find_last_of('w') == -1) {
			for (int i = 0; i < num_of_words; i++) {
				output << "0x00 0x00" << endl;
				loc_cnt+=2;
			}
			return;
		}
		for (int i = 0; i < num_of_words; i++) {
			if (regex_match(tokens[i + 1], symb_expression)) {
				calc_byte_word(tokens, output);
			}
			if (regex_match(tokens[i + 1], immed_dec)) {
				stringstream stream;
				int word = stoi(tokens[i + 1]);
				if (word > 32767 or word < -32768) {
					cout << "ERROR-zadao si nesto sto ne moze stati u 2b" << endl; std::exit(-1);
				}
				if (word > 0) {
					stream << setfill('0') << setw(4) << hex << word;
					output << "0x" << stream.str().substr(2) + " 0x" + stream.str().substr(0, 2) << endl;
				}
				else {
					stream << setfill('f') << setw(4) << hex << word;
					output << "0x" << stream.str().substr(6) + " 0x" + stream.str().substr(4, 2) << endl;
				}
			}
			if (regex_match(tokens[i + 1], immed_hex)) {
				stringstream stream;
				string val = tokens[i + 1].substr(2);
				if (val.length() > 4)
				{
					cout << "ERROR preveliki broj" << endl; std::exit(-1);
				}

				else {
					if (val.length() == 1) output << "0x0" + val + " 0x00" << endl;
					if (val.length() == 2 and (val[0] - '0' < 8)) output << "0x" + val + " 0x00"<< endl;
					if (val.length() == 2 and (val[0] - '0' >= 8)) output << "0x" + val + " 0xff"<< endl;
					if (val.length() == 3 and (val[0] - '0' < 8)) output << "0x" + val.substr(1) + " 0x0" + val.substr(0, 1) << endl;
					if (val.length() == 3 and (val[0] - '0' >= 8)) output << "0x" + val.substr(1) + "0xf" + val.substr(0, 1)<< endl;
					if (val.length() == 4) output << "0x" + val.substr(2) + " 0x" + val.substr(0, 2)<<endl;
				}
			}
			if (regex_match(tokens[i + 1], symbol)) {
				SymbolTableEntry* temp = find_symbol(tokens[i + 1]);
				if (temp == nullptr) {
					cout << "nema simbola,greska"; std::exit(-1);
				}
				else {
					int num;
					if (temp->visibility == 'g') {
						num = temp->num;
						output << "0x00 0x00 " << endl;
					}
					else {
						SymbolTableEntry* section = find_symbol(temp->section);
						num = section->num;
						if (temp->val > 32767 or temp->val < -32768)
						{
							cout << "ERROR-zadao si nesto sto ne moze stati u 2b" << endl; std::exit(-1);
						}
						stringstream stream;
						stream << setfill('0') << setw(4) << hex << temp->val;
						output << "0x" + stream.str().substr(2) + " 0x" + stream.str().substr(0, 2) << endl;
					}
					if (temp->section != "abs") {
						add_relocation_entry(loc_cnt, "R_386_16", num);
					}
					
					
				}
			}
			loc_cnt+=2;

		}
		
	}
	if (tokens[0] == ".align") {
		int alignment;
		if (tokens[1].find_last_of('x') != -1) {
			alignment = stoi(tokens[1], 0, 16);
		}
		else
			alignment = stoi(tokens[1]);
		if (alignment % 2 != 0) {
			cout << "NE MOZE mora stepen 2";
			std::exit(-1);
		}
		while (loc_cnt % alignment != 0) {
			if (tokens.size() > 2) {
				if (tokens[2].find_last_of('x') != -1) {
					string val = tokens[2].substr(2);

					if (val.length() > 2)
					{
						cout << "ERROR preveliki broj" << endl; std::exit(-1);
					}

					else {
						if (val.length() == 1) output << "0x0" + val + " ";
						if (val.length() == 2) output << "0x" + val + " ";
					}
				}
				else {
					int val = stoi(tokens[2]);
					stringstream ss;
					ss << setw(2) << hex << val;
					output << "0x" + ss.str() <<endl;
				}
			}
			else {
				output << "0x00 ";
			}
			loc_cnt++;
		}
		output << endl;
	}
	if (tokens[0] == ".equ")
		return;
}

void Assembler::turn_symbols_global(vector<string> tokens) {
	if (tokens[0] == ".extern") return;
	if (tokens[0] == ".global") {
		for (int i = 1; i < tokens.size(); i++) {
			SymbolTableEntry* temp = find_symbol(tokens[i]);
			if (temp == nullptr) {
				cout << "simbol je ostao nedefinisan" << endl;
				std::exit(-1);
			}
			else {
					temp->visibility = 'g';
			}
		}
	}
}

void Assembler::SecondRun(ifstream& input, ofstream& output) {
	string line;
	loc_cnt = 0;
	while (getline(input, line)) {
		
		vector<string> tokens = tokenize(line);
		if (tokens.size() == 0)
			continue;
		if (tokens[0] == ".end")
		{
			
			break;
		}
		if (regex_match(tokens[0], label)) {
			tokens.erase(tokens.begin());
			if (tokens.size() == 0)
				continue;
		}
		if (regex_match(tokens[0], section) or tokens[0]==".section") {
			loc_cnt = 0;
			if (tokens[0] == ".text" or tokens[0] == ".data" or tokens[0] == ".bss")
				//curr_section = tokens[0].substr(1);
				curr_section = tokens[0];
			else 
				curr_section = tokens[1];
			output << curr_section << endl;
		}
		if (regex_match(tokens[0], globl)) {
			turn_symbols_global(tokens);
		}
		if (regex_match(tokens[0], instruction)) {
			loc_cnt += instr_size(tokens);
			code_instr(tokens, output);
		}
		if (regex_match(tokens[0], directive)) {
			code_directive(tokens, output);
		}
		
	}
	cout << "drugi prolaz gotov" << endl;
	output << "SymbolTable" << endl;
	output << setw(10)<<"name\t"<<setw(10)<<"section\t"<<setw(10)<<"value\t"<<setw(10)<<"visibility" << setw(10) << "num"<< endl;
	for (SymbolTableEntry* temp = symbolTable; temp != nullptr; temp = temp->next) {
			stringstream stream; 
			stream << "0x" << hex << temp->val;
			char j = temp->visibility;
			string vis; vis.push_back(temp->visibility);
			output << setw(10)<< temp->name + "\t" <<setw(10)<< temp->section + "\t" <<setw(10)<< stream.str() + "\t" <<setw(10)<< vis + "\t" << setw(10) << temp->num << endl;	
	}
	vector<string> sections = all_sections();
	for (int i = 0; i < sections.size(); i++) {
		output << "#.ret." + sections[i] << endl;
		output << setw(10) << "offset\t" << setw(10) << "type\t" << setw(10) << "num" << endl;
		for (RelocationTableEntry* rel = relocationTable; rel != nullptr; rel = rel->next) {
			if (rel->section == sections[i]) {
				stringstream stream;//ispis relokacione tabele
				stream << "0x" << hex << rel->offset;
				string s = to_string(rel->num);
				string row = stream.str() + "\t" + rel->type + "\t" + s + "\t";
				output << setw(10) << stream.str() + "\t" << setw(10) << rel->type + "\t" << setw(10) << s << endl;
			}
		}
	}

}
vector<string> Assembler::all_sections() {
	vector<string> ret;
	for (SymbolTableEntry* temp = symbolTable->next; temp != nullptr; temp = temp->next) {
		if (temp->name == temp->section)
			ret.push_back(temp->name);
	}
	return ret;
}



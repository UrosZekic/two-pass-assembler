#include "Instruction.h"

Instruction::Instruction(string hex,regex assembly) {
	hexa_code = hex;
	assembly_code = assembly;
}
Instruction::~Instruction() {

}
regex Instruction::getAssembly_code() {
	return assembly_code;
}
string Instruction::getHexa_code() {
	return hexa_code;
}
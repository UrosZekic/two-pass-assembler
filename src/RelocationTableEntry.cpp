#include "RelocationTableEntry.h"

RelocationTableEntry::RelocationTableEntry(int offset, string type, int num,string section) {
	this->offset = offset;
	this->type = type;
	this->num = num;
	this->section = section;
	this->next = nullptr;
}
RelocationTableEntry::~RelocationTableEntry() {

}
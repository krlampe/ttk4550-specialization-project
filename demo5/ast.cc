#include <stdio.h>
#include <string>
#include <vector>
#include <stdexcept>
#include "ast.hh"

/* Symbol table */

SymbolTable *SymbolTable::singleton = nullptr;

SymbolTable *SymbolTable::get_instance() {
	if (!singleton) {
		singleton = new SymbolTable();
	}
	return singleton;
}

const Symbol& SymbolTable::get_symbol(std::string name) const {
	for (const auto& sym : symbols) {
		if (sym.name == name) {
			return sym;
		}
	}
	throw std::runtime_error("undefined symol: " + name);
}

int SymbolTable::find_symbol(std::string name) const {
	for (const auto& sym : symbols) {
		if (sym.name == name) {
			return sym.index;
		}
	}
	return -1;
}

void SymbolTable::add_symbol(std::string name, AST *equation) {
	if (find_symbol(name) != -1) {
		fprintf(stderr, "symbol defined twice: %s\n", name.c_str());
	}
	int index = symbols.size();
	symbols.push_back(Symbol{name, index, equation});
	++nr_of_equations;
}

void SymbolTable::symbol_check() const {
	for (const auto& sym : symbols) {
		sym.equation->symbol_check();
	}
}

void SymbolTable::free() {
	for (auto& sym : symbols) {
		delete sym.equation;
	}
	symbols.clear();
	nr_of_equations = 0;

	delete singleton;
	singleton = nullptr;
}

void SymbolTable::print_symbols() const {
	puts("Symbols:");
	for (const auto& sym : symbols) {
		puts(sym.name.c_str());
	}
}

void SymbolTable::print_params() const {
	puts("Parameters:");
	for (const auto& [name, value] : parameters) {
		printf("%s = %g\n", name.c_str(), value);
	}
}

bool SymbolTable::lookup_param(std::string name) const {
	try {
		parameters.at(name);
		return true;
	} catch (std::out_of_range&) {
		return false;
	}
}

void SymbolTable::add_param(std::string name) {
	parameters[name] = 1;
}

void SymbolTable::set_param(std::string name, double value) {
	parameters[name] = value;
}

/* AST nodes' print */

void AstNumber::print(FILE *out) const {
	fprintf(out, "%g", value);
}

void AstSymbol::print(FILE *out) const {
	fprintf(out, "%s", name.c_str());
}

void AstVariable::print(FILE *out) const {
	fprintf(out, "%s", name.c_str());
}

void BinaryOperator::print(FILE *out) const {
	switch (operat){
	case '+':
	case '-':
		fprintf(out, "%c",'(');
		left->print(out);
		fprintf(out, " %c ", operat); // spaces around
		right->print(out);
		fprintf(out, "%c",')');
		break;
	case '*':
	case '/':
	case '^':
		fprintf(out, "%c",'(');
		left->print(out);
		fprintf(out, "%c", operat); // without spaces around
		right->print(out);
		fprintf(out, "%c",')');
		break;
	default:
		fprintf(stderr, "Bad operator: %c\n", operat);
		break;
	}
}

void UnaryOperator::print(FILE *out) const {
	if (operat == 'M') {
		fputs("(-", out);
		operand->print(out);
    fputs(")", out);
	}
	else if (operat == 'A') {
		fprintf(out, "%c",'|');
		operand->print(out);
		fprintf(out, "%c",'|');
	}
	else {
		fprintf(stderr, "Bad operator: %c\n", operat);
	}
}

void BuiltInFunc::print(FILE *out) const {
	fprintf(out, "%s", name.c_str());
	fprintf(out, "%s","(");
	argument->print(out);
	fprintf(out, "%s",")");
}


/* Symbol check */

void AstSymbol::symbol_check() const {
	if (SymbolTable::get_instance()->find_symbol(name) == -1)
		throw std::runtime_error("undefined symol: " + name);
}

void AstVariable::symbol_check() const {}
void AstNumber::symbol_check() const {}

void BinaryOperator::symbol_check() const {
	left->symbol_check();
	right->symbol_check();
}

void UnaryOperator::symbol_check() const {
	operand->symbol_check();
}

void BuiltInFunc::symbol_check() const {
	argument->symbol_check();
}

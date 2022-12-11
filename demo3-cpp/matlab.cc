#include <stdio.h>
#include <string>
#include "matlab.hh"
#include "ast.hh"

/**
 * Modify the AST of an equation so that the names of nodes are matlab compatible.
 * Variable names are converted to y(i), where i is the variable's index in the
 * matlab matrix y, which is equal to its index in the symbol table plus 1.
 * (Matlab has 1-based indexing.)
 * 
 * Assumes that the every symbol used is defined.
*/
class MatlabPreprocessor : public AstVisitor {
  void visit(AstNumber *node);
  void visit(AstSymbol *node);
  void visit(AstVariable *node);
  void visit(BinaryOperator *node);
  void visit(UnaryOperator *node);
  void visit(BuiltInFunc *node);
};

/**
 * Print RHS of differential equations for matlab code by visiting ASTs.
 */
class MatlabPrinter : public AstVisitor {
public:
  MatlabPrinter(FILE *outputfile) : out{outputfile} {}

  void visit(AstNumber *node);
  void visit(AstSymbol *node);
  void visit(AstVariable *node);
  void visit(BinaryOperator *node);
  void visit(UnaryOperator *node);
  void visit(BuiltInFunc *node);

private:
  FILE *out;
};

void Matlab::generate(FILE *out) {
  SymbolTable *symbol_table = SymbolTable::get_instance();

  /* Heading */
  fputs("%% Simulator generated by sim-gen.\n% ODE:\n", out);
  for (const auto& sym : symbol_table->get_symbols()) {
    fprintf(out, "%% (d/dt)%s = ", sym.name.c_str());
    sym.equation->print(out);
    fputs("\n", out);
  }

  /* Modify all the ASTs */
  for (const auto& sym : symbol_table->get_symbols()) {
    MatlabPreprocessor modify_visitor{};
    sym.equation->accept(&modify_visitor);
  }

  /*
  Print matlab formatet equations:
      dydt = @(t, y) [<equations>];
  */
  fputs("\ndydt = @(t, y) [", out);
  for (const auto& sym : symbol_table->get_symbols()) {
    MatlabPrinter print_visitor{out};
    sym.equation->accept(&print_visitor);
    fputs(";", out);
  }
  fputs("];\n", out);

  /* Initial values */
  fputs("y_0 = [0", out);
  for (int i = 1; i < symbol_table->get_nr_of_equations(); i++) {
    fputs(", 0", out);
  }
  fputs("];\n", out);

  /* Integrator */
  fputs("[t,y] = ode45(dydt, [0 20], y_0);\n", out);

  /* Plotting */
  fputs(
    "figure; clf;\n"
    "hold on;\n",
    out
  );
  for (int i = 0; i < symbol_table->get_nr_of_equations(); i++) {
    fprintf(out, "plot(t,y(:,%d),'-o');\n", i+1);
  }
  fputs(
    "hold off;\n"
    "grid on;\n"
    "xlabel('Time t');\n"
    "ylabel('Solution');\n",
    out
  );
  fprintf(out, "legend('%s'", symbol_table->get_symbols()[0].name.c_str());
  for (int i = 1; i < symbol_table->get_nr_of_equations(); i++) {
    fprintf(out, ", '%s'", symbol_table->get_symbols()[i].name.c_str());
  }
  fputs(");\n", out);
}


/* MatlabPreprocessor */

void MatlabPreprocessor::visit(AstNumber *node) {}

void MatlabPreprocessor::visit(AstSymbol *node) {
  const Symbol& sym = SymbolTable::get_instance()->get_symbol(node->name);
  node->name = "y(" + std::to_string(sym.index+1) + ")";
}

void MatlabPreprocessor::visit(AstVariable *node) {}

void MatlabPreprocessor::visit(BinaryOperator *node) {
  node->left->accept(this);
  node->right->accept(this);
}

void MatlabPreprocessor::visit(UnaryOperator *node) {
  node->operand->accept(this);
}

void MatlabPreprocessor::visit(BuiltInFunc *node) {
  node->argument->accept(this);
}


/* MatlabPrinter */

void MatlabPrinter::visit(AstNumber *node) {
	fprintf(out, "%g", node->value);
}

void MatlabPrinter::visit(AstSymbol *node) {
	fprintf(out, "%s", node->name.c_str());
}

void MatlabPrinter::visit(AstVariable *node) {
	fprintf(out, "%s", node->name.c_str());
}

void MatlabPrinter::visit(BinaryOperator *node) {
  fprintf(out, "%c",'(');
  node->left->accept(this);
  fprintf(out, "%c", node->operat);
  node->right->accept(this);
  fprintf(out, "%c",')');
}

void MatlabPrinter::visit(UnaryOperator *node) {
	if (node->operat == 'M') {
		fputs("(-", out);
		node->operand->accept(this);
    fputs(")", out);
	}
	else if (node->operat == 'A') {
		fprintf(out, "%s","abs(");
		node->operand->accept(this);
		fprintf(out, "%c",')');
	}
}

void MatlabPrinter::visit(BuiltInFunc *node) {
  fprintf(out, "%s", node->name.c_str());
	fprintf(out, "%s","(");
	node->argument->accept(this);
	fprintf(out, "%s",")");
}

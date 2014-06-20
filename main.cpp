#include <iostream>
#include <cstdio>
#include "codegen.h"
#include "node.h"

using namespace std;

extern int yyparse();
extern NBlock* programBlock;

void createCoreFunctions(CodeGenContext& context);

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "no input files" << endl;
		std::cout << "usage: " << argv[0] << " source" << endl;
		return 1;
	}

	if (!freopen(argv[1], "r", stdin)) {
		std::cout << "could not open file: " << argv[1] << endl;
		return 1;
	}

	yyparse();

    // see http://comments.gmane.org/gmane.comp.compilers.llvm.devel/33877
	InitializeNativeTarget();
	CodeGenContext context;
	createCoreFunctions(context);
	context.generateCode(*programBlock);
	context.runCode();


	std::string str("error");

	llvm::raw_fd_ostream out("main.bc", str);
	context.module->print(out, NULL);

	return 0;
}


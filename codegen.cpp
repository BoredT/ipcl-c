#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::cout << "Generating code...\n";
	
	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType *ftype = FunctionType::get(Type::getInt32Ty(getGlobalContext()), makeArrayRef(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
	BasicBlock *bblock = BasicBlock::Create(getGlobalContext(), "entry", mainFunction, 0);
	
	/* Push a new variable/block context */
	pushBlock(bblock);
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	ReturnInst::Create(getGlobalContext(), ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0), currentBlock());

	removeBlock(bblock); //could be more than one block, remove all
	
	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	PassManager pm;
	pm.add(createPrintModulePass(&outs()));
	pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() {
	std::cout << "Running code...\n";
	ExecutionEngine *ee = EngineBuilder(module).create();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::cout << "Code was run.\n";
	return v;
}

/* Returns an LLVM type based on the identifier */
static Type *typeOf(const NIdentifier& type) 
{
	if (type.name.compare("long") == 0) {
		return Type::getInt64Ty(getGlobalContext());
	}
	// else if (type.name.compare("double") == 0) {
	// 	return Type::getDoubleTy(getGlobalContext());
	// }
	else if (type.name.compare("void") == 0) {
		return Type::getVoidTy(getGlobalContext());
	}
	else {
		std::cerr << "undefined type: " << type.name << std::endl;
		exit(1);
	}
}

/* -- Code Generation -- */

Value* NLong::codeGen(CodeGenContext& context)
{
	std::cout << "Creating long: " << value << endl;
	return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::cout << "Creating double: " << value << endl;
	return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::cout << "Creating identifier reference: " << name << endl;
	Value* var = context.getVar(name);
	if (!var) {
		std::cerr << "undeclared variable " << name << endl;
		exit(1);
		return NULL;
	}
	return new LoadInst(var, "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	Function *function = context.module->getFunction(id.name.c_str());
	if (function == NULL) {
		std::cerr << "no such function " << id.name << endl;
	}
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		args.push_back((**it).codeGen(context));
	}
	CallInst *call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
	std::cout << "Creating method call: " << id.name << endl;
	return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::cout << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	switch (op) {
		case TPLUS: 	instr = Instruction::Add; goto math;
		case TMINUS: 	instr = Instruction::Sub; goto math;
		case TMUL: 		instr = Instruction::Mul; goto math;
		case TDIV: 		instr = Instruction::SDiv; goto math;

		default:
			std::cerr << "Error. No handler found for given op " << op << std::endl;
			exit(1);
	}

	return NULL;
math:
	return BinaryOperator::Create(instr, lhs.codeGen(context), 
		rhs.codeGen(context), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;
	Value* var = context.getVar(lhs.name);
	if (!var) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		exit(1);
		return NULL;
	}
	return new StoreInst(rhs.codeGen(context), var, false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	Value *last = NULL;
	for (it = statements.begin(); it != statements.end(); it++) {
		std::cout << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating return code for " << typeid(expression).name() << endl;
	Value *returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
	std::cout << "Creating variable declaration " << type.name << " " << id.name << endl;
	AllocaInst *alloc = new AllocaInst(typeOf(type), id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != NULL) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType *ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
	Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	BasicBlock *bblock = BasicBlock::Create(getGlobalContext(), "entry", function, 0);

	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
    Value* argumentValue;

	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).codeGen(context);
		
		argumentValue = argsValues++;
		argumentValue->setName((*it)->id.name.c_str());
		StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
	}
	block.codeGen(context);
	ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(), context.currentBlock());

	context.removeBlock(bblock); //could be more than one block, remove all
	std::cout << "Creating function: " << id.name << endl;
	return function;
}

Value* NIfStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating if statement" << endl;
	Value *condV = _cond->codeGen(context);

	IRBuilder<> Builder(context.currentBlock());

	condV = Builder.CreateICmpNE(condV, ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0), "ifcond");

	Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the end of the function.
	BasicBlock *thenBB = BasicBlock::Create(getGlobalContext(), "then", TheFunction);
	BasicBlock *elseBB = BasicBlock::Create(getGlobalContext(), "else");
	BasicBlock *ifContBB = BasicBlock::Create(getGlobalContext(), "ifcont");

	Builder.CreateCondBr(condV, thenBB, elseBB);

	context.pushBlock(thenBB);
	Value* thenV = _then->codeGen(context);
	context.popBlock();

	Builder.SetInsertPoint(thenBB);
	Builder.CreateBr(ifContBB);

	TheFunction->getBasicBlockList().push_back(elseBB);
	Builder.SetInsertPoint(elseBB);

	if (_then != _else) { // if false - no else block defined
		context.pushBlock(elseBB);
		Value* elseV = _else->codeGen(context);
		context.popBlock();
	}

	Builder.CreateBr(ifContBB);

	TheFunction->getBasicBlockList().push_back(ifContBB);
	Builder.SetInsertPoint(ifContBB);

	context.pushBlock(ifContBB); //no need to pop

	return TheFunction;
}

Value* NWhileStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating while statement" << endl;

	IRBuilder<> Builder(context.currentBlock());

	Function *TheFunction = Builder.GetInsertBlock()->getParent();

	// Create blocks for the then and else cases.  Insert the 'then' block at the end of the function.
	BasicBlock *loopCondBB = BasicBlock::Create(getGlobalContext(), "loopCond", TheFunction);
	BasicBlock *loopBB = BasicBlock::Create(getGlobalContext(), "loop");
	BasicBlock *whileContBB = BasicBlock::Create(getGlobalContext(), "whilecont");

	Builder.CreateBr(loopCondBB);
	Builder.SetInsertPoint(loopCondBB);

	context.pushBlock(loopCondBB);
	Value *condV = _cond->codeGen(context);
	context.popBlock();

	condV = Builder.CreateICmpNE(condV, ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 0), "whilecond");
	Builder.CreateCondBr(condV, loopBB, whileContBB);
	Builder.SetInsertPoint(loopBB);

	context.pushBlock(loopBB);
	Value* thenV = _then->codeGen(context);
	context.popBlock();
	Builder.CreateBr(loopCondBB);

	TheFunction->getBasicBlockList().push_back(loopBB);

	TheFunction->getBasicBlockList().push_back(whileContBB);
	Builder.SetInsertPoint(whileContBB);

	context.pushBlock(whileContBB); //no need to pop

	return TheFunction;
}

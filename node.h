#include <iostream>
#include <vector>
#include <llvm/IR/Value.h>

class CodeGenContext;
class NStatement;
class NExpression;
class NVariableDeclaration;

typedef std::vector<NStatement*> StatementList;
typedef std::vector<NExpression*> ExpressionList;
typedef std::vector<NVariableDeclaration*> VariableList;

class Node {
public:
	virtual ~Node() {}
	virtual llvm::Value* codeGen(CodeGenContext& context) { return NULL; }
};

class NExpression : public Node {
};

class NStatement : public Node {
};

class NLong : public NExpression {
public:
	long long value;
	NLong(long long value) : value(value) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NDouble : public NExpression {
public:
	double value;
	NDouble(double value) : value(value) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NIdentifier : public NExpression {
public:
	std::string name;
	NIdentifier(const std::string& name) : name(name) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NMethodCall : public NExpression {
public:
	const NIdentifier& id;
	ExpressionList arguments;
	NMethodCall(const NIdentifier& id, ExpressionList& arguments) :
		id(id), arguments(arguments) { }
	NMethodCall(const NIdentifier& id) : id(id) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NBinaryOperator : public NExpression {
public:
	int op;
	NExpression& lhs;
	NExpression& rhs;
	NBinaryOperator(NExpression& lhs, int op, NExpression& rhs) :
		lhs(lhs), rhs(rhs), op(op) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NAssignment : public NExpression {
public:
	NIdentifier& lhs;
	NExpression& rhs;
	NAssignment(NIdentifier& lhs, NExpression& rhs) : 
		lhs(lhs), rhs(rhs) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NBlock : public NExpression {
public:
	StatementList statements;
	NBlock() { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NExpressionStatement : public NStatement {
public:
	NExpression& expression;
	NExpressionStatement(NExpression& expression) : 
		expression(expression) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NReturnStatement : public NStatement {
public:
	NExpression& expression;
	NReturnStatement(NExpression& expression) : 
		expression(expression) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NVariableDeclaration : public NStatement {
public:
	const NIdentifier& type;
	NIdentifier& id;
	NExpression *assignmentExpr;
	NVariableDeclaration(const NIdentifier& type, NIdentifier& id) :
		type(type), id(id) { }
	NVariableDeclaration(const NIdentifier& type, NIdentifier& id, NExpression *assignmentExpr) :
		type(type), id(id), assignmentExpr(assignmentExpr) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NFunctionDeclaration : public NStatement {
public:
	const NIdentifier& type;
	const NIdentifier& id;
	VariableList arguments;
	NBlock& block;
	NFunctionDeclaration(const NIdentifier& type, const NIdentifier& id, 
			const VariableList& arguments, NBlock& block) :
		type(type), id(id), arguments(arguments), block(block) { }
	virtual llvm::Value* codeGen(CodeGenContext& context);
};

class NIfStatement : public NStatement {
	bool _then_alloc;
	bool _else_alloc;
public:
	NExpression* _cond;
	NBlock* _then;
	NBlock* _else;

	NIfStatement(NExpression* cond, NStatement* then_, NStatement* else_) : 
		_cond(cond) {
		_then = new NBlock();
		_then->statements.push_back(then_);
		_then_alloc = true;

		_else = new NBlock();
		_else->statements.push_back(else_);
		_else_alloc =- true;
	}

	NIfStatement(NExpression* cond, NBlock* then_, NStatement* else_) : 
		_cond(cond), _then(then_) {

		_else = new NBlock();
		_else->statements.push_back(else_);
		_else_alloc =- true;
	}

	NIfStatement(NExpression* cond, NStatement* then_, NBlock* else_) : 
		_cond(cond), _else(else_) {
		_then = new NBlock();
		_then->statements.push_back(then_);
		_then_alloc = true;
	}

	NIfStatement(NExpression* cond, NBlock* then_, NBlock* else_) : 
		_cond(cond), _then(then_), _else(else_) {}

	virtual llvm::Value* codeGen(CodeGenContext& context);

	~NIfStatement() {
		if (_then_alloc) {
			delete _then;
			_then = 0;
		}
		if (_else_alloc) {
			delete _else;
			_else = 0;
		}
	}
};

class NWhileStatement : public NStatement {
bool _then_alloc;
public:
	NExpression* _cond;
	NBlock* _then;

	NWhileStatement(NExpression* cond, NBlock* then_) : 
		_cond(cond), _then(then_) { }
		
	NWhileStatement(NExpression* cond, NStatement* then_) : 
		_cond(cond) { 
		_then = new NBlock();
		_then->statements.push_back(then_);
		_then_alloc = true;
	}

	virtual llvm::Value* codeGen(CodeGenContext& context);

	~NWhileStatement() {
		if (_then_alloc) {
			delete _then;
			_then = 0;
		}
	}
};

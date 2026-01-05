#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>

// Forward declarations
class SymbolTable;

// ---------------------------
// Parse Tree (Concrete Syntax)
// ---------------------------
struct ParseTreeNode {
    std::string label;
    std::vector<std::unique_ptr<ParseTreeNode>> children;

    explicit ParseTreeNode(std::string l) : label(std::move(l)) {}

    ParseTreeNode* addChild(std::string l) {
        children.push_back(std::make_unique<ParseTreeNode>(std::move(l)));
        return children.back().get();
    }

    void print() const {
        printImpl("", true, true, std::cout);
    }

    void printToFile(std::ostream& out) const {
        printImpl("", true, true, out);
    }

    void printImpl(const std::string& prefix, bool isLast, bool isRoot, std::ostream& out) const {
        if (isRoot) {
            out << label << "\n";
        }
        else {
            out << prefix << (isLast ? "└── " : "├── ") << label << "\n";
        }

        const std::string childPrefix = isRoot ? "" : (prefix + (isLast ? "    " : "│   "));
        for (size_t i = 0; i < children.size(); ++i) {
            const bool last = (i + 1 == children.size());
            children[i]->printImpl(childPrefix, last, false, out);
        }
    }
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void print(int level = 0) const = 0;
    void printIndent(int level) const {
        for (int i = 0; i < level; ++i) std::cout << "  |";
        if (level > 0) std::cout << "--";
    }
};

// Вирази
class ExpressionNode : public ASTNode {};

class NumberNode : public ExpressionNode {
public:
    std::string val;
    NumberNode(const std::string& v) : val(v) {}
    void print(int l) const override { printIndent(l); std::cout << "Num: " << val << "\n"; }
};

class BoolNode : public ExpressionNode {
public:
    std::string val;  // "1" для true, "0" для false
    BoolNode(const std::string& v) : val(v) {}
    void print(int l) const override { 
        printIndent(l); 
        std::cout << "Bool: " << (val == "1" ? "TRUE" : "FALSE") << "\n"; 
    }
};

class VarRefNode : public ExpressionNode {
public:
    std::string name;
    ExpressionNode* idx;
    VarRefNode(const std::string& n, ExpressionNode* i = nullptr) : name(n), idx(i) {}
    void print(int l) const override {
        printIndent(l); std::cout << "Var: " << name;
        if (idx) std::cout << "[]";
        std::cout << "\n";
        if (idx) idx->print(l + 1);
    }
};

class StringNode : public ExpressionNode {
public:
    std::string text;
    StringNode(const std::string& t) : text(t) {}
    void print(int l) const override { printIndent(l); std::cout << "Str: " << text << "\n"; }
};

class BinaryOpNode : public ExpressionNode {
public:
    std::string op;
    ExpressionNode* left;
    ExpressionNode* right;
    BinaryOpNode(const std::string& o, ExpressionNode* l, ExpressionNode* r) : op(o), left(l), right(r) {}
    void print(int l) const override {
        printIndent(l); std::cout << "Op: " << op << "\n";
        if (left) left->print(l + 1); if (right) right->print(l + 1);
    }
};

class UnaryOpNode : public ExpressionNode {
public:
    std::string op;
    ExpressionNode* operand;
    UnaryOpNode(const std::string& o, ExpressionNode* e) : op(o), operand(e) {}
    void print(int l) const override {
        printIndent(l); std::cout << "Unary: " << op << "\n";
        if (operand) operand->print(l + 1);
    }
};

// Оператори
class StatementNode : public ASTNode {};

class BlockNode : public StatementNode {
public:
    std::vector<StatementNode*> stmts;
    void add(StatementNode* s) { stmts.push_back(s); }
    void print(int l) const override {
        printIndent(l); std::cout << "Block\n";
        for (auto* s : stmts) s->print(l + 1);
    }
};

class AssignNode : public StatementNode {
public:
    std::string name;
    ExpressionNode* idx;
    ExpressionNode* val;
    AssignNode(const std::string& n, ExpressionNode* i, ExpressionNode* v) : name(n), idx(i), val(v) {}
    void print(int l) const override {
        printIndent(l); std::cout << "Assign -> " << name << "\n";
        if (idx) { printIndent(l + 1); std::cout << "Idx:\n"; idx->print(l + 2); }
        if (val) val->print(l + 1);
    }
};

class IO_Node : public StatementNode {
public:
    std::string type;
    std::vector<ExpressionNode*> args;
    IO_Node(const std::string& t, std::vector<ExpressionNode*> a) : type(t), args(a) {}
    void print(int l) const override {
        printIndent(l); std::cout << type << "\n";
        for (auto* arg : args) {
            if (arg) arg->print(l + 1);
        }
    }
};

class IfNode : public StatementNode {
public:
    ExpressionNode* cond;
    StatementNode* thenStmt;
    StatementNode* elseStmt;
    IfNode(ExpressionNode* c, StatementNode* t, StatementNode* e = nullptr) : cond(c), thenStmt(t), elseStmt(e) {}
    void print(int l) const override {
        printIndent(l); std::cout << "If\n";
        if (cond) { printIndent(l + 1); std::cout << "Condition:\n"; cond->print(l + 2); }
        if (thenStmt) { printIndent(l + 1); std::cout << "Then:\n"; thenStmt->print(l + 2); }
        if (elseStmt) { printIndent(l + 1); std::cout << "Else:\n"; elseStmt->print(l + 2); }
    }
};

class RepeatNode : public StatementNode {
public:
    StatementNode* body;
    ExpressionNode* condition;
    RepeatNode(StatementNode* b, ExpressionNode* c) : body(b), condition(c) {}
    void print(int l) const override {
        printIndent(l); std::cout << "Repeat\n";
        if (body) { printIndent(l + 1); std::cout << "Body:\n"; body->print(l + 2); }
        if (condition) { printIndent(l + 1); std::cout << "Until:\n"; condition->print(l + 2); }
    }
};

class ProgramNode : public ASTNode {
public:
    std::string name;
    std::vector<std::string> vars;
    BlockNode* body;
    SymbolTable* symbolTable;  // Додаємо посилання на таблицю символів
    
    ProgramNode(const std::string& n) : name(n), body(nullptr), symbolTable(nullptr) {}
    void addVar(const std::string& v) { vars.push_back(v); }
    void setBody(BlockNode* b) { body = b; }
    void setSymbolTable(SymbolTable* st) { symbolTable = st; }
    SymbolTable* getSymbolTable() const { return symbolTable; }

    void print(int /*l*/ = 0) const override {
        std::cout << "AST for Program: " << name << "\n";
        if (body) body->print(1);
    }

    void printRefinedTable() const {
        std::cout << "\n=== Уточнена таблиця ідентифікаторів ===\n";
        std::cout << "-----------------------------------------------------\n";
        std::cout << "| Ім'я       | Тип       | Категорія | Розмір       |\n";
        std::cout << "-----------------------------------------------------\n";

        for (const auto& varDecl : vars) {
            std::stringstream ss(varDecl);
            std::string type, nameStr;
            ss >> type >> nameStr;

            std::string name = nameStr;
            std::string category = "Змінна";
            std::string size = "1";

            size_t bracketPos = nameStr.find('[');
            if (bracketPos != std::string::npos) {
                category = "Масив";
                name = nameStr.substr(0, bracketPos);

                size_t closeBracket = nameStr.find(']');
                size = nameStr.substr(bracketPos + 1, closeBracket - bracketPos - 1);
            }

            std::cout << "| " << std::left << std::setw(11) << name
                << "| " << std::setw(10) << type
                << "| " << std::setw(9) << category
                << "| " << std::setw(14) << size << "|\n";
        }
        std::cout << "-----------------------------------------------------\n";
    }

    void writeRefinedTableToFile(std::ostream& out) const {
        out << "\n=== Уточнена таблиця ідентифікаторів ===\n";
        out << "-----------------------------------------------------\n";
        out << "| Ім'я       | Тип       | Категорія | Розмір       |\n";
        out << "-----------------------------------------------------\n";

        for (const auto& varDecl : vars) {
            std::stringstream ss(varDecl);
            std::string type, nameStr;
            ss >> type >> nameStr;

            std::string name = nameStr;
            std::string category = "Змінна";
            std::string size = "1";

            size_t bracketPos = nameStr.find('[');
            if (bracketPos != std::string::npos) {
                category = "Масив";
                name = nameStr.substr(0, bracketPos);

                size_t closeBracket = nameStr.find(']');
                size = nameStr.substr(bracketPos + 1, closeBracket - bracketPos - 1);
            }

            out << "| " << std::left << std::setw(11) << name
                << "| " << std::setw(10) << type
                << "| " << std::setw(9) << category
                << "| " << std::setw(14) << size << "|\n";
        }
        out << "-----------------------------------------------------\n";
    }
};


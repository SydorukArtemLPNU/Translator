#include "Parser.h"
#include "SymbolTable.h"
#include "AST.h"
#include <iostream>
#include <sstream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0), hadError(false) {}

ProgramNode* Parser::parse() {
    pos = 0;
    errors.clear();
    hadError = false;
    parseTreeRoot.reset();
    parseTreeStack.clear();
    if (tokens.empty()) return nullptr;
    return program();
}

// --- Утиліти ---
bool Parser::isAtEnd() const { return pos >= tokens.size(); }

const Token& Parser::current() const {
    static Token eofToken{ "", 0, 0, "EOF", "" };
    return isAtEnd() ? eofToken : tokens[pos];
}

void Parser::advance() { if (!isAtEnd()) pos++; }

bool Parser::match(const std::string& type) {
    if (check(type)) {
        const Token t = current();
        advance();
        addTerminalForToken(t);
        return true;
    }
    return false;
}

bool Parser::check(const std::string& type) const {
    if (isAtEnd()) return false;
    return current().tokenType == type;
}

bool Parser::checkType() const { 
    return check("IntType") || check("FloatType") || check("BoolType"); 
}

bool Parser::checkBoolConst() const {
    return check("BooleanConstant");
}

bool Parser::checkRelOp() const {
    std::string t = current().tokenType;
    return t == "Equal" || t == "NotEqual" || t == "GreaterThan" || 
           t == "LessThan" || t == "GreaterEqual" || t == "LessEqual";
}

// --- Помилки ---
void Parser::error(const std::string& msg) {
    if (hadError) return;
    std::string fullMsg = "Syntax Error line " + std::to_string(current().lineNumber) + ": " + msg;
    errors.push_back(fullMsg);
    hadError = true;
}

void Parser::errorExp(const std::string& expected) {
    error("Expected " + expected + ", but found '" + current().tokenName + "'");
}

void Parser::sync() {
    advance();
    while (!isAtEnd()) {
        if (current().tokenType == "Semicolon") {
            advance();
            return;
        }
        if (check("Input") || check("Output") || check("If") || check("Repeat") || 
            check("StartProgram") || check("StopProgram") || check("Identifier")) {
            return;
        }
        advance();
    }
}

// --- Parse tree helpers ---
void Parser::enterParseNode(const std::string& label) {
    if (!parseTreeRoot) {
        parseTreeRoot = std::make_unique<ParseTreeNode>(label);
        parseTreeStack.push_back(parseTreeRoot.get());
        return;
    }
    if (parseTreeStack.empty()) return;
    ParseTreeNode* n = parseTreeStack.back()->addChild(label);
    parseTreeStack.push_back(n);
}

void Parser::leaveParseNode() {
    if (!parseTreeStack.empty()) parseTreeStack.pop_back();
}

void Parser::addTerminalForToken(const Token& t) {
    if (parseTreeStack.empty()) return;
    parseTreeStack.back()->addChild(formatTerminal(t));
}

std::string Parser::formatTerminal(const Token& t) const {
    const std::string& tt = t.tokenType;
    if (tt == "Identifier") return "<ідентифікатор> " + t.tokenName;
    if (tt == "IntegerNumber") return "<ціле> " + t.tokenName;
    if (tt == "RealNumber") return "<дійсне> " + t.tokenName;
    if (tt == "BooleanConstant") return "<булева константа> " + t.tokenName;
    if (tt == "StringLiteral") return "<рядкова константа> " + t.tokenName;
    return "'" + t.tokenName + "'";
}

// --- Програма ---
ProgramNode* Parser::program() {
    ParseTreeScope scope(this, "<програма>");
    if (hadError) return nullptr;
    
    if (!match("TaskName")) { errorExp("'TASK_NAME'"); return nullptr; }
    
    std::string n = current().tokenName;
    if (!match("Identifier")) { errorExp("Program name"); return nullptr; }
    if (!match("Semicolon")) { errorExp("';'"); return nullptr; }
    
    ProgramNode* node = new ProgramNode(n);
    
    // Створюємо таблицю символів
    SymbolTable* symTable = new SymbolTable();
    
    if (!match("StartProgram")) { errorExp("'START'"); return nullptr; }
    
    if (check("Variable")) {
        match("Variable");
        ParseTreeScope declScope(this, "<оголошення змінних>");
        declaration_block(node, symTable);
    }
    
    // Зберігаємо таблицю символів в ProgramNode
    node->setSymbolTable(symTable);
    
    {
        ParseTreeScope bodyScope(this, "<тіло програми>");
        node->setBody(statement_list());
    }
    
    if (!match("StopProgram")) { errorExp("'STOP'"); return nullptr; }
    
    return node;
}

void Parser::declaration_block(ProgramNode* p, SymbolTable* symTable) {
    // declaration_block ::= { declaration }
    // { } означає 0 або більше повторень, тому блок може бути порожнім
    while (checkType() && !hadError) {
        declaration(p, symTable);
    }
    // Якщо checkType() повертає false, блок порожній - це дозволено
}

void Parser::declaration(ProgramNode* p, SymbolTable* symTable) {
    if (hadError) return;
    ParseTreeScope scope(this, "<оголошення типу>");
    
    std::string typeName = current().tokenName;
    {
        ParseTreeScope typeScope(this, "<тип>");
        std::string typeTok = current().tokenType;
        match(typeTok);
    }
    
    var_list(p, typeName, symTable);
    
    if (!match("Semicolon")) errorExp("';'");
}

void Parser::var_list(ProgramNode* p, const std::string& t, SymbolTable* symTable) {
    if (hadError) return;
    ParseTreeScope scope(this, "<список змінних>");
    var_item(p, t, symTable);
    while (match("Comma") && !hadError) {
        var_item(p, t, symTable);
    }
}

void Parser::var_item(ProgramNode* p, const std::string& t, SymbolTable* symTable) {
    if (hadError) return;
    ParseTreeScope scope(this, "<декларація>");
    
    if (!check("Identifier")) { errorExp("Identifier"); return; }
    
    std::string name = current().tokenName;
    match("Identifier");
    
    // Масив: identifier [ int_const ]
    if (match("LSquare")) {
        ParseTreeScope arrScope(this, "<оголошення масиву>");
        std::string szStr = current().tokenName;
        if (!match("IntegerNumber")) { errorExp("Array size (Integer)"); return; }
        if (!match("RSquare")) { errorExp("']'"); return; }
        
        int size = std::stoi(szStr);
        symTable->addSymbol(name, t, "Array", size);
        p->addVar(t + " " + name + "[" + szStr + "]");
    } else {
        symTable->addSymbol(name, t, "Variable", 1);
        p->addVar(t + " " + name);
    }
}

// --- Оператори ---
BlockNode* Parser::statement_list(const std::vector<std::string>& stopTokens) {
    BlockNode* block = new BlockNode();
    while (!isAtEnd() && !check("StopProgram") && !hadError) {
        // Перевіряємо, чи поточний токен є одним з токенів зупинки
        bool shouldStop = false;
        for (const auto& stopToken : stopTokens) {
            if (check(stopToken)) {
                shouldStop = true;
                break;
            }
        }
        if (shouldStop) break;
        
        StatementNode* s = statement();
        if (s) block->add(s);
        else if (hadError) return block;
    }
    return block;
}

StatementNode* Parser::statement() {
    if (hadError) return nullptr;
    
    if (check("Input")) return io_stmt();
    if (check("Output")) return io_stmt();
    if (check("If")) return if_stmt();
    if (check("Repeat")) return repeat_stmt();
    if (check("StartProgram")) return block_stmt();
    
    // Порожній оператор (тільки ;)
    if (check("Semicolon")) {
        ParseTreeScope scope(this, "<порожній оператор>");
        match("Semicolon");
        return nullptr;
    }
    
    // Присвоєння (expression -> target_access)
    // Можна почати з expression (identifier, number, bool, (, -, !!)
    if (check("Identifier") || check("IntegerNumber") || check("RealNumber") || 
        checkBoolConst() || check("LParen") || check("Sub") || check("Not")) {
        return assignment_stmt();
    }
    
    error("Unexpected token: " + current().tokenName);
    return nullptr;
}

BlockNode* Parser::block_stmt() {
    ParseTreeScope scope(this, "<складений оператор>");
    if (!match("StartProgram")) { errorExp("'START'"); return nullptr; }
    BlockNode* block = statement_list();
    if (!match("StopProgram")) { errorExp("'STOP'"); return nullptr; }
    return block;
}

StatementNode* Parser::assignment_stmt() {
    ParseTreeScope scope(this, "<присвоєння>");
    
    // Нова граматика: expression -> target_access
    ExpressionNode* expr = expression();
    if (!expr) { errorExp("Expression"); return nullptr; }
    
    if (!match("Assignment")) { errorExp("'->'"); return nullptr; }
    
    ExpressionNode* target = target_access();
    if (!target) { errorExp("Target"); return nullptr; }
    
    if (!match("Semicolon")) { errorExp("';'"); return nullptr; }
    
    // target_access повертає VarRefNode
    VarRefNode* varRef = dynamic_cast<VarRefNode*>(target);
    if (!varRef) { 
        error("Invalid assignment target"); 
        return nullptr; 
    }
    
    // Створюємо AssignNode з правильним порядком: name, idx, value
    return new AssignNode(varRef->name, varRef->idx, expr);
}

StatementNode* Parser::io_stmt() {
    if (check("Input")) return input_stmt();
    if (check("Output")) return output_stmt();
    return nullptr;
}

StatementNode* Parser::input_stmt() {
    ParseTreeScope scope(this, "<ввід>");
    if (!match("Input")) { errorExp("'READ'"); return nullptr; }
    if (!match("LParen")) { errorExp("'('"); return nullptr; }
    
    std::vector<ExpressionNode*> args;
    ExpressionNode* target = target_access();
    if (!target) { errorExp("Target access"); return nullptr; }
    args.push_back(target);
    
    while (match("Comma") && !hadError) {
        target = target_access();
        if (!target) { errorExp("Target access"); return nullptr; }
        args.push_back(target);
    }
    
    if (!match("RParen")) { errorExp("')'"); return nullptr; }
    if (!match("Semicolon")) { errorExp("';'"); return nullptr; }
    
    return new IO_Node("Read", args);
}

StatementNode* Parser::output_stmt() {
    ParseTreeScope scope(this, "<вивід>");
    if (!match("Output")) { errorExp("'WRITE'"); return nullptr; }
    if (!match("LParen")) { errorExp("'('"); return nullptr; }
    
    std::vector<ExpressionNode*> args;
    
    // write_arg ::= expression | string_literal
    if (check("StringLiteral")) {
        ExpressionNode* str = new StringNode(current().tokenName);
        match("StringLiteral");
        args.push_back(str);
    } else {
        ExpressionNode* expr = expression();
        if (!expr) { errorExp("Expression or string literal"); return nullptr; }
        args.push_back(expr);
    }
    
    while (match("Comma") && !hadError) {
        if (check("StringLiteral")) {
            ExpressionNode* str = new StringNode(current().tokenName);
            match("StringLiteral");
            args.push_back(str);
        } else {
            ExpressionNode* expr = expression();
            if (!expr) { errorExp("Expression or string literal"); return nullptr; }
            args.push_back(expr);
        }
    }
    
    if (!match("RParen")) { errorExp("')'"); return nullptr; }
    if (!match("Semicolon")) { errorExp("';'"); return nullptr; }
    
    return new IO_Node("Write", args);
}

StatementNode* Parser::if_stmt() {
    ParseTreeScope scope(this, "<умовний оператор>");
    if (!match("If")) { errorExp("'IF'"); return nullptr; }
    
    ExpressionNode* cond = boolean_expr();
    if (!cond) { errorExp("Boolean expression"); return nullptr; }
    
    if (!match("Then")) { errorExp("'THEN'"); return nullptr; }
    
    StatementNode* thenStmt = statement();
    if (!thenStmt) { errorExp("Statement"); return nullptr; }
    
    StatementNode* elseStmt = nullptr;
    if (match("Else")) {
        elseStmt = statement();
        if (!elseStmt) { errorExp("Statement"); return nullptr; }
    }
    
    return new IfNode(cond, thenStmt, elseStmt);
}

StatementNode* Parser::repeat_stmt() {
    ParseTreeScope scope(this, "<цикл>");
    if (!match("Repeat")) { errorExp("'REPEAT'"); return nullptr; }
    
    // statement_list має зупинитися на UNTIL
    BlockNode* body = statement_list({"Until"});
    if (!body) { errorExp("Statement list"); return nullptr; }
    
    if (!match("Until")) { errorExp("'UNTIL'"); return nullptr; }
    
    ExpressionNode* condition = boolean_expr();
    if (!condition) { errorExp("Boolean expression"); return nullptr; }
    
    if (!match("Semicolon")) { errorExp("';'"); return nullptr; }
    
    return new RepeatNode(body, condition);
}

// --- Вирази ---
ExpressionNode* Parser::boolean_expr() {
    ParseTreeScope scope(this, "<логічний вираз>");
    if (hadError) return nullptr;
    
    ExpressionNode* l = boolean_term();
    while (check("Or")) {
        match("Or");
        ExpressionNode* r = boolean_term();
        if (r) l = new BinaryOpNode("||", l, r);
    }
    return l;
}

ExpressionNode* Parser::boolean_term() {
    ParseTreeScope scope(this, "<логічний терм>");
    if (hadError) return nullptr;
    
    ExpressionNode* l = relation();
    while (check("And")) {
        match("And");
        ExpressionNode* r = relation();
        if (r) l = new BinaryOpNode("&&", l, r);
    }
    return l;
}

ExpressionNode* Parser::relation() {
    ParseTreeScope scope(this, "<відношення>");
    if (hadError) return nullptr;
    
    ExpressionNode* l = expression();
    if (checkRelOp()) {
        std::string op = current().tokenName;
        match(current().tokenType);
        ExpressionNode* r = expression();
        if (r) {
            return new BinaryOpNode(op, l, r);
        }
    }
    return l;
}

ExpressionNode* Parser::expression() {
    ParseTreeScope scope(this, "<арифметичний вираз>");
    if (hadError) return nullptr;
    
    ExpressionNode* l = term();
    while (check("Add") || check("Sub")) {
        std::string op = current().tokenName;
        match(current().tokenType);
        ExpressionNode* r = term();
        if (r) l = new BinaryOpNode(op, l, r);
    }
    return l;
}

ExpressionNode* Parser::term() {
    ParseTreeScope scope(this, "<доданок>");
    if (hadError) return nullptr;
    
    ExpressionNode* l = factor();
    while (check("Multiply") || check("Divide") || check("DivideRemainder")) {
        std::string op = current().tokenName;
        match(current().tokenType);
        ExpressionNode* r = factor();
        if (r) l = new BinaryOpNode(op, l, r);
    }
    return l;
}

ExpressionNode* Parser::factor() {
    ParseTreeScope scope(this, "<множник>");
    if (hadError) return nullptr;
    
    // Унарні операції
    if (check("Sub")) {
        match("Sub");
        ExpressionNode* f = factor();
        if (f) return new UnaryOpNode("-", f);
        return nullptr;
    }
    if (check("Not")) {
        match("Not");
        ExpressionNode* f = factor();
        if (f) return new UnaryOpNode("!!", f);
        return nullptr;
    }
    
    // Дужки з boolean_expr
    if (match("LParen")) {
        ExpressionNode* e = boolean_expr();
        if (!match("RParen")) { errorExp("')'"); return nullptr; }
        return e;
    }
    
    // Константи
    if (check("IntegerNumber") || check("RealNumber")) {
        std::string v = current().tokenName;
        match(current().tokenType);
        return new NumberNode(v);
    }
    
    // Булеві константи
    if (checkBoolConst()) {
        std::string v = current().tokenName;
        match("BooleanConstant");
        return new BoolNode(v == "TRUE" ? "1" : "0");
    }
    
    // target_access
    if (check("Identifier")) {
        return target_access();
    }
    
    errorExp("Expression factor");
    return nullptr;
}

ExpressionNode* Parser::target_access() {
    ParseTreeScope scope(this, "<адреса>");
    if (hadError) return nullptr;
    
    if (!check("Identifier")) { errorExp("Identifier"); return nullptr; }
    
    std::string name = current().tokenName;
    match("Identifier");
    
    if (match("LSquare")) {
        ExpressionNode* idx = expression();
        if (!match("RSquare")) { errorExp("']'"); return nullptr; }
        return new VarRefNode(name, idx);
    }
    
    return new VarRefNode(name);
}


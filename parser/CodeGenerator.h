#pragma once
#include "AST.h"
#include "SymbolTable.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

class CodeGenerator {
public:
    explicit CodeGenerator(SymbolTable* symTable) : symbolTable(symTable), labelCounter(0) {}
    
    // Генерація програми
    void generate(ProgramNode* program, const std::string& outputFile);
    
private:
    SymbolTable* symbolTable;
    std::ofstream out;
    int labelCounter;
    
    // Генерація унікальної мітки
    std::string newLabel(const std::string& prefix);
    
    // Генерація коду для вузлів AST
    void generateProgram(ProgramNode* node);
    void generateStatement(StatementNode* stmt);
    void generateExpression(ExpressionNode* expr);
    void generateBlock(BlockNode* block);
    
    // Генерація конкретних конструкцій
    void generateAssign(AssignNode* node);
    void generateIO(IO_Node* node);
    void generateIf(IfNode* node);
    void generateRepeat(RepeatNode* node);
    
    // Генерація виразів
    std::string generateExpr(ExpressionNode* expr);
    std::string generateTarget(VarRefNode* varRef);
    
    // Допоміжні функції
    std::string getCType(const std::string& type);
    std::string getCVarName(const std::string& name);
    std::string getExprType(ExpressionNode* expr);
};


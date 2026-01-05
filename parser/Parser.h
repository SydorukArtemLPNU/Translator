#pragma once
#include "LexicalAnalyzer.h"
#include "AST.h"
#include <vector>
#include <string>
#include <initializer_list>

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    ProgramNode* parse();
    const std::vector<std::string>& getErrors() const { return errors; }
    const ParseTreeNode* getParseTreeRoot() const { return parseTreeRoot.get(); }

private:
    std::vector<Token> tokens;
    size_t pos;
    std::vector<std::string> errors;
    bool hadError;

    bool isAtEnd() const;
    const Token& current() const;
    void advance();
    bool match(const std::string& type);
    bool check(const std::string& type) const;
    bool checkType() const;
    bool checkBoolConst() const;
    bool checkRelOp() const;

    void error(const std::string& msg);
    void errorExp(const std::string& exp);
    void sync();

    // Parse tree building
    std::unique_ptr<ParseTreeNode> parseTreeRoot;
    std::vector<ParseTreeNode*> parseTreeStack;

    struct ParseTreeScope {
        Parser* p;
        explicit ParseTreeScope(Parser* parser, const std::string& label) : p(parser) { p->enterParseNode(label); }
        ~ParseTreeScope() { if (p) p->leaveParseNode(); }
    };

    void enterParseNode(const std::string& label);
    void leaveParseNode();
    void addTerminalForToken(const Token& t);
    std::string formatTerminal(const Token& t) const;

    ProgramNode* program();
    void declaration_block(ProgramNode* p, class SymbolTable* symTable);
    void declaration(ProgramNode* p, class SymbolTable* symTable);
    void var_list(ProgramNode* p, const std::string& t, class SymbolTable* symTable);
    void var_item(ProgramNode* p, const std::string& t, class SymbolTable* symTable);

    BlockNode* statement_list(const std::vector<std::string>& stopTokens = {});
    StatementNode* statement();
    BlockNode* block_stmt();

    StatementNode* assignment_stmt();
    StatementNode* io_stmt();
    StatementNode* input_stmt();
    StatementNode* output_stmt();
    StatementNode* if_stmt();
    StatementNode* repeat_stmt();

    ExpressionNode* boolean_expr();
    ExpressionNode* boolean_term();
    ExpressionNode* relation();
    ExpressionNode* expression();
    ExpressionNode* term();
    ExpressionNode* factor();
    ExpressionNode* target_access();
};


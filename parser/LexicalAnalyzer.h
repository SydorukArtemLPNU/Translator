#pragma once

#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

struct Token {
    std::string tokenName;
    int lineNumber;
    int tokenCode;
    std::string tokenType;
    std::string tokenOut;
};

class LexicalAnalyzer {
public:
    void analyze(const std::string& filename);
    void printTokenTable() const;
    void printErrorSummary() const;
    void writeTokenTableToFile(const std::string& filename) const;
    void writeErrorsToFile(const std::string& filename) const;

    const std::vector<Token>& getTokens() const { return tokens; }
    const std::vector<std::string>& getErrors() const { return errorLog; }

private:
    bool getNextChar();
    char peekNextChar();
    void processLiteral();
    void processNumber();
    void processKeywordOrOperator();
    void processIdentifierOrKeywordOrOperator();
    void processOperatorOrDelimiter();
    void processComment();

    void addToken(const std::string& tokenName, int code, const std::string& type, const std::string& tokenOut);
    void logError(const std::string& msg);
    void printLiteralsTable() const;
    void printIdentifiersTable() const;
    void resetState();

    // Нові ключові слова відповідно до EBNF
    const std::map<std::string, std::pair<int, std::string>> keywords = {
        {"TASK_NAME", {100, "TaskName"}},
        {"START", {101, "StartProgram"}},
        {"STOP", {107, "StopProgram"}},
        {"VARIABLE", {102, "Variable"}},
        {"INT16T", {105, "IntType"}},
        {"FLOAT32", {106, "FloatType"}},
        {"BOOL", {110, "BoolType"}},
        {"IF", {108, "If"}},
        {"THEN", {109, "Then"}},
        {"ELSE", {114, "Else"}},
        {"REPEAT", {115, "Repeat"}},
        {"UNTIL", {116, "Until"}},
        {"READ", {103, "Input"}},
        {"WRITE", {104, "Output"}},
        {"DIV", {404, "Divide"}},
        {"MOD", {405, "DivideRemainder"}},
        {"TRUE", {303, "BooleanConstant"}},
        {"FALSE", {304, "BooleanConstant"}}
    };

    const std::map<std::string, std::pair<int, std::string>> operators = {
        {"->", {400, "Assignment"}},
        {"+", {401, "Add"}},
        {"-", {402, "Sub"}},
        {"*", {403, "Multiply"}},
        {"DIV", {404, "Divide"}},
        {"MOD", {405, "DivideRemainder"}},
        {"==", {406, "Equal"}},
        {"!=", {407, "NotEqual"}},
        {">", {408, "GreaterThan"}},
        {"<", {409, "LessThan"}},
        {">=", {410, "GreaterEqual"}},
        {"<=", {411, "LessEqual"}},
        {"&&", {412, "And"}},
        {"||", {413, "Or"}},
        {"!!", {414, "Not"}}
    };

    const std::map<std::string, std::pair<int, std::string>> delimiters = {
        {";", {500, "Semicolon"}},
        {",", {501, "Comma"}},
        {"(", {503, "LParen"}},
        {")", {504, "RParen"}},
        {"[", {505, "LSquare"}},
        {"]", {506, "RSquare"}}
    };

    std::ifstream inputFile;
    std::vector<Token> tokens;
    std::vector<std::pair<char, std::string>> literals;
    std::vector<std::string> identifiers;
    std::vector<std::string> errorLog;
    char currentChar = '\0';
    int currentLine = 1;
};


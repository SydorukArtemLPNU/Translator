#include "LexicalAnalyzer.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

namespace {
    bool isIdentifierStart(char ch) {
        return std::islower(static_cast<unsigned char>(ch));
    }
    
    // Після першого символу (малої літери) можуть бути тільки великі літери або цифри
    bool isIdentifierChar(char ch) {
        return std::isupper(static_cast<unsigned char>(ch)) || std::isdigit(static_cast<unsigned char>(ch));
    }
    
    // Перевірка чи символ може бути частиною ключового слова (великі літери, цифри, підкреслення)
    bool isKeywordChar(char ch) {
        return std::isupper(static_cast<unsigned char>(ch)) || 
               std::isdigit(static_cast<unsigned char>(ch)) || 
               ch == '_';
    }
}

void LexicalAnalyzer::analyze(const std::string& filename) {
    resetState();
    inputFile.open(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }
    while (getNextChar()) {
        if (std::isspace(static_cast<unsigned char>(currentChar))) {
            if (currentChar == '\n') ++currentLine;
            continue;
        }
        if (currentChar == '/' && peekNextChar() == '/') {
            getNextChar(); processComment(); continue;
        }
        if (currentChar == '\"') {
            processLiteral(); continue;
        }
        if (std::isdigit(static_cast<unsigned char>(currentChar))) {
            processNumber(); continue;
        }
        // Спочатку перевіряємо ключові слова (великі літери)
        if (std::isupper(static_cast<unsigned char>(currentChar))) {
            processKeywordOrOperator(); continue;
        }
        // Потім ідентифікатори (малі літери)
        if (isIdentifierStart(currentChar)) {
            processIdentifierOrKeywordOrOperator(); continue;
        }
        processOperatorOrDelimiter();
    }
    inputFile.close();
}

void LexicalAnalyzer::printTokenTable() const {
    if (tokens.empty()) {
        std::cout << "Лексем не знайдено." << std::endl;
        return;
    }
    const std::string headers[] = { "Рядок", "Лексема", "Значення (Посилання)", "Код", "Тип" };
    const int COL = 5;
    int widths[COL];
    for (int i = 0; i < COL; ++i) widths[i] = headers[i].length();

    for (const auto& t : tokens) {
        std::string val = "-";
        if (t.tokenCode == 200) {
            auto it = std::find(identifiers.begin(), identifiers.end(), t.tokenName);
            if (it != identifiers.end()) val = "@id" + std::to_string(std::distance(identifiers.begin(), it));
        }
        else if (t.tokenCode == 302) val = "@" + t.tokenName;
        else if (t.tokenCode == 300 || t.tokenCode == 301) val = t.tokenName;

        widths[0] = std::max(widths[0], (int)std::to_string(t.lineNumber).length());
        widths[1] = std::max(widths[1], (int)t.tokenName.length());
        widths[2] = std::max(widths[2], (int)val.length());
        widths[3] = std::max(widths[3], (int)std::to_string(t.tokenCode).length());
        widths[4] = std::max(widths[4], (int)t.tokenType.length());
    }
    for (int i = 0; i < COL; ++i) widths[i] += 2;

    auto sep = [&]() {
        int sum = 1; for (int w : widths) sum += w + 1;
        std::cout << std::string(sum, '-') << std::endl;
        };

    sep();
    std::cout << "|";
    for (int i = 0; i < COL; ++i) std::cout << std::left << std::setw(widths[i]) << (" " + headers[i]) << "|";
    std::cout << "\n";
    sep();

    for (const auto& t : tokens) {
        std::string val = "-";
        if (t.tokenCode == 200) {
            auto it = std::find(identifiers.begin(), identifiers.end(), t.tokenName);
            if (it != identifiers.end()) val = "@id" + std::to_string(std::distance(identifiers.begin(), it));
        }
        else if (t.tokenCode == 302) val = "@" + t.tokenName;
        else if (t.tokenCode == 300 || t.tokenCode == 301) val = t.tokenName;

        std::cout << "|";
        std::cout << std::left << std::setw(widths[0]) << (" " + std::to_string(t.lineNumber)) << "|";
        std::cout << std::left << std::setw(widths[1]) << (" " + t.tokenName) << "|";
        std::cout << std::left << std::setw(widths[2]) << (" " + val) << "|";
        std::cout << std::left << std::setw(widths[3]) << (" " + std::to_string(t.tokenCode)) << "|";
        std::cout << std::left << std::setw(widths[4]) << (" " + t.tokenType) << "|";
        std::cout << "\n";
    }
    sep();
    printLiteralsTable();
    printIdentifiersTable();
}

void LexicalAnalyzer::printErrorSummary() const {
    std::cout << "\n--- Лексичні помилки ---\n";
    if (errorLog.empty()) std::cout << "Лексичних помилок не знайдено.\n";
    else for (const auto& err : errorLog) std::cout << err << std::endl;
    std::cout << "-------------------------\n";
}

void LexicalAnalyzer::logError(const std::string& msg) {
    errorLog.push_back("Error (Line " + std::to_string(currentLine) + "): " + msg);
}

bool LexicalAnalyzer::getNextChar() {
    int next = inputFile.get();
    if (next == std::char_traits<char>::eof()) { currentChar = '\0'; return false; }
    currentChar = static_cast<char>(next); return true;
}

char LexicalAnalyzer::peekNextChar() {
    int next = inputFile.peek();
    if (next == std::char_traits<char>::eof()) return '\0';
    return static_cast<char>(next);
}

void LexicalAnalyzer::processLiteral() {
    const char quoteChar = currentChar;
    std::string literalValue;

    while (getNextChar()) {
        if (currentChar == quoteChar) {
            std::size_t literalIndex = literals.size();
            literals.emplace_back(quoteChar, literalValue);
            addToken("l" + std::to_string(literalIndex), 302, "StringLiteral", "LiteralOut");
            return;
        }
        if (currentChar == '\r') continue;
        if (currentChar == '\n') {
            ++currentLine;
            logError("String literal not closed (unexpected newline)");
            addToken("ErrorStr", 999, "Unknown", "LiteralOut");
            return;
        }
        literalValue.push_back(currentChar);
    }

    logError("String literal not closed (unexpected EOF)");
    addToken("ErrorStr", 999, "Unknown", "LiteralOut");
}

void LexicalAnalyzer::processNumber() {
    std::string buf; buf += currentChar;
    bool real = false;
    while (std::isdigit((unsigned char)peekNextChar())) { getNextChar(); buf += currentChar; }
    if (peekNextChar() == '.') {
        getNextChar(); buf += currentChar; real = true;
        if (!std::isdigit((unsigned char)peekNextChar())) {
            logError("Invalid float"); addToken(buf, 999, "Unk", "Num"); return;
        }
        while (std::isdigit((unsigned char)peekNextChar())) { getNextChar(); buf += currentChar; }
    }
    if (real) addToken(buf, 301, "RealNumber", "NumOut");
    else addToken(buf, 300, "IntegerNumber", "NumOut");
}

void LexicalAnalyzer::processKeywordOrOperator() {
    // Обробка ключових слів та операторів, які починаються з великої літери
    std::string buf; buf += currentChar;
    
    // Читаємо послідовність великих літер, цифр та підкреслень
    while (isKeywordChar(peekNextChar())) {
        getNextChar();
        buf += currentChar;
    }
    
    // Перевіряємо ключові слова (мають пріоритет)
    if (keywords.count(buf)) {
        addToken(buf, keywords.at(buf).first, keywords.at(buf).second, "KeyOut");
        return;
    }
    
    // Перевіряємо оператори (DIV, MOD)
    if (operators.count(buf)) {
        addToken(buf, operators.at(buf).first, operators.at(buf).second, "OpOut");
        return;
    }
    
    // Якщо не ключове слово і не оператор - помилка
    addToken(buf, 999, "Unknown", "KeyOut");
    logError("Unknown keyword or operator: " + buf);
}

void LexicalAnalyzer::processIdentifierOrKeywordOrOperator() {
    // Обробка ідентифікаторів, які починаються з малої літери
    std::string buf; buf += currentChar;
    
    // Читаємо всі допустимі символи для ідентифікатора
    while (isIdentifierChar(peekNextChar())) {
        getNextChar();
        buf += currentChar;
        // Перевіряємо максимальну довжину 8 символів
        if (buf.length() > 8) {
            logError("Identifier too long (max 8 characters): " + buf);
            // Продовжуємо читати до кінця для кращої діагностики
            while (isIdentifierChar(peekNextChar())) {
                getNextChar();
                buf += currentChar;
            }
            addToken(buf, 999, "Unknown", "IdOut");
            return;
        }
    }

    // Перевірка валідності ідентифікатора: 
    // - починається з малої літери
    // - решта символів - великі літери або цифри
    // - максимум 8 символів
    bool isValid = buf.length() > 0 && 
                   buf.length() <= 8 && 
                   std::islower(static_cast<unsigned char>(buf[0]));
    
    // Перевіряємо, що всі символи після першого - великі літери або цифри
    for (size_t i = 1; i < buf.length(); ++i) {
        char c = buf[i];
        if (!std::isupper(static_cast<unsigned char>(c)) && !std::isdigit(static_cast<unsigned char>(c))) {
            isValid = false;
            break;
        }
    }
    
    if (isValid) {
        addToken(buf, 200, "Identifier", "IdOut");
    } else {
        addToken(buf, 999, "Unknown", "IdOut");
        if (buf.length() > 8) {
            logError("Identifier too long (max 8 characters): " + buf);
        } else if (buf.length() == 0 || !std::islower(static_cast<unsigned char>(buf[0]))) {
            logError("Invalid identifier (must start with lowercase letter): " + buf);
        } else {
            logError("Invalid identifier (after first letter, only uppercase letters or digits allowed): " + buf);
        }
    }
}

void LexicalAnalyzer::processOperatorOrDelimiter() {
    std::string op(1, currentChar);
    char nc = peekNextChar();
    
    // Спеціальна обробка для дво-символьних операторів
    if (nc != '\0') {
        std::string two = op + nc;
        // Перевіряємо дво-символьні оператори (включаючи !!)
        if (operators.count(two)) { 
            getNextChar(); 
            op = two; 
        } else if (delimiters.count(two)) {
            getNextChar();
            op = two;
        }
    }
    
    if (operators.count(op)) addToken(op, operators.at(op).first, operators.at(op).second, "OpOut");
    else if (delimiters.count(op)) addToken(op, delimiters.at(op).first, delimiters.at(op).second, "DelimOut");
    else { 
        addToken(op, 999, "Unknown", "DelimOut"); 
        logError("Unknown symbol: " + op); 
    }
}

void LexicalAnalyzer::processComment() {
    // Коментарі мають формат // ... //
    // Поточний символ вже другий '/' (ми вже прочитали два //)
    // Тепер читаємо вміст до закриваючого //
    char prev = '\0';
    
    while (getNextChar()) {
        if (currentChar == '\n') ++currentLine;
        if (prev == '/' && currentChar == '/') {
            // Знайшли закриваючий //
            return;
        }
        prev = currentChar;
    }
    
    // Якщо досягли кінця файлу без закриваючого //
    logError("Comment not closed (expected // to close)");
}

void LexicalAnalyzer::addToken(const std::string& name, int code, const std::string& type, const std::string& out) {
    tokens.push_back(Token{ name, currentLine, code, type, out });
    if (code == 200) {
        if (std::find(identifiers.begin(), identifiers.end(), name) == identifiers.end()) identifiers.push_back(name);
    }
}

void LexicalAnalyzer::printLiteralsTable() const {
    if (literals.empty()) return;
    std::cout << "\nТаблиця літералів:\n--------------------------\n";
    std::cout << "Індекс | Літерал\n--------------------------\n";
    for (size_t i = 0; i < literals.size(); ++i) {
        std::cout << "l" << i << "    | " << literals[i].second << "\n";
    }
    std::cout << "--------------------------\n";
}

void LexicalAnalyzer::printIdentifiersTable() const {
    if (identifiers.empty()) return;
    std::cout << "\nТаблиця ідентифікаторів:\n";
    for (size_t i = 0; i < identifiers.size(); ++i) std::cout << "id" << i << ": " << identifiers[i] << "\n";
}

void LexicalAnalyzer::writeTokenTableToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << filename << std::endl;
        return;
    }

    if (tokens.empty()) {
        out << "Лексем не знайдено." << std::endl;
        out.close();
        return;
    }

    const std::string headers[] = { "Рядок", "Лексема", "Значення (Посилання)", "Код", "Тип" };
    const int COL = 5;
    int widths[COL];
    for (int i = 0; i < COL; ++i) widths[i] = headers[i].length();

    for (const auto& t : tokens) {
        std::string val = "-";
        if (t.tokenCode == 200) {
            auto it = std::find(identifiers.begin(), identifiers.end(), t.tokenName);
            if (it != identifiers.end()) val = "@id" + std::to_string(std::distance(identifiers.begin(), it));
        }
        else if (t.tokenCode == 302) val = "@" + t.tokenName;
        else if (t.tokenCode == 300 || t.tokenCode == 301) val = t.tokenName;

        widths[0] = std::max(widths[0], (int)std::to_string(t.lineNumber).length());
        widths[1] = std::max(widths[1], (int)t.tokenName.length());
        widths[2] = std::max(widths[2], (int)val.length());
        widths[3] = std::max(widths[3], (int)std::to_string(t.tokenCode).length());
        widths[4] = std::max(widths[4], (int)t.tokenType.length());
    }
    for (int i = 0; i < COL; ++i) widths[i] += 2;

    auto sep = [&]() {
        int sum = 1; for (int w : widths) sum += w + 1;
        out << std::string(sum, '-') << std::endl;
    };

    sep();
    out << "|";
    for (int i = 0; i < COL; ++i) out << std::left << std::setw(widths[i]) << (" " + headers[i]) << "|";
    out << "\n";
    sep();

    for (const auto& t : tokens) {
        std::string val = "-";
        if (t.tokenCode == 200) {
            auto it = std::find(identifiers.begin(), identifiers.end(), t.tokenName);
            if (it != identifiers.end()) val = "@id" + std::to_string(std::distance(identifiers.begin(), it));
        }
        else if (t.tokenCode == 302) val = "@" + t.tokenName;
        else if (t.tokenCode == 300 || t.tokenCode == 301) val = t.tokenName;

        out << "|";
        out << std::left << std::setw(widths[0]) << (" " + std::to_string(t.lineNumber)) << "|";
        out << std::left << std::setw(widths[1]) << (" " + t.tokenName) << "|";
        out << std::left << std::setw(widths[2]) << (" " + val) << "|";
        out << std::left << std::setw(widths[3]) << (" " + std::to_string(t.tokenCode)) << "|";
        out << std::left << std::setw(widths[4]) << (" " + t.tokenType) << "|";
        out << "\n";
    }
    sep();

    // Таблиця літералів
    if (!literals.empty()) {
        out << "\nТаблиця літералів:\n--------------------------\n";
        out << "Індекс | Літерал\n--------------------------\n";
        for (size_t i = 0; i < literals.size(); ++i) {
            out << "l" << i << "    | " << literals[i].second << "\n";
        }
        out << "--------------------------\n";
    }

    // Таблиця ідентифікаторів
    if (!identifiers.empty()) {
        out << "\nТаблиця ідентифікаторів:\n";
        for (size_t i = 0; i < identifiers.size(); ++i) {
            out << "id" << i << ": " << identifiers[i] << "\n";
        }
    }

    out.close();
}

void LexicalAnalyzer::writeErrorsToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Помилка: Неможливо відкрити файл для запису: " << filename << std::endl;
        return;
    }

    out << "=== Лексичні помилки ===\n";
    if (errorLog.empty()) {
        out << "Лексичних помилок не знайдено.\n";
    } else {
        for (const auto& err : errorLog) {
            out << err << "\n";
        }
    }
    out << "========================\n";

    out.close();
}

void LexicalAnalyzer::resetState() {
    tokens.clear(); literals.clear(); identifiers.clear(); errorLog.clear(); currentChar = '\0'; currentLine = 1;
    if (inputFile.is_open()) inputFile.close();
}


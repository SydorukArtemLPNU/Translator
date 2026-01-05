#include "LexicalAnalyzer.h"
#include "Parser.h"
#include "CodeGenerator.h"
#include "SymbolTable.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <limits>
#include <fstream>

namespace fs = std::filesystem;

static bool isProgD06(const fs::directory_entry& e) {
    if (!e.is_regular_file()) return false;
    const fs::path p = e.path();
    return p.extension() == ".s20";
}

static void processFile(const std::string& filename) {
    LexicalAnalyzer analyzer;
    std::cout << "\n==============================\n";
    std::cout << "ФАЙЛ: " << filename << "\n";
    std::cout << "==============================\n";

    // Створюємо папку з назвою файлу (без розширення)
    fs::path inputPath(filename);
    std::string outputDirName = inputPath.stem().string();
    fs::path outputDir = fs::current_path() / outputDirName;
    
    // Створюємо папку, якщо її немає
    if (!fs::exists(outputDir)) {
        fs::create_directory(outputDir);
        std::cout << "Створено папку виводу: " << outputDir << "\n";
    }

    std::cout << "\n=== Лексичний аналіз ===\n";
    analyzer.analyze(filename);
    analyzer.printTokenTable();
    analyzer.printErrorSummary();

    // Записуємо лексеми в файл
    fs::path lexemesFile = outputDir / (outputDirName + "_lexemes.txt");
    analyzer.writeTokenTableToFile(lexemesFile.string());
    std::cout << "Лексеми записані до: " << lexemesFile << "\n";

    std::cout << "\n=== Синтаксичний аналіз ===\n";
    Parser parser(analyzer.getTokens());

    ProgramNode* root = parser.parse();

    // Збираємо всі помилки (лексемні + синтаксичні)
    fs::path errorsFile = outputDir / (outputDirName + "_errors.txt");
    std::ofstream errorsOut(errorsFile.string());
    if (errorsOut.is_open()) {
        errorsOut << "=== Звіт про помилки трансляції ===\n\n";
        
        // Лексичні помилки
        errorsOut << "--- Лексичні помилки ---\n";
        const auto& lexErrors = analyzer.getErrors();
        if (lexErrors.empty()) {
            errorsOut << "Лексичних помилок не знайдено.\n";
        } else {
            for (const auto& err : lexErrors) {
                errorsOut << err << "\n";
            }
        }
        errorsOut << "\n";
        
        // Синтаксичні помилки
        errorsOut << "--- Синтаксичні помилки ---\n";
        const auto& syntaxErrors = parser.getErrors();
        if (syntaxErrors.empty()) {
            errorsOut << "Синтаксичних помилок не знайдено.\n";
        } else {
            for (const auto& err : syntaxErrors) {
                errorsOut << err << "\n";
            }
        }
        errorsOut << "\n";
        
        // Підсумок
        if (lexErrors.empty() && syntaxErrors.empty()) {
            errorsOut << "=== ПІДСУМОК ===\n";
            errorsOut << "Помилок не знайдено. Трансляція успішна.\n";
        } else {
            errorsOut << "=== ПІДСУМОК ===\n";
            errorsOut << "Трансляція завершена з помилками.\n";
        }
        errorsOut.close();
        std::cout << "Помилки записані до: " << errorsFile << "\n";
    }

    if (parser.getErrors().empty() && root) {
        std::cout << "[УСПІХ] Синтаксично коректно.\n\n";

        // 0. Concrete parse tree - вивід в файл
        if (const ParseTreeNode* pt = parser.getParseTreeRoot()) {
            std::cout << "=== Дерево розбору ===\n";
            pt->print();
            std::cout << "\n";
            
            // Записуємо дерево розбору в файл
            fs::path parseTreeFile = outputDir / (outputDirName + "_parse_tree.txt");
            std::ofstream parseTreeOut(parseTreeFile.string());
            if (parseTreeOut.is_open()) {
                parseTreeOut << "=== Дерево розбору ===\n\n";
                pt->printToFile(parseTreeOut);
                parseTreeOut.close();
                std::cout << "Дерево розбору записано до: " << parseTreeFile << "\n";
            }
        }

        // 1. Refined table
        root->printRefinedTable();
        
        // Записуємо уточнену таблицю ідентифікаторів в файл
        fs::path refinedTableFile = outputDir / (outputDirName + "_refined_table.txt");
        std::ofstream refinedTableOut(refinedTableFile.string());
        if (refinedTableOut.is_open()) {
            root->writeRefinedTableToFile(refinedTableOut);
            refinedTableOut.close();
            std::cout << "Уточнена таблиця ідентифікаторів записана до: " << refinedTableFile << "\n";
        }
        
        // 2. Symbol table
        if (SymbolTable* symTable = root->getSymbolTable()) {
            symTable->printTable();
            
            // Записуємо таблицю символів в файл
            fs::path symbolTableFile = outputDir / (outputDirName + "_symbol_table.txt");
            std::ofstream symbolTableOut(symbolTableFile.string());
            if (symbolTableOut.is_open()) {
                symTable->writeTableToFile(symbolTableOut);
                symbolTableOut.close();
                std::cout << "Таблиця символів записана до: " << symbolTableFile << "\n";
            }
        }
        
        // 3. Генерація коду C (в папку)
        std::cout << "\n=== Генерація коду ===\n";
        fs::path outputFile = outputDir / (outputDirName + ".c");
        if (SymbolTable* symTable = root->getSymbolTable()) {
            CodeGenerator generator(symTable);
            generator.generate(root, outputFile.string());
            std::cout << "Згенеровано код C: " << outputFile << "\n";
        } else {
            std::cout << "Попередження: Таблиця символів недоступна, пропущено генерацію коду\n";
        }
    }
    else {
        std::cout << "[ПОМИЛКА] Синтаксичні помилки:\n";
        for (const auto& e : parser.getErrors()) std::cout << e << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    
    // Якщо передано аргумент командного рядка - використовуємо його
    if (argc >= 2) {
        inputFile = argv[1];
        // Перевіряємо, чи файл існує
        if (!fs::exists(inputFile) || !fs::is_regular_file(inputFile)) {
            std::cerr << "Помилка: Файл '" << inputFile << "' не знайдено\n";
            return 1;
        }
        // Перевіряємо розширення
        if (fs::path(inputFile).extension() != ".s20") {
            std::cerr << "Помилка: Файл повинен мати розширення .s20\n";
            return 1;
        }
        processFile(inputFile);
        return 0;
    }
    
    // Інакше - інтерактивний вибір
    std::vector<std::string> files;
    for (const auto& e : fs::directory_iterator(fs::current_path())) {
        if (isProgD06(e)) files.push_back(e.path().string());
    }
    std::sort(files.begin(), files.end());

    if (files.empty()) {
        std::cout << "Вхідні файли не знайдено. Очікувані файли з розширенням '.s20' в: "
                  << fs::current_path().string() << "\n";
        return 1;
    }

    std::cout << "Знайдено " << files.size() << " файл(ів):\n";
    for (size_t i = 0; i < files.size(); ++i) {
        std::cout << "  " << (i + 1) << ") " << fs::path(files[i]).filename().string() << "\n";
    }

    size_t choice = 0;
    while (true) {
        std::cout << "\nВиберіть номер файлу (1-" << files.size() << ", 0 для виходу): ";
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (choice == 0) return 0;
        if (choice >= 1 && choice <= files.size()) break;
    }

    processFile(files[choice - 1]);
    return 0;
}


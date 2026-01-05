#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>

struct SymbolEntry {
    std::string name;
    std::string type;           // INT16T, FLOAT32, BOOL
    std::string category;       // Variable, Array
    int size;                   // 1 для змінної, N для масиву
    int address;                // Адреса/зміщення для генерації коду
    
    SymbolEntry(const std::string& n, const std::string& t, const std::string& c, int s, int addr)
        : name(n), type(t), category(c), size(s), address(addr) {}
};

class SymbolTable {
public:
    // Додати символ до таблиці
    void addSymbol(const std::string& name, const std::string& type, const std::string& category, int size) {
        int address = calculateNextAddress(type, size);
        symbols.push_back(SymbolEntry(name, type, category, size, address));
        nameToIndex[name] = symbols.size() - 1;
    }
    
    // Знайти символ за іменем
    SymbolEntry* lookup(const std::string& name) {
        auto it = nameToIndex.find(name);
        if (it != nameToIndex.end()) {
            return &symbols[it->second];
        }
        return nullptr;
    }
    
    // Отримати адресу символу
    int getAddress(const std::string& name) {
        SymbolEntry* entry = lookup(name);
        return entry ? entry->address : -1;
    }
    
    // Перевірити чи символ існує
    bool exists(const std::string& name) const {
        return nameToIndex.find(name) != nameToIndex.end();
    }
    
    // Вивести таблицю символів
    void printTable() const {
        std::cout << "\n=== Таблиця символів ===\n";
        std::cout << "-----------------------------------------------------\n";
        std::cout << "| Ім'я       | Тип       | Категорія | Розмір | Адреса  |\n";
        std::cout << "-----------------------------------------------------\n";
        
        for (const auto& sym : symbols) {
            std::string category = (sym.category == "Variable") ? "Змінна" : "Масив";
            std::cout << "| " << std::left << std::setw(11) << sym.name
                << "| " << std::setw(10) << sym.type
                << "| " << std::setw(9) << category
                << "| " << std::setw(7) << sym.size
                << "| " << std::setw(9) << sym.address << "|\n";
        }
        std::cout << "-----------------------------------------------------\n";
    }

    void writeTableToFile(std::ostream& out) const {
        out << "\n=== Таблиця символів ===\n";
        out << "-----------------------------------------------------\n";
        out << "| Ім'я       | Тип       | Категорія | Розмір | Адреса  |\n";
        out << "-----------------------------------------------------\n";
        
        for (const auto& sym : symbols) {
            std::string category = (sym.category == "Variable") ? "Змінна" : "Масив";
            out << "| " << std::left << std::setw(11) << sym.name
                << "| " << std::setw(10) << sym.type
                << "| " << std::setw(9) << category
                << "| " << std::setw(7) << sym.size
                << "| " << std::setw(9) << sym.address << "|\n";
        }
        out << "-----------------------------------------------------\n";
    }
    
    // Отримати розмір типу в байтах
    static int getTypeSize(const std::string& type) {
        if (type == "INT16T") return 2;  // 16 біт = 2 байти
        if (type == "FLOAT32") return 4; // 32 біти = 4 байти
        if (type == "BOOL") return 1;    // 1 байт для булевого типу
        return 0;
    }
    
    const std::vector<SymbolEntry>& getSymbols() const { return symbols; }

private:
    std::vector<SymbolEntry> symbols;
    std::map<std::string, int> nameToIndex;  // name -> index in symbols vector
    
    int calculateNextAddress(const std::string& /*type*/, int /*size*/) {
        if (symbols.empty()) return 0;
        const SymbolEntry& last = symbols.back();
        int typeSize = getTypeSize(last.type);
        return last.address + typeSize * last.size;
    }
};


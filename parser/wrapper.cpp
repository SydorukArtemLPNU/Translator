#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <limits>

namespace fs = std::filesystem;

void printMenu() {
    std::cout << "\n=======================================\n";
    std::cout << "  Обгортка для скриптів транслятора\n";
    std::cout << "=======================================\n";
    std::cout << "  1) translate.sh - Трансляція вхідного файлу\n";
    std::cout << "  2) compile.sh   - Компіляція вихідного файлу\n";
    std::cout << "  3) build.sh     - Трансляція та компіляція\n";
    std::cout << "  0) Вихід\n";
    std::cout << "=======================================\n";
    std::cout << "Виберіть опцію (0-3): ";
}

std::string getInputFile(int choice) {
    std::string extension;
    std::string action;
    
    switch (choice) {
        case 1:
            extension = ".s20";
            action = "трансляції";
            break;
        case 3:
            extension = ".s20";
            action = "трансляції та компіляції";
            break;
        case 2:
            extension = ".c";
            action = "компіляції";
            break;
        default:
            return "";
    }
    
    std::cout << "\nВведіть шлях до файлу для " << action << " (розширення " << extension << "): ";
    std::string filename;
    std::cin >> filename;
    
    // Видаляємо пробіли з початку та кінця
    while (!filename.empty() && (filename.front() == ' ' || filename.front() == '\t')) {
        filename.erase(0, 1);
    }
    while (!filename.empty() && (filename.back() == ' ' || filename.back() == '\t')) {
        filename.pop_back();
    }
    
    // Перевірка розширення
    if (!filename.empty() && filename.find(extension) == std::string::npos) {
        std::cout << "Попередження: файл не має розширення " << extension << "\n";
    }
    
    return filename;
}

bool fileExists(const std::string& filename) {
    return fs::exists(filename) && fs::is_regular_file(filename);
}

int executeScript(int choice, const std::string& filename) {
    std::string scriptPath;
    std::string scriptName;
    
    // Використовуємо поточну директорію для пошуку скриптів
    std::string scriptDir = fs::current_path().string();
    
    // Визначаємо який скрипт виконувати
    switch (choice) {
        case 1:
            scriptName = "translate.sh";
            scriptPath = scriptDir + "/translate.sh";
            break;
        case 2:
            scriptName = "compile.sh";
            scriptPath = scriptDir + "/compile.sh";
            break;
        case 3:
            scriptName = "build.sh";
            scriptPath = scriptDir + "/build.sh";
            break;
        default:
            return 1;
    }
    
    if (!fileExists(scriptPath)) {
        std::cerr << "Помилка: Скрипт '" << scriptName << "' не знайдено в поточній директорії.\n";
        return 1;
    }
    
    // Створюємо команду для виконання
    std::string command = scriptPath + " \"" + filename + "\"";
    
    std::cout << "\nВиконання: " << scriptName << " " << filename << "\n";
    std::cout << "---------------------------------------\n";
    
    // Виконуємо скрипт
    int result = std::system(command.c_str());
    
    std::cout << "---------------------------------------\n";
    
    if (result != 0) {
        std::cerr << "\nПомилка: Скрипт завершився з кодом " << result << "\n";
    }
    
    return result;
}

int main() {
    int choice = -1;
    
    while (true) {
        printMenu();
        
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Невірний ввід. Будь ласка, введіть число від 0 до 3.\n";
            continue;
        }
        
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        if (choice == 0) {
            std::cout << "Вихід з програми.\n";
            return 0;
        }
        
        if (choice < 1 || choice > 3) {
            std::cout << "Невірний вибір. Будь ласка, введіть число від 0 до 3.\n";
            continue;
        }
        
        std::string filename = getInputFile(choice);
        
        if (filename.empty()) {
            std::cout << "Помилка: Не введено ім'я файлу.\n";
            continue;
        }
        
        // Перевірка існування файлу
        if (!fileExists(filename)) {
            std::cerr << "Помилка: Файл '" << filename << "' не існує.\n";
            continue;
        }
        
        // Виконання скрипту
        int result = executeScript(choice, filename);
        
        if (result == 0) {
            std::cout << "\n✓ Операція успішно завершена!\n";
        }
    }
    
    return 0;
}


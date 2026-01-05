#!/bin/bash

# Скрипт для трансляції вхідного .s20 файлу

if [ $# -eq 0 ]; then
    echo "Використання: $0 <input_file.s20>"
    echo "Приклад: $0 prog_test1.s20"
    exit 1
fi

INPUT_FILE="$1"

# Перевірка наявності файлу
if [ ! -f "$INPUT_FILE" ]; then
    echo "Помилка: Файл '$INPUT_FILE' не знайдено"
    exit 1
fi

# Перевірка розширення
if [[ ! "$INPUT_FILE" == *.s20 ]]; then
    echo "Помилка: Файл повинен мати розширення .s20"
    exit 1
fi

# Перевірка наявності транслятора
if [ ! -f "./translator" ]; then
    echo "Помилка: Транслятор 'translator' не знайдено. Спочатку виконайте 'make'"
    exit 1
fi

# Запускаємо транслятор з ім'ям файлу як аргументом
./translator "$INPUT_FILE"
EXIT_CODE=$?
exit $EXIT_CODE

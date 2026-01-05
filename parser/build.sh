#!/bin/bash

# Скрипт для виконання трансляції та компіляції

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

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Крок 1: Трансляція
echo "=== Крок 1: Трансляція ==="
cd "$SCRIPT_DIR"
"$SCRIPT_DIR/translate.sh" "$INPUT_FILE"
TRANSLATE_EXIT=$?

if [ $TRANSLATE_EXIT -ne 0 ]; then
    echo "Помилка трансляції"
    exit 1
fi

# Отримуємо ім'я вихідного .c файлу та папку
BASENAME=$(basename "$INPUT_FILE" .s20)
OUTPUT_DIR="$BASENAME"
C_FILE="$OUTPUT_DIR/$BASENAME.c"

# Перевірка створення .c файлу
if [ ! -f "$C_FILE" ]; then
    echo "Помилка: Файл '$C_FILE' не було створено після трансляції"
    exit 1
fi

# Крок 2: Компіляція
echo ""
echo "=== Крок 2: Компіляція ==="
SKIP_RUN=1 "$SCRIPT_DIR/compile.sh" "$C_FILE"
COMPILE_EXIT=$?

if [ $COMPILE_EXIT -ne 0 ]; then
    echo "Помилка компіляції"
    exit 1
fi

echo ""
echo "=== Готово ==="
echo "Всі файли знаходяться в папці: $OUTPUT_DIR/"
echo "  - Лексеми: $OUTPUT_DIR/${BASENAME}_lexemes.txt"
echo "  - Помилки: $OUTPUT_DIR/${BASENAME}_errors.txt"
echo "  - C код: $OUTPUT_DIR/$BASENAME.c"
echo "  - Об'єктний файл: $OUTPUT_DIR/$BASENAME.o"
echo "  - Виконуваний файл: $OUTPUT_DIR/$BASENAME"
echo ""

# Перевірка наявності виконуваного файлу та запуск
EXECUTABLE="$OUTPUT_DIR/$BASENAME"
if [ -f "$EXECUTABLE" ] && [ -x "$EXECUTABLE" ]; then
    echo "=== Запуск програми ==="
    "$EXECUTABLE"
else
    echo "Помилка: Виконуваний файл не знайдено або не має прав на виконання"
    exit 1
fi

exit 0


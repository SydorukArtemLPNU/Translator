#!/bin/bash

# Скрипт для компіляції вихідного .c файлу

if [ $# -eq 0 ]; then
    echo "Використання: $0 <output_file.c>"
    echo "Приклад: $0 prog_test1.c"
    exit 1
fi

C_FILE="$1"

# Перевірка наявності файлу
if [ ! -f "$C_FILE" ]; then
    echo "Помилка: Файл '$C_FILE' не знайдено"
    exit 1
fi

# Перевірка розширення
if [[ ! "$C_FILE" == *.c ]]; then
    echo "Помилка: Файл повинен мати розширення .c"
    exit 1
fi

# Отримуємо директорію та базове ім'я файлу
C_DIR=$(dirname "$C_FILE")
BASENAME=$(basename "$C_FILE" .c)
OBJECT_FILE="$C_DIR/$BASENAME.o"
OUTPUT_FILE="$C_DIR/$BASENAME"

# Перевірка наявності компілятора
if command -v clang &> /dev/null; then
    COMPILER="clang"
elif command -v gcc &> /dev/null; then
    COMPILER="gcc"
else
    echo "Помилка: Не знайдено компілятор C (clang або gcc)"
    exit 1
fi

echo "Компіляція $C_FILE за допомогою $COMPILER..."

# Компіляція в об'єктний файл
echo "Крок 1: Створення об'єктного файлу..."
"$COMPILER" -c "$C_FILE" -o "$OBJECT_FILE" -std=c11

if [ $? -ne 0 ]; then
    echo "Помилка компіляції в об'єктний файл"
    exit 1
fi

echo "Об'єктний файл створено: $OBJECT_FILE"

# Лінкування в виконуваний файл
echo "Крок 2: Створення виконуваного файлу..."
"$COMPILER" -o "$OUTPUT_FILE" "$OBJECT_FILE" -std=c11

if [ $? -eq 0 ]; then
    echo "Успішно скомпільовано: $OUTPUT_FILE"
    echo "Об'єктний файл: $OBJECT_FILE"
    # Запускаємо програму тільки якщо скрипт викликається напряму (не з build.sh)
    # Перевіряємо, чи викликається з build.sh через перевірку змінної середовища
    if [ -z "$SKIP_RUN" ]; then
        echo ""
        echo "=== Запуск програми ==="
        "$OUTPUT_FILE"
    fi
    exit 0
else
    echo "Помилка лінкування"
    exit 1
fi


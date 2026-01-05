# s20 - Адаптований під нову EBNF граматику

Цей парсер адаптований під нову розширену форму Бекуса-Наура (EBNF) з обмеженням на довжину ідентифікатора (максимум 8 символів).

## Основні зміни:

1. **Ключові слова**: TASK_NAME, START, STOP, VARIABLE, INT16T, FLOAT32, IF, THEN, ELSE, REPEAT, UNTIL, READ, WRITE, DIV, MOD

2. **Ідентифікатори**: 
   - Починаються з малої літери
   - Решта символів - великі літери або цифри
   - Максимум 8 символів загалом
   - Приклад: `x`, `y`, `mAXVAL`, `tESTLIN`

3. **Структура програми**: 
   ```
   TASK_NAME identifier;
   START 
     VARIABLE declaration_block
     statement_list
   STOP
   ```

4. **Присвоєння**: `expression -> target_access` (зворотний порядок)

5. **READ/WRITE**: з дужками та списками аргументів
   - `READ(target_access, ...)`
   - `WRITE(expression | string_literal, ...)`

6. **IF-THEN-ELSE**: без Goto, з опціональним Else
   ```
   IF boolean_expr THEN statement [ELSE statement]
   ```

7. **REPEAT-UNTIL**: замість For
   ```
   REPEAT statement_list UNTIL boolean_expr;
   ```

8. **Оператори**: 
   - Порівняння: `==`, `!=`, `>`, `<`, `>=`, `<=`
   - Логічні: `&&`, `||`, `!!` (унарне НЕ)
   - Арифметичні: `+`, `-`, `*`, `DIV`, `MOD`
   - Присвоєння: `->`

9. **Коментарі**: формат `// ... //` (закриваються подвійним слешем)

## Файли:

- `LexicalAnalyzer.h/cpp` - лексичний аналізатор
- `Parser.h/cpp` - синтаксичний аналізатор
- `AST.h` - дерево абстрактного синтаксису
- `SymbolTable.h` - таблиця символів з hash-map для швидкого доступу
- `CodeGenerator.h/cpp` - транслятор AST в мову C
- `main.cpp` - головна програма

## Компіляція та запуск:

### Варіант 1: Використання Makefile

```bash
# Компіляція
make

# Запуск
./translator

# Або одразу компіляція + запуск
make run

# Очистка
make clean
```

### Варіант 2: Ручна компіляція

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o translator \
    main.cpp LexicalAnalyzer.cpp Parser.cpp CodeGenerator.cpp

# Запуск
./translator
```

## Використання транслятора:

1. **Запустіть програму:**
   ```bash
   ./translator
   ```

2. **Виберіть файл для трансляції:**
   - Програма знайде всі файли `prog_*.d06` в поточній директорії
   - Введіть номер файлу (1, 2, 3, ...) або 0 для виходу

3. **Результати роботи:**
   - Виведення таблиці лексичного аналізу (токени)
   - Дерево розбору (parse tree)
   - Таблиця ідентифікаторів
   - Таблиця символів (Symbol Table) з адресами
   - **Згенерований C код** в файлі `*.c` (наприклад, `prog_linear.c`)

4. **Компіляція згенерованого C коду:**
   ```bash
   gcc -o prog_linear prog_linear.c -std=c11
   ./prog_linear
   ```

## Тестові програми:

- `prog_linear.d06` - тестування лінійності (послідовність операторів)
- `prog_branch.d06` - тестування розгалуження (IF-THEN-ELSE)
- `prog_cycle.d06` - тестування циклів (REPEAT-UNTIL)

## Приклад використання:

```bash
$ make
$ ./translator

Found 3 file(s):
  1) prog_branch.d06
  2) prog_cycle.d06
  3) prog_linear.d06

Choose file number (1-3, 0 to exit): 1

==============================
FILE: prog_branch.d06
==============================

=== Lexical Analysis ===
[... таблиця токенів ...]

=== Syntax Analysis ===
[SUCCESS] Syntactically correct.

=== Parse Tree ===
[... дерево розбору ...]

=== Refined Identifier Table ===
[... таблиця ідентифікаторів ...]

=== Symbol Table ===
[... таблиця символів з адресами ...]

=== Code Generation ===
Generated C code: prog_branch.c

$ gcc -o prog_branch prog_branch.c -std=c11
$ ./prog_branch
[... виконання програми ...]
```

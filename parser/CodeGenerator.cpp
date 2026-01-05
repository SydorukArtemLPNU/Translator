#include "CodeGenerator.h"
#include "AST.h"
#include "SymbolTable.h"
#include <cctype>
#include <algorithm>

std::string CodeGenerator::newLabel(const std::string& prefix) {
    return prefix + std::to_string(labelCounter++);
}

std::string CodeGenerator::getCType(const std::string& type) {
    if (type == "INT16T") return "int16_t";
    if (type == "FLOAT32") return "float";
    if (type == "BOOL") return "bool";
    return "int";  // default
}

std::string CodeGenerator::getCVarName(const std::string& name) {
    // Конвертуємо імена з формату Low-Up8 в C (можна залишити як є або конвертувати)
    // Наприклад: tESTLIN -> testlin, mAXVAL -> maxval
    std::string result = name;
    if (!result.empty()) {
        result[0] = std::tolower(static_cast<unsigned char>(result[0]));
        for (size_t i = 1; i < result.length(); ++i) {
            result[i] = std::tolower(static_cast<unsigned char>(result[i]));
        }
    }
    return result;
}

std::string CodeGenerator::getExprType(ExpressionNode* expr) {
    if (!expr) return "int";
    
    // Булеві константи
    if (dynamic_cast<BoolNode*>(expr)) {
        return "bool";
    }
    
    // Числові константи
    if (auto* num = dynamic_cast<NumberNode*>(expr)) {
        // Перевіряємо чи це float (містить крапку)
        if (num->val.find('.') != std::string::npos) {
            return "float";
        }
        return "int";
    }
    
    // Змінні - перевіряємо тип через SymbolTable
    if (auto* var = dynamic_cast<VarRefNode*>(expr)) {
        if (symbolTable) {
            SymbolEntry* sym = symbolTable->lookup(var->name);
            if (sym) {
                if (sym->type == "BOOL") return "bool";
                if (sym->type == "FLOAT32") return "float";
                if (sym->type == "INT16T") return "int";
            }
        }
        return "int"; // default
    }
    
    // Бінарні операції - визначаємо тип на основі операндів
    if (auto* binOp = dynamic_cast<BinaryOpNode*>(expr)) {
        // Логічні операції завжди повертають bool
        if (binOp->op == "&&" || binOp->op == "||" || 
            binOp->op == "==" || binOp->op == "!=" || 
            binOp->op == ">" || binOp->op == "<" || 
            binOp->op == ">=" || binOp->op == "<=") {
            return "bool";
        }
        
        // Для арифметичних операцій перевіряємо операнди
        std::string leftType = getExprType(binOp->left);
        std::string rightType = getExprType(binOp->right);
        
        // Якщо хоча б один операнд float, результат float
        if (leftType == "float" || rightType == "float") {
            return "float";
        }
        // Якщо обидва int, результат int
        if (leftType == "int" && rightType == "int") {
            return "int";
        }
        // Для булевих операцій
        if (leftType == "bool" || rightType == "bool") {
            return "int"; // Булеві значення конвертуються в int для арифметики
        }
        
        return "int"; // default
    }
    
    // Унарні операції
    if (auto* unOp = dynamic_cast<UnaryOpNode*>(expr)) {
        if (unOp->op == "!!") {
            return "bool"; // Логічне заперечення
        }
        // Для унарного мінуса тип залишається тим самим
        return getExprType(unOp->operand);
    }
    
    return "int"; // default
}

void CodeGenerator::generate(ProgramNode* program, const std::string& outputFile) {
    out.open(outputFile);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output file " << outputFile << std::endl;
        return;
    }
    
    generateProgram(program);
    out.close();
}

void CodeGenerator::generateProgram(ProgramNode* node) {
    // Заголовок програми
    out << "#include <stdio.h>\n";
    out << "#include <stdint.h>\n";
    out << "#include <stdbool.h>\n\n";
    
    // Генерація оголошень змінних
    out << "int main(void) {\n";
    
    // Оголошення змінних
    if (symbolTable) {
        for (const auto& sym : symbolTable->getSymbols()) {
            std::string cType = getCType(sym.type);
            std::string cName = getCVarName(sym.name);
            
            if (sym.category == "Array") {
                out << "    " << cType << " " << cName << "[" << sym.size << "];\n";
            } else {
                out << "    " << cType << " " << cName << ";\n";
            }
        }
        out << "\n";
    }
    
    // Генерація тіла програми (без додаткових дужок, бо вони вже в main)
    if (node->body) {
        for (auto* stmt : node->body->stmts) {
            if (stmt) {
                generateStatement(stmt);
            }
        }
    }
    
    out << "    return 0;\n";
    out << "}\n";
}

void CodeGenerator::generateBlock(BlockNode* block) {
    if (!block) return;
    
    // Блок - це послідовність операторів
    // В C блоку потрібні дужки { }
    // Але для основного блоку програми дужки не потрібні (вони вже є в main)
    // Це тільки для вкладених блоків
    out << "    {\n";
    
    for (auto* stmt : block->stmts) {
        if (stmt) {
            generateStatement(stmt);
        }
    }
    
    out << "    }\n";
}

void CodeGenerator::generateStatement(StatementNode* stmt) {
    if (!stmt) return;
    
    if (auto* assign = dynamic_cast<AssignNode*>(stmt)) {
        generateAssign(assign);
    }
    else if (auto* io = dynamic_cast<IO_Node*>(stmt)) {
        generateIO(io);
    }
    else if (auto* ifStmt = dynamic_cast<IfNode*>(stmt)) {
        generateIf(ifStmt);
    }
    else if (auto* repeat = dynamic_cast<RepeatNode*>(stmt)) {
        generateRepeat(repeat);
    }
    else if (auto* block = dynamic_cast<BlockNode*>(stmt)) {
        generateBlock(block);
    }
}

void CodeGenerator::generateAssign(AssignNode* node) {
    // Присвоєння: expression -> target_access
    // В AssignNode: val (expression) присвоюється name[idx] (target)
    std::string cName = getCVarName(node->name);
    std::string target;
    
    if (node->idx) {
        // Масив: name[index]
        std::string index = generateExpr(node->idx);
        target = cName + "[" + index + "]";
    } else {
        // Проста змінна
        target = cName;
    }
    
    std::string expr = generateExpr(node->val);
    out << "    " << target << " = " << expr << ";\n";
}

std::string CodeGenerator::generateTarget(VarRefNode* varRef) {
    std::string cName = getCVarName(varRef->name);
    
    if (varRef->idx) {
        // Масив: arr[index]
        std::string index = generateExpr(varRef->idx);
        return cName + "[" + index + "]";
    } else {
        // Проста змінна
        return cName;
    }
}

std::string CodeGenerator::generateExpr(ExpressionNode* expr) {
    if (!expr) return "";
    
    if (auto* num = dynamic_cast<NumberNode*>(expr)) {
        return num->val;
    }
    else if (auto* boolNode = dynamic_cast<BoolNode*>(expr)) {
        return boolNode->val == "1" ? "true" : "false";
    }
    else if (auto* str = dynamic_cast<StringNode*>(expr)) {
        return "\"" + str->text + "\"";
    }
    else if (auto* var = dynamic_cast<VarRefNode*>(expr)) {
        return generateTarget(var);
    }
    else if (auto* binOp = dynamic_cast<BinaryOpNode*>(expr)) {
        std::string left = generateExpr(binOp->left);
        std::string right = generateExpr(binOp->right);
        std::string op = binOp->op;
        
        // Конвертуємо оператори
        if (op == "DIV") op = "/";
        else if (op == "MOD") op = "%";
        else if (op == "&&") op = "&&";
        else if (op == "||") op = "||";
        else if (op == "==") op = "==";
        else if (op == "!=") op = "!=";
        
        return "(" + left + " " + op + " " + right + ")";
    }
    else if (auto* unOp = dynamic_cast<UnaryOpNode*>(expr)) {
        std::string operand = generateExpr(unOp->operand);
        std::string op = unOp->op;
        
        if (op == "!!") op = "!";
        else if (op == "-") op = "-";
        
        return "(" + op + operand + ")";
    }
    
    return "";
}

void CodeGenerator::generateIO(IO_Node* node) {
    if (node->type == "Read") {
        // READ(target_access, ...)
        for (auto* arg : node->args) {
            if (auto* varRef = dynamic_cast<VarRefNode*>(arg)) {
                std::string target = generateTarget(varRef);
                SymbolEntry* sym = symbolTable->lookup(varRef->name);
                if (sym) {
                    if (sym->type == "INT16T") {
                        out << "    scanf(\"%hd\", &" << target << ");\n";
                    } else if (sym->type == "FLOAT32") {
                        out << "    scanf(\"%f\", &" << target << ");\n";
                    } else if (sym->type == "BOOL") {
                        out << "    {\n";
                        out << "        int temp;\n";
                        out << "        scanf(\"%d\", &temp);\n";
                        out << "        " << target << " = (temp != 0);\n";
                        out << "    }\n";
                    }
                }
            }
        }
    }
    else if (node->type == "Write") {
        // WRITE(expression | string_literal, ...)
        bool first = true;
        for (auto* arg : node->args) {
            if (!first) {
                // Для множинних аргументів можна додати printf без переносу рядка
            }
            first = false;
            
            if (auto* str = dynamic_cast<StringNode*>(arg)) {
                out << "    printf(\"" << str->text << "\\n\");\n";
            }
            else {
                std::string expr = generateExpr(arg);
                std::string exprType = getExprType(arg);
                
                // Визначаємо тип для формату виводу
                if (exprType == "bool") {
                    out << "    printf(\"%d\\n\", " << expr << " ? 1 : 0);\n";
                } else if (exprType == "float") {
                    out << "    printf(\"%f\\n\", " << expr << ");\n";
                } else {
                    // Для int та інших типів
                    out << "    printf(\"%d\\n\", (int)(" << expr << "));\n";
                }
            }
        }
    }
}

void CodeGenerator::generateIf(IfNode* node) {
    // IF condition THEN statement [ELSE statement]
    std::string cond = generateExpr(node->cond);
    
    if (node->elseStmt) {
        // IF-THEN-ELSE
        std::string labelElse = newLabel("label_else_");
        std::string labelEnd = newLabel("label_end_");
        
        out << "    if (!(" << cond << ")) goto " << labelElse << ";\n";
        
        if (node->thenStmt) {
            generateStatement(node->thenStmt);
        }
        
        out << "    goto " << labelEnd << ";\n";
        out << labelElse << ":\n";
        generateStatement(node->elseStmt);
        out << labelEnd << ":\n";
    } else {
        // IF-THEN (без ELSE)
        std::string labelEnd = newLabel("label_end_");
        
        out << "    if (!(" << cond << ")) goto " << labelEnd << ";\n";
        
        if (node->thenStmt) {
            generateStatement(node->thenStmt);
        }
        
        out << labelEnd << ":\n";
    }
}

void CodeGenerator::generateRepeat(RepeatNode* node) {
    std::string labelStart = newLabel("label_repeat_");
    std::string labelEnd = newLabel("label_repeat_end_");
    
    out << labelStart << ":\n";
    
    if (node->body) {
        if (auto* block = dynamic_cast<BlockNode*>(node->body)) {
            for (auto* stmt : block->stmts) {
                if (stmt) generateStatement(stmt);
            }
        } else {
            // Якщо body не BlockNode, генеруємо як statement
            generateStatement(node->body);
        }
    }
    
    // REPEAT ... UNTIL condition - перевіряємо умову в кінці
    std::string cond = generateExpr(node->condition);
    out << "    if (" << cond << ") goto " << labelEnd << ";\n";
    out << "    goto " << labelStart << ";\n";
    out << labelEnd << ":\n";
}


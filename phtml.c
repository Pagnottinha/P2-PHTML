#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"

// Definição das estruturas para os tipos da linguagem
typedef enum
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        int intValue;
        float floatValue;
        char charValue;
        int boolValue;
        char *stringValue;
    } value;
} Value;

// Estrutura para parâmetros de função
typedef struct
{
    char *name;
    ValueType type;
} Parameter;

// Estrutura para armazenar funções
typedef struct Function
{
    char *name;
    ValueType returnType;
    int paramCount;
    Parameter *parameters;
    mpc_ast_t *body;
} Function;

// Estrutura para armazenar variáveis
typedef struct Variable
{
    char *name;
    Value value;
    struct Variable *next;
} Variable;

// Ambiente de execução
typedef struct Environment
{
    Variable *variables;
    Function *functions;
    int functionCount;
    struct Environment *parent;
} Environment;

// Funções utilitárias
Value fixValueType(Value value);

ValueType getType(const char *typeStr)
{
    if (strcmp(typeStr, "int") == 0)
        return TYPE_INT;
    if (strcmp(typeStr, "float") == 0)
        return TYPE_FLOAT;
    if (strcmp(typeStr, "char") == 0)
        return TYPE_CHAR;
    if (strcmp(typeStr, "bool") == 0)
        return TYPE_BOOL;
    if (strcmp(typeStr, "string") == 0)
        return TYPE_STRING;
    if (strcmp(typeStr, "void") == 0)
        return TYPE_VOID;

    printf("Tipo desconhecido: %s\n", typeStr);
    exit(1);
}

char *getTypeString(ValueType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_CHAR:
        return "char";
    case TYPE_BOOL:
        return "bool";
    case TYPE_STRING:
        return "string";
    case TYPE_VOID:
        return "void";
    default:
        return "unknown";
    }
}

// Cria um novo ambiente
Environment *createEnvironment(Environment *parent)
{
    Environment *env = malloc(sizeof(Environment));
    env->variables = NULL;
    env->functions = NULL;
    env->functionCount = 0;
    env->parent = parent;
    return env;
}

// Procura uma variável no ambiente
Variable *findVariable(Environment *env, const char *name)
{

    Variable *current = env->variables;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current;
        }
        current = current->next;
    }
    // Procura no ambiente pai se não encontrar no ambiente atual
    if (env->parent)
    {
        return findVariable(env->parent, name);
    }

    return NULL;
}

// Adiciona ou atualiza uma variável no ambiente
void setVariable(Environment *env, const char *name, Value value)
{
    // Corrige o tipo antes de atribuir
    value = fixValueType(value);

    Variable *var = findVariable(env, name);
    if (var)
    {
        // Atualiza variável existente
        if (var->value.type == TYPE_STRING && value.type != TYPE_STRING)
        {
            free(var->value.value.stringValue);
        }

        if (value.type == TYPE_STRING)
        {
            if (var->value.type == TYPE_STRING)
            {
                // Se ambos são strings, temos que liberar o antigo antes de atribuir o novo
                free(var->value.value.stringValue);
            }
            // Atribui o novo valor string (sempre fazendo uma cópia)
            var->value.type = TYPE_STRING;
            var->value.value.stringValue = strdup(value.value.stringValue);
        }
        else
        {
            // Para outros tipos, copiamos o valor diretamente
            var->value = value;
        }
    }
    else
    {
        // Cria nova variável
        var = malloc(sizeof(Variable));
        var->name = strdup(name);

        if (value.type == TYPE_STRING)
        {
            var->value.type = TYPE_STRING;
            var->value.value.stringValue = strdup(value.value.stringValue);
        }
        else
        {
            var->value = value;
        }

        var->next = env->variables;
        env->variables = var;
    }
}

// Procura uma função no ambiente
Function *findFunction(Environment *env, const char *name)
{
    for (int i = 0; i < env->functionCount; i++)
    {
        if (strcmp(env->functions[i].name, name) == 0)
        {
            return &env->functions[i];
        }
    }
    return NULL;
}

// Adiciona uma função ao ambiente
void addFunction(Environment *env, Function func)
{
    env->functionCount++;
    env->functions = realloc(env->functions, sizeof(Function) * env->functionCount);
    env->functions[env->functionCount - 1] = func;
}

// Converte uma string para um valor
Value parseValue(const char *str, ValueType type)
{
    Value val;
    val.type = type;

    switch (type)
    {
    case TYPE_INT:
        val.value.intValue = atoi(str);
        break;
    case TYPE_FLOAT:
        val.value.floatValue = atof(str);
        break;
    case TYPE_CHAR:
        val.value.charValue = str[0];
        break;
    case TYPE_BOOL:
        // Garante que booleanos são armazenados corretamente
        if (strcmp(str, "true") == 0)
        {
            val.value.boolValue = 1;
        }
        else if (strcmp(str, "false") == 0)
        {
            val.value.boolValue = 0;
        }
        else
        {
            printf("Erro: valor booleano inválido: %s\n", str);
            exit(1);
        }
        break;
    case TYPE_STRING:
        val.value.stringValue = strdup(str);
        break;
    case TYPE_VOID:
        break;
    }

    return val;
}

// Converte um valor para string
char *valueToString(Value value)
{
    char buffer[256];

    switch (value.type)
    {
    case TYPE_INT:
        sprintf(buffer, "%d", value.value.intValue);
        break;
    case TYPE_FLOAT:
        sprintf(buffer, "%f", value.value.floatValue);
        break;
    case TYPE_CHAR:
        sprintf(buffer, "%c", value.value.charValue);
        break;
    case TYPE_BOOL:
        if (value.value.boolValue)
        {
            strcpy(buffer, "true");
        }
        else
        {
            strcpy(buffer, "false");
        }
        break;
    case TYPE_STRING:
        if (value.value.stringValue)
        {
            // Garante que fazemos uma cópia profunda da string
            char *result = malloc(strlen(value.value.stringValue) + 1);
            strcpy(result, value.value.stringValue);
            return result;
        }
        else
        {
            return strdup("");
        }
    case TYPE_VOID:
        return strdup("");
    }

    // Para outros tipos, fazemos uma cópia do buffer
    char *result = malloc(strlen(buffer) + 1);
    strcpy(result, buffer);
    return result;
}

// Forward declaration para funções de avaliação
Value evaluateExpression(mpc_ast_t *ast, Environment *env);
void evaluateCommandList(mpc_ast_t *ast, Environment *env);
void evaluateCommand(mpc_ast_t *ast, Environment *env);

// Avalia uma chamada de função
Value evaluateCall(mpc_ast_t *ast, Environment *env)
{
    char *functionName = NULL;

    // Encontra o nome da função
    for (int i = 0; i < ast->children_num; i++)
    {
        if (strstr(ast->children[i]->tag, "identifier"))
        {
            functionName = ast->children[i]->contents;
            break;
        }
    }

    if (!functionName)
    {
        printf("Erro: nome da função não encontrado\n");
        exit(1);
    }

    Function *function = findFunction(env, functionName);
    if (!function)
    {
        printf("Erro: função '%s' não encontrada\n", functionName);
        exit(1);
    }

    // Avalia os argumentos
    Value *args = NULL;
    int argCount = 0;

    //  Procura por tags args_block
    for (int i = 0; i < ast->children_num; i++)
    {
        if (strstr(ast->children[i]->tag, "args_block"))
        {
            mpc_ast_t *argsBlockNode = ast->children[i];

            // Procura por args dentro do args_block
            for (int j = 0; j < argsBlockNode->children_num; j++)
            {
                if (strstr(argsBlockNode->children[j]->tag, "arg_list"))
                {
                    mpc_ast_t *argListNode = argsBlockNode->children[j];

                    // Na nova estrutura, arg_list pode ser um único arg
                    if (strstr(argListNode->tag, "|arg"))
                    {
                        argCount = 1; // temos um argumento direto

                        args = malloc(sizeof(Value) * argCount);

                        // Procura expressão dentro do argumento
                        for (int k = 0; k < argListNode->children_num; k++)
                        {
                            if (strstr(argListNode->children[k]->tag, "expression"))
                            {
                                args[0] = evaluateExpression(argListNode->children[k], env);
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Estrutura mais complexa onde arg_list contém vários args
                        // Conta os argumentos

                        for (int k = 0; k < argListNode->children_num; k++)
                        {
                            if (strstr(argListNode->children[k]->tag, "arg"))
                            {
                                argCount++;
                            }
                        }

                        args = malloc(sizeof(Value) * argCount);
                        int argIndex = 0;

                        // Avalia cada expressão dentro de <arg>
                        for (int k = 0; k < argListNode->children_num; k++)
                        {
                            if (strstr(argListNode->children[k]->tag, "arg"))
                            {
                                mpc_ast_t *argNode = argListNode->children[k];
                                // Procura expressão dentro do argumento
                                for (int l = 0; l < argNode->children_num; l++)
                                {
                                    if (strstr(argNode->children[l]->tag, "expression"))
                                    {
                                        args[argIndex++] = evaluateExpression(argNode->children[l], env);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            break;
        }
    }

    // Verifica se a quantidade de argumentos está correta
    if (argCount != function->paramCount)
    {
        printf("Erro: função '%s' espera %d argumentos, mas recebeu %d\n",
               functionName, function->paramCount, argCount);
        free(args);
        exit(1);
    }

    // Cria um novo ambiente para a execução da função
    Environment *funcEnv = createEnvironment(env);

    // Define os parâmetros como variáveis no ambiente da função
    for (int i = 0; i < function->paramCount; i++)
    {
        setVariable(funcEnv, function->parameters[i].name, args[i]);
    }

    free(args);

    // Executa o corpo da função
    evaluateCommandList(function->body, funcEnv);

    // Obtém o valor de retorno (se houver)
    Value returnValue;
    returnValue.type = function->returnType;

    // Inicializa com valor padrão
    switch (returnValue.type)
    {
    case TYPE_INT:
        returnValue.value.intValue = 0;
        break;
    case TYPE_FLOAT:
        returnValue.value.floatValue = 0.0;
        break;
    case TYPE_CHAR:
        returnValue.value.charValue = '\0';
        break;
    case TYPE_BOOL:
        returnValue.value.boolValue = 0;
        break;
    case TYPE_STRING:
        returnValue.value.stringValue = strdup("");
        break;
    case TYPE_VOID:
        // Nada a fazer para void
        break;
    }
    if (function->returnType != TYPE_VOID)
    {
        Variable *returnVar = findVariable(funcEnv, "return");
        if (returnVar)
        {

            // Se tivermos um valor de retorno, usamos ele
            if (returnVar->value.type == TYPE_STRING && returnValue.type == TYPE_STRING)
            {
                free(returnValue.value.stringValue); // Libera a string padrão que foi alocada acima
                returnValue.value.stringValue = strdup(returnVar->value.value.stringValue);
            }
            else
            {
                returnValue = returnVar->value;
            }
        }
        else
        {
            // TODO: TRATAR
        }
    }
    // TODO: Limpar o ambiente da função

    return returnValue;
}

// Avalia uma expressão
Value evaluateExpression(mpc_ast_t *ast, Environment *env)
{
    // Verifica se é uma chamada de função
    if (strstr(ast->tag, "function_call"))
    {
        return evaluateCall(ast, env);
    } // Verifica se é um identificador (variável)
    if (strstr(ast->tag, "identifier"))
    {
        Variable *var = findVariable(env, ast->contents);
        if (!var)
        {
            printf("Erro: variável '%s' não encontrada\n", ast->contents);
            exit(1);
        }
        // Garantindo que booleanos são retornados com tipo correto
        if (var->value.type == TYPE_STRING && var->value.value.stringValue != NULL)
        {
            if (strcmp(var->value.value.stringValue, "true") == 0 ||
                strcmp(var->value.value.stringValue, "false") == 0)
            {
                Value correctedValue = fixValueType(var->value);

                // Atualizar a variável para uso futuro
                var->value = correctedValue;
                return var->value;
            }
        }

        return var->value;
    }

    // Verificar se é um número
    if (strstr(ast->tag, "number"))
    {
        if (strchr(ast->contents, '.'))
        {
            Value val;
            val.type = TYPE_FLOAT;
            val.value.floatValue = atof(ast->contents);
            return val;
        }
        else
        {
            Value val;
            val.type = TYPE_INT;
            val.value.intValue = atoi(ast->contents);
            return val;
        }
    }
    // Verificar se é uma string
    if (strstr(ast->tag, "string"))
    {
        // Se for string mas contém "true" ou "false" sem aspas, é um booleano
        if ((strcmp(ast->contents, "true") == 0 || strcmp(ast->contents, "false") == 0) &&
            !strchr(ast->contents, '"'))
        {
            Value val;
            val.type = TYPE_BOOL;
            val.value.boolValue = (strcmp(ast->contents, "true") == 0);
            return val;
        }

        Value val;
        val.type = TYPE_STRING;

        // Cria uma cópia do conteúdo para não modificar o original
        char *content = strdup(ast->contents);

        // Remove as aspas da cópia
        char *str = content + 1;
        str[strlen(str) - 1] = '\0';
        // Garante que fazemos uma cópia profunda da string
        val.value.stringValue = malloc(strlen(str) + 1);
        strcpy(val.value.stringValue, str);

        // Libera a cópia temporária
        free(content);
        return val;
    }

    // Verificar se é um caractere
    if (strstr(ast->tag, "character"))
    {
        Value val;
        val.type = TYPE_CHAR;
        val.value.charValue = ast->contents[1]; // Ignora a aspas inicial
        return val;
    } // Verificar se é um boolean
    if (strstr(ast->tag, "boolean"))
    {
        Value val;
        val.type = TYPE_BOOL;
        val.value.boolValue = (strcmp(ast->contents, "true") == 0) ? 1 : 0;

        return val;
    }

    // Expressão entre parênteses
    if (strstr(ast->tag, "primary") && strstr(ast->children[0]->tag, "string") && ast->children[0]->contents[0] == '(')
    {
        for (int i = 0; i < ast->children_num; i++)
        {
            if (strstr(ast->children[i]->tag, "expression"))
            {
                return evaluateExpression(ast->children[i], env);
            }
        }
    }

    // Operadores lógicos e aritméticos
    if (ast->children_num >= 3)
    {
        // Primeiro, tente encontrar operadores
        for (int i = 1; i < ast->children_num - 1; i++)
        {
            char *op = ast->children[i]->contents;

            if (strstr(op, "||") || strstr(op, "&&") ||
                strstr(op, "==") || strstr(op, "!=") ||
                strstr(op, "<") || strstr(op, ">") ||
                strstr(op, "<=") || strstr(op, ">=") ||
                strstr(op, "+") || strstr(op, "-") ||
                strstr(op, "*") || strstr(op, "/"))
            {

                Value left = evaluateExpression(ast->children[i - 1], env);
                Value right = evaluateExpression(ast->children[i + 1], env);
                Value result;

                if (strcmp(op, "||") == 0)
                {
                    result.type = TYPE_BOOL;
                    result.value.boolValue = (left.value.boolValue || right.value.boolValue);
                    return result;
                }
                else if (strcmp(op, "&&") == 0)
                {
                    result.type = TYPE_BOOL;

                    // Verificar se ambos os operandos são booleanos, verificando os valores diretamente
                    if (left.type != TYPE_BOOL || right.type != TYPE_BOOL)
                    {
                        printf("Erro: operador && requer operandos do tipo boolean (tipos: %s e %s)\n",
                               getTypeString(left.type), getTypeString(right.type));
                        // Debug para ajudar a identificar o problema
                        exit(1);
                    }

                    // Executar a operação lógica AND
                    result.value.boolValue = (left.value.boolValue && right.value.boolValue);
                    return result;
                }
                else if (strcmp(op, "==") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue == right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue == right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue == right.value.charValue);
                    }
                    else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL)
                    {
                        result.value.boolValue = (left.value.boolValue == right.value.boolValue);
                    }
                    else if (left.type == TYPE_STRING && right.type == TYPE_STRING)
                    {
                        result.value.boolValue = (strcmp(left.value.stringValue, right.value.stringValue) == 0);
                    }
                    return result;
                }
                else if (strcmp(op, "!=") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue != right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue != right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue != right.value.charValue);
                    }
                    else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL)
                    {
                        result.value.boolValue = (left.value.boolValue != right.value.boolValue);
                    }
                    else if (left.type == TYPE_STRING && right.type == TYPE_STRING)
                    {
                        result.value.boolValue = (strcmp(left.value.stringValue, right.value.stringValue) != 0);
                    }
                    return result;
                }
                else if (strcmp(op, "<") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue < right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue < right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue < right.value.charValue);
                    }
                    return result;
                }
                else if (strcmp(op, ">") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue > right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue > right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue > right.value.charValue);
                    }
                    return result;
                }
                else if (strcmp(op, "<=") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue <= right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue <= right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue <= right.value.charValue);
                    }
                    return result;
                }
                else if (strcmp(op, ">=") == 0)
                {
                    result.type = TYPE_BOOL;
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.value.boolValue = (left.value.intValue >= right.value.intValue);
                    }
                    else if (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT)
                    {
                        result.value.boolValue = (left.value.floatValue >= right.value.floatValue);
                    }
                    else if (left.type == TYPE_CHAR && right.type == TYPE_CHAR)
                    {
                        result.value.boolValue = (left.value.charValue >= right.value.charValue);
                    }
                    return result;
                }
                else if (strcmp(op, "+") == 0)
                {
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.type = TYPE_INT;
                        result.value.intValue = left.value.intValue + right.value.intValue;
                    }
                    else if ((left.type == TYPE_FLOAT && right.type == TYPE_INT) ||
                             (left.type == TYPE_INT && right.type == TYPE_FLOAT) ||
                             (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT))
                    {
                        result.type = TYPE_FLOAT;
                        float leftVal = (left.type == TYPE_INT) ? left.value.intValue : left.value.floatValue;
                        float rightVal = (right.type == TYPE_INT) ? right.value.intValue : right.value.floatValue;
                        result.value.floatValue = leftVal + rightVal;
                    }
                    else if (left.type == TYPE_STRING || right.type == TYPE_STRING)
                    {
                        result.type = TYPE_STRING;
                        char *leftStr = NULL;
                        char *rightStr = NULL; // Cria cópias profundas das strings ou converte outros tipos para string
                        if (left.type == TYPE_BOOL)
                        {
                            // Tratamento especial para booleanos para evitar problemas de concatenação
                            leftStr = strdup(left.value.boolValue ? "true" : "false");
                        }
                        else if (left.type == TYPE_STRING)
                        {
                            leftStr = strdup(left.value.stringValue);
                        }
                        else
                        {
                            leftStr = valueToString(left);
                        }

                        if (right.type == TYPE_BOOL)
                        {
                            // Tratamento especial para booleanos para evitar problemas de concatenação
                            rightStr = strdup(right.value.boolValue ? "true" : "false");
                        }
                        else if (right.type == TYPE_STRING)
                        {
                            rightStr = strdup(right.value.stringValue);
                        }
                        else
                        {
                            rightStr = valueToString(right);
                        }

                        // Aloca memória para a nova string concatenada
                        size_t leftLen = strlen(leftStr);
                        size_t rightLen = strlen(rightStr);
                        result.value.stringValue = malloc(leftLen + rightLen + 1);

                        // Copia o conteúdo das strings usando strcpy/strcat para maior segurança
                        strcpy(result.value.stringValue, leftStr);
                        strcat(result.value.stringValue, rightStr);

                        // Libera as strings temporárias
                        free(leftStr);
                        free(rightStr);
                    }
                    return result;
                }
                else if (strcmp(op, "-") == 0)
                {
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.type = TYPE_INT;
                        result.value.intValue = left.value.intValue - right.value.intValue;
                    }
                    else if ((left.type == TYPE_FLOAT && right.type == TYPE_INT) ||
                             (left.type == TYPE_INT && right.type == TYPE_FLOAT) ||
                             (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT))
                    {
                        result.type = TYPE_FLOAT;
                        float leftVal = (left.type == TYPE_INT) ? left.value.intValue : left.value.floatValue;
                        float rightVal = (right.type == TYPE_INT) ? right.value.intValue : right.value.floatValue;
                        result.value.floatValue = leftVal - rightVal;
                    }
                    return result;
                }
                else if (strcmp(op, "*") == 0)
                {
                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.type = TYPE_INT;
                        result.value.intValue = left.value.intValue * right.value.intValue;
                    }
                    else if ((left.type == TYPE_FLOAT && right.type == TYPE_INT) ||
                             (left.type == TYPE_INT && right.type == TYPE_FLOAT) ||
                             (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT))
                    {
                        result.type = TYPE_FLOAT;
                        float leftVal = (left.type == TYPE_INT) ? left.value.intValue : left.value.floatValue;
                        float rightVal = (right.type == TYPE_INT) ? right.value.intValue : right.value.floatValue;
                        result.value.floatValue = leftVal * rightVal;
                    }
                    return result;
                }
                else if (strcmp(op, "/") == 0)
                {
                    if (right.type == TYPE_INT && right.value.intValue == 0)
                    {
                        printf("Erro: divisão por zero\n");
                        exit(1);
                    }
                    else if (right.type == TYPE_FLOAT && right.value.floatValue == 0.0)
                    {
                        printf("Erro: divisão por zero\n");
                        exit(1);
                    }

                    if (left.type == TYPE_INT && right.type == TYPE_INT)
                    {
                        result.type = TYPE_INT;
                        result.value.intValue = left.value.intValue / right.value.intValue;
                    }
                    else if ((left.type == TYPE_FLOAT && right.type == TYPE_INT) ||
                             (left.type == TYPE_INT && right.type == TYPE_FLOAT) ||
                             (left.type == TYPE_FLOAT && right.type == TYPE_FLOAT))
                    {
                        result.type = TYPE_FLOAT;
                        float leftVal = (left.type == TYPE_INT) ? left.value.intValue : left.value.floatValue;
                        float rightVal = (right.type == TYPE_INT) ? right.value.intValue : right.value.floatValue;
                        result.value.floatValue = leftVal / rightVal;
                    }
                    return result;
                }
            }
        }
    }

    // Operador unário
    if (ast->children_num >= 2)
    {
        char *op = ast->children[0]->contents;

        if (strcmp(op, "-") == 0 || strcmp(op, "!") == 0)
        {
            Value val = evaluateExpression(ast->children[1], env);
            Value result;

            if (strcmp(op, "-") == 0)
            {
                if (val.type == TYPE_INT)
                {
                    result.type = TYPE_INT;
                    result.value.intValue = -val.value.intValue;
                }
                else if (val.type == TYPE_FLOAT)
                {
                    result.type = TYPE_FLOAT;
                    result.value.floatValue = -val.value.floatValue;
                }
            }
            else if (strcmp(op, "!") == 0)
            {
                result.type = TYPE_BOOL;
                result.value.boolValue = !val.value.boolValue;
            }

            return result;
        }
    }

    printf("Erro: expressão %s %s não reconhecida\n", ast->tag);

    exit(1);
}

// Avalia um comando de retorno
Value evaluateReturn(mpc_ast_t *ast, Environment *env)
{
    Value returnValue;
    returnValue.type = TYPE_VOID; // Default

    // Procura a expressão no nó filho
    for (int j = 0; j < ast->children_num; j++)
    {
        if (strstr(ast->children[j]->tag, "expression"))
        {
            returnValue = evaluateExpression(ast->children[j], env);
            return returnValue;
        }
        else if (strstr(ast->children[j]->tag, "sum"))
        {
            // Alguns nós podem ser identificados como sum diretamente
            returnValue = evaluateExpression(ast->children[j], env);
            return returnValue;
        }
        else if (strcmp(ast->children[j]->contents, "+") == 0)
        {
            // Encontramos um operador de soma, vamos avaliar a expressão como um todo
            if (j > 0 && j + 1 < ast->children_num)
            {
                //  Avalia o lado esquerdo
                Value leftVal = evaluateExpression(ast->children[j - 1], env);

                // Avalia o lado direito
                Value rightVal = evaluateExpression(ast->children[j + 1], env);

                // Realiza a operação
                if (leftVal.type == TYPE_STRING || rightVal.type == TYPE_STRING)
                {
                    char *leftStr = NULL;
                    char *rightStr = NULL;

                    // Cria cópias profundas das strings ou converte outros tipos para string
                    if (leftVal.type == TYPE_STRING)
                    {
                        leftStr = strdup(leftVal.value.stringValue);
                    }
                    else
                    {
                        leftStr = valueToString(leftVal);
                    }

                    if (rightVal.type == TYPE_STRING)
                    {
                        rightStr = strdup(rightVal.value.stringValue);
                    }
                    else
                    {
                        rightStr = valueToString(rightVal);
                    }

                    // Aloca memória para a nova string concatenada
                    returnValue.type = TYPE_STRING;
                    size_t leftLen = strlen(leftStr);
                    size_t rightLen = strlen(rightStr);
                    returnValue.value.stringValue = malloc(leftLen + rightLen + 1);

                    // Copia o conteúdo das strings usando memcpy que é mais eficiente
                    memcpy(returnValue.value.stringValue, leftStr, leftLen);
                    memcpy(returnValue.value.stringValue + leftLen, rightStr, rightLen);
                    returnValue.value.stringValue[leftLen + rightLen] = '\0';
                    // Libera as strings temporárias
                    free(leftStr);
                    free(rightStr);

                    return returnValue;
                }
            }
        }
    }

    return returnValue;
}

// Avalia uma lista de comandos
void evaluateCommandList(mpc_ast_t *ast, Environment *env)
{
    if (strstr(ast->tag, "command_list") && strstr(ast->tag, "command"))
    {
        evaluateCommand(ast, env);
    }

    for (int i = 0; i < ast->children_num; i++)
    {
        mpc_ast_t *child = ast->children[i];
        evaluateCommand(child, env);
    }
}

void evaluateCommand(mpc_ast_t *ast, Environment *env)
{
    // Declaração de variável
    if (strstr(ast->tag, "variable_declaration"))
    {
        char *varType = NULL;
        char *varName = NULL;

        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "type"))
            {
                varType = ast->children[j]->contents;
            }
            else if (strstr(ast->children[j]->tag, "identifier"))
            {
                varName = ast->children[j]->contents;
            }
        }

        if (varType && varName)
        {
            Value val;
            val.type = getType(varType);

            // Inicializa com valor padrão
            switch (val.type)
            {
            case TYPE_INT:
                val.value.intValue = 0;
                break;
            case TYPE_FLOAT:
                val.value.floatValue = 0.0;
                break;
            case TYPE_CHAR:
                val.value.charValue = '\0';
                break;
            case TYPE_BOOL:
                val.value.boolValue = 0;
                break;
            case TYPE_STRING:
                val.value.stringValue = strdup("");
                break;
            case TYPE_VOID:
                // Nada a fazer para void
                break;
            }

            setVariable(env, varName, val);
        }
    }

    // Atribuição
    else if (strstr(ast->tag, "assignment"))
    {
        char *varName = NULL;
        mpc_ast_t *exprNode = NULL;

        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "identifier"))
            {
                varName = ast->children[j]->contents;
            }
            else if (strstr(ast->children[j]->tag, "expression"))
            {
                exprNode = ast->children[j];
            }
        }

        if (varName && exprNode)
        {
            Value val = evaluateExpression(exprNode, env);
            setVariable(env, varName, val);
        }
    }

    // If-estrutura
    else if (strstr(ast->tag, "if_structure"))
    {
        mpc_ast_t *condNode = NULL;
        mpc_ast_t *thenNode = NULL;
        mpc_ast_t *elseNode = NULL;

        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "expression"))
            {
                condNode = ast->children[j];
            }
            else if (strstr(ast->children[j]->tag, "command_list") && !thenNode)
            {
                thenNode = ast->children[j];
            }
            else if (strstr(ast->children[j]->tag, "else"))
            {
                for (int k = 0; k < ast->children[j]->children_num; k++)
                {
                    if (strstr(ast->children[j]->children[k]->tag, "command_list"))
                    {
                        elseNode = ast->children[j]->children[k];
                        break;
                    }
                }
            }
        }

        if (condNode && thenNode)
        {
            Value condVal = evaluateExpression(condNode, env);

            if (condVal.type != TYPE_BOOL)
            {
                printf("Erro: condição deve ser do tipo bool\n");
                exit(1);
            }

            if (condVal.value.boolValue)
            {
                evaluateCommandList(thenNode, env);
            }
            else if (elseNode)
            {
                evaluateCommandList(elseNode, env);
            }
        }
    }

    // While-estrutura
    else if (strstr(ast->tag, "while_structure"))
    {
        mpc_ast_t *condNode = NULL;
        mpc_ast_t *bodyNode = NULL;

        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "expression"))
            {
                condNode = ast->children[j];
            }
            else if (strstr(ast->children[j]->tag, "command_list"))
            {
                bodyNode = ast->children[j];
            }
        }

        if (condNode && bodyNode)
        {
            while (1)
            {
                Value condVal = evaluateExpression(condNode, env);

                if (condVal.type != TYPE_BOOL)
                {
                    printf("Erro: condição deve ser do tipo bool\n");
                    exit(1);
                }

                if (!condVal.value.boolValue)
                {
                    break;
                }

                evaluateCommandList(bodyNode, env);
            }
        }
    }

    // Chamada de função
    else if (strstr(ast->tag, "function_call"))
    {
        evaluateCall(ast, env);
    }
    // Return
    else if (strstr(ast->tag, "return"))
    {
        mpc_ast_t *exprNode = NULL;
        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "expression"))
            {
                exprNode = ast->children[j];
            }
        }
        if (exprNode)
        {
            Value val = evaluateExpression(exprNode, env);
            setVariable(env, "return", val);
            return;
        }
    }

    // Print
    else if (strstr(ast->tag, "print"))
    {
        mpc_ast_t *exprNode = NULL;
        for (int j = 0; j < ast->children_num; j++)
        {
            if (strstr(ast->children[j]->tag, "expression"))
            {
                exprNode = ast->children[j];
            }
        }

        if (exprNode)
        {
            Value val = evaluateExpression(exprNode, env);

            switch (val.type)
            {
            case TYPE_INT:
                printf("%d\n", val.value.intValue);
                break;
            case TYPE_FLOAT:
                printf("%f\n", val.value.floatValue);
                break;
            case TYPE_CHAR:
                printf("%c\n", val.value.charValue);
                break;
            case TYPE_BOOL:
                printf("%s\n", val.value.boolValue ? "true" : "false");
                break;
            case TYPE_STRING:
                printf("%s\n", val.value.stringValue);
                break;
            case TYPE_VOID:
                printf("void\n");
                break;
            }
        }
    }
}

void loadFunction(mpc_ast_t *child, Environment *env)
{
    if (strstr(child->tag, "function_declaration"))
    {
        char *funcName = NULL;
        char *returnType = NULL;
        mpc_ast_t *paramListNode = NULL;
        mpc_ast_t *commandListNode = NULL;

        // Extrai informações da função
        for (int j = 0; j < child->children_num; j++)
        {
            mpc_ast_t *node = child->children[j];

            if (strstr(node->tag, "identifier") && !funcName)
            {
                funcName = node->contents;
            }
            else if (strstr(node->tag, "primitive_type") && !returnType)
            {
                returnType = node->contents;
            }
            else if (strstr(node->tag, "parameter_list"))
            {
                paramListNode = node;
            }
            else if (strstr(node->tag, "command_list"))
            {
                commandListNode = node;
            }
        }

        if (funcName && returnType && commandListNode)
        {
            Function func;
            func.name = strdup(funcName);
            func.returnType = getType(returnType);
            func.body = commandListNode;
            func.paramCount = 0;
            func.parameters = NULL;

            // Processa parâmetros, se houver
            if (paramListNode)
            {
                // Conta parâmetros
                for (int j = 0; j < paramListNode->children_num; j++)
                {
                    if (strstr(paramListNode->children[j]->tag, "parameter"))
                    {
                        func.paramCount++;
                    }
                }

                if (func.paramCount > 0)
                {
                    func.parameters = malloc(sizeof(Parameter) * func.paramCount);
                    int paramIndex = 0;

                    for (int j = 0; j < paramListNode->children_num; j++)
                    {
                        mpc_ast_t *paramNode = paramListNode->children[j];

                        if (strstr(paramNode->tag, "parameter"))
                        {
                            char *paramType = NULL;
                            char *paramName = NULL;

                            for (int k = 0; k < paramNode->children_num; k++)
                            {
                                if (strstr(paramNode->children[k]->tag, "type"))
                                {
                                    paramType = paramNode->children[k]->contents;
                                }
                                else if (strstr(paramNode->children[k]->tag, "identifier"))
                                {
                                    paramName = paramNode->children[k]->contents;
                                }
                            }

                            if (paramType && paramName)
                            {
                                func.parameters[paramIndex].name = strdup(paramName);
                                func.parameters[paramIndex].type = getType(paramType);
                                paramIndex++;
                            }
                        }
                    }
                }
            }

            addFunction(env, func);
        }
    }
}

// Carrega todas as funções do código fonte
void loadFunctions(mpc_ast_t *ast, Environment *env)
{
    // Se o nó raiz não contém function_list, procura nós aninhados
    if (!strstr(ast->tag, "function_list"))
    {
        for (int i = 0; i < ast->children_num; i++)
        {
            if (strstr(ast->children[i]->tag, "function_list"))
            {
                return loadFunctions(ast->children[i], env);
            }
        }
    }

    if (strstr(ast->tag, "function_declaration"))
    {
        loadFunction(ast, env);
        return;
    }

    // Percorre o código buscando declarações de função
    for (int i = 0; i < ast->children_num; i++)
    {
        mpc_ast_t *child = ast->children[i];

        loadFunction(child, env);
    }
}

int main(int argc, char **argv)
{
    // Definição dos parsers usando a gramática BNF
    mpc_parser_t *Code = mpc_new("code");
    mpc_parser_t *FunctionList = mpc_new("function_list");
    mpc_parser_t *FunctionDecl = mpc_new("function_declaration");
    mpc_parser_t *ParamList = mpc_new("parameter_list");
    mpc_parser_t *Parameter = mpc_new("parameter");
    mpc_parser_t *CmdList = mpc_new("command_list");
    mpc_parser_t *Command = mpc_new("command");
    mpc_parser_t *VarDecl = mpc_new("variable_declaration");
    mpc_parser_t *Assignment = mpc_new("assignment");
    mpc_parser_t *IfStruct = mpc_new("if_structure");
    mpc_parser_t *ElseOpt = mpc_new("else_optional");
    mpc_parser_t *WhileStruct = mpc_new("while_structure");
    mpc_parser_t *FunctionCall = mpc_new("function_call");
    mpc_parser_t *ArgsBlock = mpc_new("args_block");
    mpc_parser_t *ArgList = mpc_new("arg_list");
    mpc_parser_t *Arg = mpc_new("arg");
    mpc_parser_t *Return = mpc_new("return");
    mpc_parser_t *Print = mpc_new("print");
    mpc_parser_t *Expression = mpc_new("expression");
    mpc_parser_t *LogicalOr = mpc_new("logical_or");
    mpc_parser_t *LogicalAnd = mpc_new("logical_and");
    mpc_parser_t *Equality = mpc_new("equality");
    mpc_parser_t *Relational = mpc_new("relational");
    mpc_parser_t *Sum = mpc_new("sum");
    mpc_parser_t *Product = mpc_new("product");
    mpc_parser_t *Unary = mpc_new("unary");
    mpc_parser_t *Primary = mpc_new("primary");
    mpc_parser_t *Type = mpc_new("type");
    mpc_parser_t *PrimitiveType = mpc_new("primitive_type");
    mpc_parser_t *String = mpc_new("string");
    mpc_parser_t *Identifier = mpc_new("identifier");
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Character = mpc_new("character");
    mpc_parser_t *Boolean = mpc_new("boolean");

    // Gramática da linguagem
    mpca_lang(MPCA_LANG_DEFAULT,
              "code          : /^/ <function_list> /$/ ;\n"
              "function_list : <function_declaration>+ ;\n"
              "function_declaration : \"<function name='\" <identifier> \"' return='\" <type> \"'>\" <parameter_list>? <command_list>? \"</function>\" ;\n"
              "parameter_list : \"<params>\" <parameter>+ \"</params>\" ;\n"
              "parameter     : \"<param type='\" <type> \"'>\" <identifier> \"</param>\" ;\n"
              "command_list  : <command>+ ;\n"
              "command       : <return>\n"
              "              | <print>\n"
              "              | <variable_declaration>\n"
              "              | <assignment>\n"
              "              | <if_structure>\n"
              "              | <while_structure>\n"
              "              | <function_call> ;\n"
              "variable_declaration : \"<var type='\" <type> \"'>\" <identifier> \"</var>\" ;\n"
              "assignment    : \"<assign var='\" <identifier> \"'>\" <expression> \"</assign>\" ;\n"
              "if_structure  : \"<if cond='\" <expression> \"'>\" <command_list>? \"</if>\" <else_optional>? ;\n"
              "else_optional : \"<else>\" <command_list>? \"</else>\" ;\n"
              "while_structure : \"<while cond='\" <expression> \"'>\" <command_list>? \"</while>\" ;\n"
              "function_call : \"<call name='\" <identifier> \"'>\" <args_block>? \"</call>\" ;\n"
              "args_block    : \"<args>\" <arg_list> \"</args>\" ;\n"
              "arg_list      : <arg>+ ;\n"
              "arg           : \"<arg>\" <expression> \"</arg>\" ;\n"
              "return        : \"<return>\" <expression> \"</return>\" ;\n"
              "print         : \"<print>\" <expression> \"</print>\" ;\n"
              "expression    : <logical_or> ;\n"
              "logical_or    : <logical_and> (\"||\" <logical_or>)* ;\n"
              "logical_and   : <equality> (\"&&\" <logical_and>)* ;\n"
              "equality      : <relational> ((\"==\" | \"!=\") <equality>)* ;\n"
              "relational    : <sum> ((\"<=\" | \">=\" | \"<\" | \">\") <relational>)* ;\n"
              "sum           : <product> ((\"+\" | \"-\") <sum>)* ;\n"
              "product       : <unary> ((\"*\" | \"/\") <product>)* ;\n"
              "unary         : (\"-\" | \"!\") <unary> | <primary> ;    \n"
              "primary       : <number> | <character> | <boolean> | <string> | <identifier> | \"(\" <expression> \")\" | <function_call> ;\n"
              "type          : <primitive_type> ;\n"
              "primitive_type : \"int\" | \"float\" | \"char\" | \"bool\" | \"string\" | \"void\" ;\n"
              "string        : /\"([^\"])*\"/ ;\n"
              "identifier    : /[a-zA-Z][a-zA-Z0-9_]*/ ;\n"
              "number        : /[0-9]+(\\.[0-9]+)?/ ;\n"
              "character     : /\'[a-zA-Z]\'/ ;\n"
              "boolean       : \"true\" | \"false\" ;\n",
              Code, FunctionList, FunctionDecl, ParamList, Parameter,
              CmdList, Command, Return, Print, VarDecl, Assignment, IfStruct,
              ElseOpt, WhileStruct, FunctionCall, ArgsBlock, ArgList, Arg,
              Expression, LogicalOr, LogicalAnd, Equality, Relational,
              Sum, Product, Unary, Primary, Type, PrimitiveType,
              String, Identifier, Number, Character, Boolean);

    // Verifica se um arquivo foi fornecido como argumento
    if (argc > 1)
    {
        mpc_result_t r;
        if (mpc_parse_contents(argv[1], Code, &r))
        {
            mpc_ast_t *ast = (mpc_ast_t *)r.output;
            // Inicializa o ambiente de execução
            Environment *env = createEnvironment(NULL);

            // Carrega as funções do arquivo
            loadFunctions(ast, env);

            // Encontra a função main e executa
            Function *mainFunc = findFunction(env, "main");
            if (mainFunc)
            {
                // Executa a função main
                evaluateCommandList(mainFunc->body, env);
            }
            else
            {
                printf("Erro: função 'main' não encontrada\n");
            }

            mpc_ast_delete(ast);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
    }
    else
    {
        printf("Uso: %s <arquivo.phtml>\n", argv[0]);
    }
    // Limpa os parsers (33 parsers)
    mpc_cleanup(34,
                Code, FunctionList, FunctionDecl, ParamList, Parameter,
                CmdList, Command, VarDecl, Assignment, IfStruct, ElseOpt, WhileStruct,
                FunctionCall, ArgsBlock, ArgList, Arg, Return, Print, Expression, LogicalOr,
                LogicalAnd, Equality, Relational, Sum, Product, Unary, Primary,
                Type, PrimitiveType, String, Identifier, Number, Character, Boolean);

    return 0;
}

// Função para corrigir o tipo de valor
// Esta função é usada para garantir que valores booleanos sejam tratados corretamente
Value fixValueType(Value value)
{
    // Se for uma string "true" ou "false", converte para booleano
    if (value.type == TYPE_STRING)
    {
        if (value.value.stringValue != NULL)
        {
            if (strcmp(value.value.stringValue, "true") == 0)
            {
                free(value.value.stringValue);
                value.type = TYPE_BOOL;
                value.value.boolValue = 1;
            }
            else if (strcmp(value.value.stringValue, "false") == 0)
            {
                free(value.value.stringValue);
                value.type = TYPE_BOOL;
                value.value.boolValue = 0;
            }
        }
    }
    return value;
}

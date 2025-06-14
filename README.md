# PHTML Language Interpreter

## Visão Geral
PHTML é uma linguagem de programação simples com sintaxe inspirada em HTML/XML, projetada para fins educacionais e para demonstrar conceitos de compiladores. Este projeto consiste em um interpretador completo para a linguagem PHTML, implementado em C usando a biblioteca MPC (Micro Parser Combinators) para análise sintática.

## Características da Linguagem

### Tipos de Dados
PHTML suporta os seguintes tipos de dados primitivos:
- `int` - Números inteiros
- `float` - Números de ponto flutuante
- `char` - Caracteres únicos
- `bool` - Valores booleanos (true/false)
- `string` - Cadeias de caracteres
- `void` - Tipo para funções sem valor de retorno

### Estrutura de Programa
Um programa PHTML consiste em uma ou mais funções. A função `main` é o ponto de entrada do programa:

```xml
<function name='main' return='void'>
  <!-- Código aqui -->
</function>
```

### Declaração de Variáveis
```xml
<var type='int'>contador</var>
<var type='string'>nome</var>
```

### Atribuição
```xml
<assign var='contador'>10</assign>
<assign var='nome'>"PHTML"</assign>
```

### Expressões
PHTML suporta várias operações em expressões:

#### Operações Aritméticas
- Adição: `+`
- Subtração: `-`
- Multiplicação: `*`
- Divisão: `/`

#### Operações Relacionais
- Igual: `==`
- Diferente: `!=`
- Maior que: `>`
- Menor que: `<`
- Maior ou igual: `>=`
- Menor ou igual: `<=`

#### Operações Lógicas
- AND: `&&`
- OR: `||`
- NOT: `!`

### Estruturas de Controle

#### Condicionais (if-else)
```xml
<if cond='a > b'>
  <!-- Código se condição verdadeira -->
</if>
<else>
  <!-- Código se condição falsa -->
</else>
```

#### Loops (while)
```xml
<while cond='i < 10'>
  <!-- Código do loop -->
</while>
```

### Funções

#### Declaração
```xml
<function name='soma' return='int'>
  <params>
    <param type='int'>a</param>
    <param type='int'>b</param>
  </params>
  <return>a + b</return>
</function>
```

#### Chamada
```xml
<call name='soma'>
  <args>
    <arg>5</arg>
    <arg>3</arg>
  </args>
</call>
```

### Saída
```xml
<print>expressão</print>
```

## Compilação e Execução

### Pré-requisitos
- Compilador C (GCC recomendado)
- A biblioteca MPC é incluída no projeto (mpc.c e mpc.h)

### Compilando
```bash
gcc -o phtml phtml.c mpc.c
```

### Executando
```bash
./phtml arquivo.phtml
```

## Exemplos

### Exemplo Simples
```xml
<function name='main' return='void'>
  <var type='int'>x</var>
  <assign var='x'>10</assign>
  <print>"Valor: " + x</print>
</function>
```

## Arquivos do Projeto

- `phtml.c` - Código-fonte do interpretador
- `mpc.c` e `mpc.h` - Biblioteca de análise sintática
- `gramatica.txt` - Descrição BNF da gramática PHTML
- `exemplos/` - Diretório contendo arquivos de exemplo em PHTML

## Notas de Desenvolvimento

Este interpretador foi desenvolvido para fins educacionais e demonstra conceitos como:
- Análise léxica e sintática
- Avaliação de expressões
- Escopo de variáveis
- Tipos de dados e conversão de tipos
- Chamadas de função
- Não possui suporte a recursão

Embora seja funcional, este interpretador não é otimizado para uso em produção.
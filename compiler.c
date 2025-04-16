/*
  SimpleLang Compiler
  
  This compiler translates SimpleLang code into assembly language for an 8-bit CPU.
  It consists of three main components:
  1. Lexer - breaks source code into tokens
  2. Parser - analyzes token stream 
  3. Code Generator - emits assembly code 
*/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <stdarg.h>
 
 // Constants for compiler limits 
 #define MAX_TOKEN_LEN 100    // Maximum length of a token
 #define MAX_VARS 100         // Maximum number of variables
 #define MAX_CODE_LINES 1000  // Maximum lines of assembly output
 
 // Token types for our language 
 typedef enum {
     TOKEN_INT, TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_ASSIGN,
     TOKEN_PLUS, TOKEN_MINUS, TOKEN_IF, TOKEN_EQUAL,
     TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_SEMICOLON,
     TOKEN_EOF, TOKEN_UNKNOWN
 } TokenType;
 
 // Structure to represent a token 
 typedef struct {
     TokenType type;      
     char text[MAX_TOKEN_LEN];  // Actual text of the token
 } Token;
 
 // Structure to track variables in symbol table 
 typedef struct {
     char name[MAX_TOKEN_LEN];  // Variable name
     int address;               // Memory address for the variable
 } Variable;
 
 // Global variables for compiler state 
 Variable vars[MAX_VARS];       // Symbol table for variables
 int varCount = 0;              // Number of variables declared
 int labelCount = 0;            // Counter for generating unique labels
 int currentAddress = 16;       // Next available memory address (starts at 16)
 
 char assembly[MAX_CODE_LINES][100];  // Buffer for generated assembly code
 int asmLine = 0;                     // Current line in assembly output
 
 Token currentToken;            // Current token being processed
 int hasToken = 0;              // Flag indicating if we have a token pushed back
 


  // Emit assembly code to output buffer
 void emit(const char *fmt, ...) {
     va_list args;
     va_start(args, fmt);
     vsprintf(assembly[asmLine++], fmt, args);  // Format and store assembly line
     va_end(args);
 }
 
 /*
   Print token information 
   Shows token type and its text content
  */
 void printToken(Token token) {
     const char* typeNames[] = {
         "TOKEN_INT", "TOKEN_IDENTIFIER", "TOKEN_NUMBER", "TOKEN_ASSIGN",
         "TOKEN_PLUS", "TOKEN_MINUS", "TOKEN_IF", "TOKEN_EQUAL",
         "TOKEN_LPAREN", "TOKEN_RPAREN", "TOKEN_LBRACE", "TOKEN_RBRACE", "TOKEN_SEMICOLON",
         "TOKEN_EOF", "TOKEN_UNKNOWN"
     };
     printf("Token: %s ('%s')\n", typeNames[token.type], token.text);
 }
 

  // Lexer - Get next token from input file
 Token getNextToken(FILE *file) {
     // If we have a token pushed back, return that first
     if (hasToken) {
         hasToken = 0;
         return currentToken;
     }
 
     Token token;
     int c;
     
     // Skip whitespace characters
     while ((c = fgetc(file)) != EOF && isspace(c));
     
     // Handle end of file
     if (c == EOF) {
         token.type = TOKEN_EOF;
         strcpy(token.text, "");
         return token;
     }
 
     // Handle identifiers and keywords
     if (isalpha(c)) {
         int len = 0;
         token.text[len++] = c;
         // Read until non-alphanumeric character
         while ((c = fgetc(file)) != EOF && isalnum(c)) {
             if (len < MAX_TOKEN_LEN - 1) token.text[len++] = c;
         }
         if (c != EOF) ungetc(c, file);  // Put back the extra character
         token.text[len] = '\0';
         
         // Check if identifier is a keyword
         if (strcmp(token.text, "int") == 0) token.type = TOKEN_INT;
         else if (strcmp(token.text, "if") == 0) token.type = TOKEN_IF;
         else token.type = TOKEN_IDENTIFIER;
         return token;
     }
 
     // Handle numbers
     if (isdigit(c)) {
         int len = 0;
         token.text[len++] = c;
         // Read until non-digit character
         while ((c = fgetc(file)) != EOF && isdigit(c)) {
             if (len < MAX_TOKEN_LEN - 1) token.text[len++] = c;
         }
         if (c != EOF) ungetc(c, file);  // Put back the extra character
         token.text[len] = '\0';
         token.type = TOKEN_NUMBER;
         return token;
     }
 
     // Handle single-character tokens
     token.text[0] = c;
     token.text[1] = '\0';
     
     // Determine token type based on character
     switch (c) {
         case '=':
             if ((c = fgetc(file)) == '=') {
                 token.type = TOKEN_EQUAL;
                 strcpy(token.text, "==");
             } else {
                 if (c != EOF) ungetc(c, file);
                 token.type = TOKEN_ASSIGN;
                 strcpy(token.text, "=");
             }
             break;
         case '+': token.type = TOKEN_PLUS; break;
         case '-': token.type = TOKEN_MINUS; break;
         case '(': token.type = TOKEN_LPAREN; break;
         case ')': token.type = TOKEN_RPAREN; break;
         case '{': token.type = TOKEN_LBRACE; break;
         case '}': token.type = TOKEN_RBRACE; break;
         case ';': token.type = TOKEN_SEMICOLON; break;
         default:
             token.type = TOKEN_UNKNOWN;
             break;
     }
     
     return token;
 }
 
/*
   Push a token back into the token stream
   Allows us to "unread" a token when we need lookahead
*/
 void ungetToken(Token token) {
     currentToken = token;
     hasToken = 1;
 }
 
/*
   Get memory address for a variable
   Adds to symbol table if not already present
*/
 int getVarAddress(const char *name) {
     // Check if variable already exists
     for (int i = 0; i < varCount; i++) {
         if (strcmp(vars[i].name, name) == 0) return vars[i].address;
     }
 
     // Check if we have space for more variables
     if (varCount >= MAX_VARS) {
         fprintf(stderr, "Error: Too many variables\n");
         exit(1);
     }
 
     // Add new variable to symbol table
     strcpy(vars[varCount].name, name);
     vars[varCount].address = currentAddress++;
     return vars[varCount++].address;
 }
 
 /*
   Compile an expression (right-hand side of assignment)
   Handles numbers, variables, and binary operations
  */
 void compileExpression(FILE *file, const char *targetVar) {
     Token token = getNextToken(file);
     printToken(token); 
     
     if (token.type == TOKEN_NUMBER) {
         // Handle literal numbers (e.g., a = 5)
         emit("LDI %s", token.text);  // Load immediate value
         
         // Check for binary operation
         Token op = getNextToken(file);
         printToken(op);
         
         if (op.type == TOKEN_PLUS || op.type == TOKEN_MINUS) {
             Token rhs = getNextToken(file);
             printToken(rhs);
             
             if (rhs.type == TOKEN_NUMBER) {
                 // Number op Number (e.g., 5 + 3)
                 emit("%sI %s", (op.type == TOKEN_PLUS) ? "ADD" : "SUB", rhs.text);
             } else if (rhs.type == TOKEN_IDENTIFIER) {
                 // Number op Variable (e.g., 5 + x)
                 emit("%s %d", (op.type == TOKEN_PLUS) ? "ADD" : "SUB", getVarAddress(rhs.text));
             } else {
                 fprintf(stderr, "Error: Expected number or identifier after operator\n");
                 exit(1);
             }
         } else {
             // No operator - just put it back
             ungetToken(op);
         }
         // Store result in target variable
         emit("STA %d", getVarAddress(targetVar));
     } 
     else if (token.type == TOKEN_IDENTIFIER) {
         // Handle variable expressions
         char var[MAX_TOKEN_LEN];
         strcpy(var, token.text);
         
         Token op = getNextToken(file);
         printToken(op);
         
         if (op.type == TOKEN_PLUS || op.type == TOKEN_MINUS) {
             // Binary operation (e.g., a + b)
             emit("LDA %d", getVarAddress(var));  // Load first operand
             
             Token rhs = getNextToken(file);
             printToken(rhs);
             
             if (rhs.type == TOKEN_NUMBER) {
                 // Variable op Number (e.g., x + 5)
                 emit("%sI %s", (op.type == TOKEN_PLUS) ? "ADD" : "SUB", rhs.text);
             } else if (rhs.type == TOKEN_IDENTIFIER) {
                 // Variable op Variable (e.g., x + y)
                 emit("%s %d", (op.type == TOKEN_PLUS) ? "ADD" : "SUB", getVarAddress(rhs.text));
             } else {
                 fprintf(stderr, "Error: Expected number or identifier after operator\n");
                 exit(1);
             }
             // Store result in target variable
             emit("STA %d", getVarAddress(targetVar));
         } else {
             // Simple assignment (e.g., a = b)
             emit("LDA %d", getVarAddress(var));
             emit("STA %d", getVarAddress(targetVar));
             ungetToken(op);
         }
     } else {
         fprintf(stderr, "Error: Expected identifier or number in expression\n");
         exit(1);
     }
 }
 
 /*
   Compile a single statement
   Handles variable declarations, assignments, and if statements
  */
 void compileStatement(FILE *file) {
     Token token = getNextToken(file);
     printToken(token); // Debug output
     
     if (token.type == TOKEN_EOF) {
         return;  // End of file
     }
     
     if (token.type == TOKEN_INT) {
         // Variable declaration 
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_IDENTIFIER) {
             fprintf(stderr, "Error: Expected identifier after 'int'\n");
             exit(1);
         }
         // Add variable to symbol table
         getVarAddress(token.text);
         
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_SEMICOLON) {
             fprintf(stderr, "Error: Expected ';' after variable declaration\n");
             exit(1);
         }
     } 
     else if (token.type == TOKEN_IDENTIFIER) {
         // Assignment statement (e.g. x = 5)
         char var[MAX_TOKEN_LEN];
         strcpy(var, token.text);
         
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_ASSIGN) {
             fprintf(stderr, "Error: Expected '=' after identifier\n");
             exit(1);
         }
         
         // Compile the right-hand side expression
         compileExpression(file, var);
         
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_SEMICOLON) {
             fprintf(stderr, "Error: Expected ';' after assignment\n");
             exit(1);
         }
     } 
     else if (token.type == TOKEN_IF) {
         // If statement ( e.g. if (x == 5) { ... } )
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_LPAREN) {
             fprintf(stderr, "Error: Expected '(' after 'if'\n");
             exit(1);
         }
         
         // Get left side of comparison
         Token lhs = getNextToken(file);
         printToken(lhs);
         
         if (lhs.type != TOKEN_IDENTIFIER) {
             fprintf(stderr, "Error: Expected identifier in if condition\n");
             exit(1);
         }
         
         // Get comparison operator
         Token op = getNextToken(file);
         printToken(op);
         
         if (op.type != TOKEN_EQUAL) {
             fprintf(stderr, "Error: Expected '==' in if condition\n");
             exit(1);
         }
         
         // Get right side of comparison
         Token rhs = getNextToken(file);
         printToken(rhs);
         
         if (rhs.type != TOKEN_IDENTIFIER && rhs.type != TOKEN_NUMBER) {
             fprintf(stderr, "Error: Expected identifier or number in if condition\n");
             exit(1);
         }
         
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_RPAREN) {
             fprintf(stderr, "Error: Expected ')' after if condition\n");
             exit(1);
         }
         
         token = getNextToken(file);
         printToken(token);
         
         if (token.type != TOKEN_LBRACE) {
             fprintf(stderr, "Error: Expected '{' after if condition\n");
             exit(1);
         }
         
         // Generate unique labels for jumps
         char labelTrue[20], labelEnd[20];
         sprintf(labelTrue, "L%d", labelCount++);
         sprintf(labelEnd, "L%d", labelCount++);
         
         // Generate comparison code
         emit("LDA %d", getVarAddress(lhs.text));
         if (rhs.type == TOKEN_NUMBER) {
             emit("SUBI %s", rhs.text);  // Compare with immediate value
         } else {
             emit("SUB %d", getVarAddress(rhs.text));  // Compare with variable
         }
         // Jump if equal (result is zero)
         emit("JZ %s", labelTrue);
         // Jump to end if not equal
         emit("JMP %s", labelEnd);
         // Label for true case
         emit("%s:", labelTrue);
         
         // Compile statements inside if block
         while ((token = getNextToken(file)).type != TOKEN_RBRACE) {
             if (token.type == TOKEN_EOF) {
                 fprintf(stderr, "Error: Unexpected EOF while parsing if block\n");
                 exit(1);
             }
             ungetToken(token);
             compileStatement(file);
         }
         printToken(token);
         
         // Label for end of if statement
         emit("%s:", labelEnd);
     } 
     else if (token.type == TOKEN_SEMICOLON) {
         // Empty statement (just a semicolon)
     } 
     else if (token.type == TOKEN_RBRACE) {
         // End of block - return to caller
         ungetToken(token);
         return;
     } 
     else {
         fprintf(stderr, "Error: Unexpected token '%s' (type: %d)\n", token.text, token.type);
         exit(1);
     }
 }
 
 /*
   Main compilation function
   Processes all statements in the input file
  */
 void compile(FILE *file) {
     while (1) {
         Token token = getNextToken(file);
         if (token.type == TOKEN_EOF) break;
         ungetToken(token);
         compileStatement(file);
     }
 }
 

 int main() {
     printf("SimpleLang Compiler\n");
     
     FILE *file = fopen("input.sl", "r");
     if (!file) {
         perror("Error opening file");
         return 1;
     }
 
     // Perform compilation
     compile(file);
     fclose(file);
 
     // Write assembly output
     FILE *out = fopen("output.asm", "w");
     if (!out) {
         perror("Error creating output file");
         return 1;
     }
 
     // Output all generated assembly lines
     for (int i = 0; i < asmLine; i++) {
         fprintf(out, "%s\n", assembly[i]);
     }
     fclose(out);
 
     printf("Compilation successful! Assembly written to output.asm\n");
     return 0;
 }

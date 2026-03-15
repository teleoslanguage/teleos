#pragma once
#include "lexer.h"
#include "ast.h"

class Parser {
public:
    Parser(std::vector<Token> tokens);
    ASTNodePtr parse();
private:
    std::vector<Token> tokens;
    size_t pos = 0;
    Token& peek(size_t off = 0);
    Token& advance();
    bool check(TokenType t, size_t off = 0);
    bool match(TokenType t);
    Token expect(TokenType t, const std::string& msg);
    bool atEnd();
    bool isKeywordUsableAsIdent(TokenType t);

    ASTNodePtr parseStatement();
    ASTNodePtr parseVarDecl(bool isConst = false);
    ASTNodePtr parseCallStatement();
    ASTNodePtr parseConsoleOutput();
    ASTNodePtr parseConsoleAsk();
    ASTNodePtr parseConsoleCheck();
    ASTNodePtr parseConsoleClear();
    ASTNodePtr parseConsoleColor();
    ASTNodePtr parseConsoleTitle();
    ASTNodePtr parseConsoleWait();
    ASTNodePtr parseConsoleInput();
    ASTNodePtr parseMathCall();
    ASTNodePtr parseMathFunc(TokenType t);
    ASTNodePtr parseSystemHardware();
    ASTNodePtr parseAppOpen();
    ASTNodePtr parseCallLog();
    ASTNodePtr parseFetch(bool isPost = false);
    ASTNodePtr parseImport();
    ASTNodePtr parseExport();
    ASTNodePtr parseRepeat();
    ASTNodePtr parseIf();
    ASTNodePtr parseUnless();
    ASTNodePtr parseWhy();
    ASTNodePtr parseCompare();
    ASTNodePtr parseFunctionEnd();
    ASTNodePtr parseFuncDef();
    ASTNodePtr parseTryCatch();
    ASTNodePtr parseThrow();
    ASTNodePtr parseSwitch();
    ASTNodePtr parseStringFunc(TokenType t);
    ASTNodePtr parseTableFunc(TokenType t);
    ASTNodePtr parseFileFunc(TokenType t);
    ASTNodePtr parseOsFunc(TokenType t);
    ASTNodePtr parseTypeFunc(TokenType t);
    ASTNodePtr parseRangeCall();
    ASTNodePtr parseGenericFunc(NodeType nt);
    ASTNodePtr parseEnumDef();
    ASTNodePtr parseAssert();
    ASTNodePtr parseInspect();
    ASTNodePtr parseWhile();
    ASTNodePtr parseIncr();
    ASTNodePtr parseDecr();
    ASTNodePtr parseBlock();
    ASTNodePtr parseExpr();
    ASTNodePtr parseOr();
    ASTNodePtr parseAnd();
    ASTNodePtr parseComparison();
    ASTNodePtr parseAddSub();
    ASTNodePtr parseMulDiv();
    ASTNodePtr parseUnary();
    ASTNodePtr parsePrimary();
    ASTNodePtr parseTableLiteral();
    std::vector<ASTNodePtr> parseArgList();
};

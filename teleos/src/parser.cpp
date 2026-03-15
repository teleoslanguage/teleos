#include "parser.h"
#include "errors.h"
#include <sstream>

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

Token& Parser::peek(size_t off) { size_t idx = pos + off; if (idx >= tokens.size()) return tokens.back(); return tokens[idx]; }
Token& Parser::advance() { if (!atEnd()) pos++; return tokens[pos-1]; }
bool Parser::check(TokenType t, size_t off) { return peek(off).type == t; }
bool Parser::match(TokenType t) { if (check(t)) { advance(); return true; } return false; }
bool Parser::atEnd() { return pos >= tokens.size() || tokens[pos].type == TokenType::END_OF_FILE; }

bool Parser::isKeywordUsableAsIdent(TokenType t) {
    return t == TokenType::TO || t == TokenType::AND || t == TokenType::OR || t == TokenType::NUM_KW || t == TokenType::GLOBAL || t == TokenType::LOCAL_KW;
}

Token Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    if (isKeywordUsableAsIdent(t) && check(TokenType::IDENTIFIER)) return advance();
    throw TeleosError(ErrorCode::SYNTAX_ERROR, msg + " (got '" + peek().value + "')", peek().line);
}

ASTNodePtr Parser::parse() {
    auto prog = makeNode(NodeType::Program);
    while (!atEnd()) {
        if (check(TokenType::COMMENT)) { auto n = makeNode(NodeType::Comment, peek().line); n->sval = advance().value; prog->children.push_back(n); continue; }
        if (check(TokenType::PARAGRAPH)) { auto n = makeNode(NodeType::Paragraph, peek().line); n->sval = advance().value; prog->children.push_back(n); continue; }
        auto stmt = parseStatement();
        if (stmt) prog->children.push_back(stmt);
    }
    return prog;
}

ASTNodePtr Parser::parseStatement() {
    int line = peek().line;
    if (check(TokenType::COMMENT)) { auto n = makeNode(NodeType::Comment, line); n->sval = advance().value; return n; }
    if (check(TokenType::PARAGRAPH)) { auto n = makeNode(NodeType::Paragraph, line); n->sval = advance().value; return n; }
    if (check(TokenType::CREATE)) return parseVarDecl(false);
    if (check(TokenType::CONST_DEF)) return parseVarDecl(true);
    if (check(TokenType::CALL) || check(TokenType::DO) || check(TokenType::LOCAL_KW)) return parseCallStatement();
    if (check(TokenType::FUNCTION)) {
        // 'function call; why/unless ...' is a call block; 'function compare(...)' is a compare block
        if (pos+1 < tokens.size() && tokens[pos+1].value == "call") {
            advance(); // eat FUNCTION, leave CALL for parseCallStatement
            return parseCallStatement();
        }
        return parseCompare();
    }
    if (check(TokenType::CONSOLE_OUTPUT)) return parseConsoleOutput();
    if (check(TokenType::CONSOLE_ASK)) return parseConsoleAsk();
    if (check(TokenType::CONSOLE_CHECK)) return parseConsoleCheck();
    if (check(TokenType::CONSOLE_CLEAR)) return parseConsoleClear();
    if (check(TokenType::CONSOLE_COLOR)) return parseConsoleColor();
    if (check(TokenType::CONSOLE_TITLE)) return parseConsoleTitle();
    if (check(TokenType::CONSOLE_WAIT)) return parseConsoleWait();
    if (check(TokenType::CONSOLE_INPUT)) return parseConsoleInput();
    if (check(TokenType::MATH_CALL)) return parseMathCall();
    if (check(TokenType::MATH_FLOOR)||check(TokenType::MATH_CEIL)||check(TokenType::MATH_SQRT)||check(TokenType::MATH_POW)||check(TokenType::MATH_ABS)||check(TokenType::MATH_ROUND)||check(TokenType::MATH_MIN)||check(TokenType::MATH_MAX)||check(TokenType::MATH_RANDOM)||check(TokenType::MATH_SIN)||check(TokenType::MATH_COS)||check(TokenType::MATH_TAN)||check(TokenType::MATH_LOG)||check(TokenType::MATH_MOD)) return parseMathFunc(peek().type);
    if (check(TokenType::SYSTEM_HARDWARE)) return parseSystemHardware();
    if (check(TokenType::APP_OPEN)) return parseAppOpen();
    if (check(TokenType::CALLLOG)) return parseCallLog();
    if (check(TokenType::FETCH)) return parseFetch(false);
    if (check(TokenType::FETCH_POST)) return parseFetch(true);
    if (check(TokenType::IMPORT)) return parseImport();
    if (check(TokenType::EXPORT)) return parseExport();
    if (check(TokenType::REPEAT)) return parseRepeat();
    if (check(TokenType::IF)) return parseIf();
    if (check(TokenType::UNLESS)) return parseUnless();
    if (check(TokenType::WHY)) return parseWhy();
    if (check(TokenType::FUNCTION_END)) return parseFunctionEnd();
    if (check(TokenType::FUNC_DEF)) return parseFuncDef();
    if (check(TokenType::ERROR_TRY)) return parseTryCatch();
    if (check(TokenType::ERROR_THROW)) return parseThrow();
    if (check(TokenType::SWITCH_DO)) return parseSwitch();
    if (check(TokenType::FUNC_RETURN)) { advance(); auto n = makeNode(NodeType::FuncReturn, line); if (!atEnd() && !check(TokenType::END)) n->children.push_back(parseExpr()); return n; }
    if (check(TokenType::ENUM_DEF)) return parseEnumDef();
    if (check(TokenType::RANGE_CALL)) return parseRangeCall();
    if (check(TokenType::ASSERT_KW)) return parseAssert();
    if (check(TokenType::INSPECT_KW)) return parseInspect();
    if (check(TokenType::WHILE_KW)) return parseWhile();
    if (check(TokenType::BREAK_KW)) { advance(); return makeNode(NodeType::BreakStmt, line); }
    if (check(TokenType::CONTINUE_KW)) { advance(); return makeNode(NodeType::ContinueStmt, line); }
    if (check(TokenType::CONSOLE_PRINT) || check(TokenType::CONSOLE_PRINTLN)) return parseConsoleOutput();
    if (check(TokenType::IDENTIFIER)) {
        if (peek(1).type == TokenType::INCR) return parseIncr();
        if (peek(1).type == TokenType::DECR) return parseDecr();
    }
    if (check(TokenType::STRING_LEN)||check(TokenType::STRING_UPPER)||check(TokenType::STRING_LOWER)||check(TokenType::STRING_TRIM)||check(TokenType::STRING_SPLIT)||check(TokenType::STRING_JOIN)||check(TokenType::STRING_CONTAINS)||check(TokenType::STRING_REPLACE)||check(TokenType::STRING_STARTS)||check(TokenType::STRING_ENDS)||check(TokenType::STRING_SUB)||check(TokenType::STRING_FIND)||check(TokenType::STRING_REPEAT)||check(TokenType::STRING_REVERSE)||check(TokenType::STRING_FORMAT)) return parseStringFunc(peek().type);
    if (check(TokenType::TABLE_PUSH)||check(TokenType::TABLE_POP)||check(TokenType::TABLE_LEN)||check(TokenType::TABLE_GET)||check(TokenType::TABLE_SET)||check(TokenType::TABLE_REMOVE)||check(TokenType::TABLE_CONTAINS)||check(TokenType::TABLE_KEYS)||check(TokenType::TABLE_MERGE)||check(TokenType::TABLE_SORT)||check(TokenType::TABLE_SLICE)||check(TokenType::TABLE_MAP)||check(TokenType::TABLE_FILTER)||check(TokenType::TABLE_EACH)) return parseTableFunc(peek().type);
    if (check(TokenType::FILE_READ)||check(TokenType::FILE_WRITE)||check(TokenType::FILE_APPEND)||check(TokenType::FILE_EXISTS)||check(TokenType::FILE_DELETE)||check(TokenType::FILE_LINES)||check(TokenType::FILE_COPY)||check(TokenType::FILE_MOVE)||check(TokenType::FILE_SIZE)||check(TokenType::FILE_MKDIR)) return parseFileFunc(peek().type);
    if (check(TokenType::OS_ENV)||check(TokenType::OS_EXIT)||check(TokenType::OS_TIME)||check(TokenType::OS_DATE)||check(TokenType::OS_SLEEP)||check(TokenType::OS_RUN)||check(TokenType::OS_CWD)||check(TokenType::OS_PLATFORM)||check(TokenType::OS_ARGS)) return parseOsFunc(peek().type);
    if (check(TokenType::TYPE_OF)||check(TokenType::TYPE_CAST)||check(TokenType::TYPE_IS)) return parseTypeFunc(peek().type);
    if (check(TokenType::CRYPTO_HASH)||check(TokenType::CRYPTO_B64ENC)||check(TokenType::CRYPTO_B64DEC)||check(TokenType::CRYPTO_HEX)) return parseGenericFunc(NodeType::CryptoFunc);
    if (check(TokenType::JSON_PARSE)||check(TokenType::JSON_STRINGIFY)) return parseGenericFunc(NodeType::JsonFunc);
    if (check(TokenType::CLIP_READ)||check(TokenType::CLIP_WRITE)) return parseGenericFunc(NodeType::ClipFunc);
    if (check(TokenType::NET_PING)||check(TokenType::NET_PORT)||check(TokenType::NET_HOST)) return parseGenericFunc(NodeType::NetFunc);
    if (check(TokenType::REGEX_MATCH)||check(TokenType::REGEX_TEST)||check(TokenType::REGEX_REPLACE)||check(TokenType::REGEX_FIND)) return parseGenericFunc(NodeType::RegexFunc);
    if (check(TokenType::TIME_NOW)||check(TokenType::TIME_FORMAT)||check(TokenType::TIME_DIFF)||check(TokenType::TIME_STAMP)) return parseGenericFunc(NodeType::TimeFunc);
    if (check(TokenType::PROC_PID)||check(TokenType::PROC_LIST)||check(TokenType::PROC_KILL)) return parseGenericFunc(NodeType::ProcFunc);
    if (check(TokenType::CONSOLE_TABLE)) return parseGenericFunc(NodeType::ConsoleTable);
    if (check(TokenType::CONSOLE_BELL)) { int line=peek().line; advance(); return makeNode(NodeType::ConsoleBell,line); }
    if (check(TokenType::STRING_PAD_LEFT)||check(TokenType::STRING_PAD_RIGHT)||check(TokenType::STRING_COUNT)||check(TokenType::STRING_WRAP)) return parseStringFunc(peek().type);
    if (check(TokenType::MATH_CLAMP)||check(TokenType::MATH_LERP)||check(TokenType::MATH_MAP)||check(TokenType::MATH_PI)||check(TokenType::MATH_E)) return parseMathFunc(peek().type);
    if (check(TokenType::FETCH_GET)||check(TokenType::FETCH_POST_KW)) return parseGenericFunc(NodeType::FetchSimple);
    if (check(TokenType::END)) { advance(); return nullptr; }
    if (check(TokenType::IDENTIFIER)) {
        std::string name = peek().value;
        size_t savedPos = pos; advance();
        if (check(TokenType::ASSIGN)) { advance(); auto n = makeNode(NodeType::VarAssign, line); n->sval = name; n->children.push_back(parseExpr()); return n; }
        if (check(TokenType::LPAREN)) { advance(); auto n = makeNode(NodeType::FuncCallUser, line); n->sval = name; while (!check(TokenType::RPAREN) && !atEnd()) { n->children.push_back(parseExpr()); match(TokenType::COMMA); } expect(TokenType::RPAREN,"Expected ')'"); return n; }
        pos = savedPos;
        auto tok = advance();
        throw TeleosError(ErrorCode::UNKNOWN_STATEMENT, "Unknown statement starting with '" + tok.value + "'", tok.line);
    }
    auto tok = advance();
    throw TeleosError(ErrorCode::UNKNOWN_STATEMENT, "Unknown statement starting with '" + tok.value + "'", tok.line);
}

ASTNodePtr Parser::parseVarDecl(bool isConst) {
    int line = peek().line; advance(); match(TokenType::VAR);
    expect(TokenType::ASSIGN, "Expected '='");
    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").value;
    expect(TokenType::GT, "Expected '>'");
    auto node = makeNode(isConst ? NodeType::ConstDecl : NodeType::VarDecl, line);
    node->sval = name;
    if (check(TokenType::LBRACE)) node->children.push_back(parseTableLiteral());
    else node->children.push_back(parseExpr());
    return node;
}

ASTNodePtr Parser::parseCallStatement() {
    int line = peek().line;
    if (check(TokenType::LOCAL_KW)) advance();
    if (check(TokenType::DO)) {
        advance();
        if (check(TokenType::IF)) return parseIf();
        if (check(TokenType::UNLESS)) return parseUnless();
        if (check(TokenType::CONSOLE_OUTPUT)) return parseConsoleOutput();
        auto n = makeNode(NodeType::CallFunction, line); n->sval = "__do__";
        if (!atEnd() && !check(TokenType::END)) n->children.push_back(parseStatement());
        return n;
    }
    advance();
    if (check(TokenType::FUNCTION)) {
        advance();
        // 'function call; why/unless' — eat the call keyword too
        if (check(TokenType::CALL)) advance();
        expect(TokenType::SEMICOLON,"Expected ';'");
        std::string fname;
        if (check(TokenType::IDENTIFIER)) fname = advance().value;
        else if (check(TokenType::TRUE_KW)) { fname="true"; advance(); }
        else if (check(TokenType::FALSE_KW)) { fname="false"; advance(); }
        else if (check(TokenType::WHY)) { fname="why"; advance(); }
        else if (check(TokenType::UNLESS)) { fname="unless"; advance(); }
        match(TokenType::THEN); match(TokenType::DO);
        auto node = makeNode(NodeType::CallFunction, line); node->sval = fname;
        node->children.push_back(parseBlock()); return node;
    }
    if (check(TokenType::IDENTIFIER) && peek().value == "variable") {
        advance(); expect(TokenType::SEMICOLON,"Expected ';'"); match(TokenType::VAR);
        expect(TokenType::ASSIGN,"Expected '='");
        std::string vname = expect(TokenType::IDENTIFIER,"Expected variable name").value;
        match(TokenType::THEN);
        auto node = makeNode(NodeType::CallVar, line); node->sval = vname;
        node->children.push_back(parseBlock()); return node;
    }
    if (check(TokenType::SEMICOLON)) {
        advance(); // eat ;
        if (check(TokenType::WHY)) {
            auto node = makeNode(NodeType::CallFunction, line); node->sval = "why";
            advance(); match(TokenType::THEN); match(TokenType::DO);
            node->children.push_back(parseBlock()); return node;
        }
        if (check(TokenType::UNLESS)) {
            auto node = makeNode(NodeType::CallFunction, line); node->sval = "unless";
            advance(); match(TokenType::THEN); match(TokenType::DO);
            node->children.push_back(parseBlock()); return node;
        }
        // generic: call ; name then do
        std::string fname;
        if (check(TokenType::IDENTIFIER)) fname = advance().value;
        match(TokenType::THEN); match(TokenType::DO);
        auto node = makeNode(NodeType::CallFunction, line); node->sval = fname;
        node->children.push_back(parseBlock()); return node;
    }
    if (check(TokenType::SYSTEM_HARDWARE)) {
        advance(); auto node = makeNode(NodeType::SystemHardware, line);
        if (check(TokenType::DO)) { advance(); while (!atEnd() && !check(TokenType::END)) node->children.push_back(parseStatement()); match(TokenType::END); }
        return node;
    }
    if (check(TokenType::IDENTIFIER)) {
        std::string name = advance().value;
        auto node = makeNode(NodeType::CallFunction, line); node->sval = name;
        if (match(TokenType::THEN)) node->children.push_back(parseBlock());
        return node;
    }
    auto tok = advance();
    throw TeleosError(ErrorCode::SYNTAX_ERROR, "Unexpected token in call: '" + tok.value + "'", line);
}

ASTNodePtr Parser::parseConsoleOutput() {
    int line = peek().line;
    std::string which = peek().value;
    advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleOutput, line);
    if (which == "console.print") node->sval = "print";
    if (!check(TokenType::RPAREN)) node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'");
    return node;
}

ASTNodePtr Parser::parseConsoleAsk() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleAsk, line);
    node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'");
    if (check(TokenType::LBRACKET)) { advance(); while (!check(TokenType::RBRACKET) && !atEnd()) { auto opt = makeNode(NodeType::Literal, line); if (check(TokenType::STRING)) opt->sval = advance().value; node->children.push_back(opt); match(TokenType::COMMA); } expect(TokenType::RBRACKET,"Expected ']'"); }
    if (check(TokenType::LPAREN)) { advance(); if (check(TokenType::IDENTIFIER)) node->sval = advance().value; expect(TokenType::RPAREN,"Expected ')'"); }
    return node;
}

ASTNodePtr Parser::parseConsoleCheck() {
    int line = peek().line;
    std::string tokenVal = advance().value;
    std::string vname;
    const std::string prefix = "console.check.";
    if (tokenVal.size() > prefix.size() && tokenVal.rfind(prefix,0)==0) vname = tokenVal.substr(prefix.size());
    else { if (check(TokenType::DOT)) advance(); if (check(TokenType::IDENTIFIER)) vname = advance().value; }
    expect(TokenType::LPAREN,"Expected '('");
    if (check(TokenType::IDENTIFIER)) advance();
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::ConsoleCheck, line); node->sval = vname;
    node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseConsoleClear() { int line = peek().line; advance(); if (check(TokenType::LPAREN)) { advance(); match(TokenType::RPAREN); } return makeNode(NodeType::ConsoleClear, line); }

ASTNodePtr Parser::parseConsoleColor() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleColor, line);
    node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'"); return node;
}

ASTNodePtr Parser::parseConsoleTitle() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleTitle, line);
    node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'"); return node;
}

ASTNodePtr Parser::parseConsoleWait() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleWait, line);
    if (!check(TokenType::RPAREN)) node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'"); return node;
}

ASTNodePtr Parser::parseConsoleInput() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::ConsoleInput, line);
    if (!check(TokenType::RPAREN)) node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'");
    if (check(TokenType::LPAREN)) { advance(); if (check(TokenType::IDENTIFIER)) node->sval = advance().value; expect(TokenType::RPAREN,"Expected ')'"); }
    return node;
}

ASTNodePtr Parser::parseMathCall() {
    int line = peek().line; advance();
    expect(TokenType::SEMICOLON,"Expected ';'");
    auto left = parseExpr();
    expect(TokenType::ASSIGN,"Expected '='");
    std::string outvar = expect(TokenType::IDENTIFIER,"Expected output variable").value;
    auto node = makeNode(NodeType::MathCall, line); node->sval = outvar;
    node->children.push_back(left);
    node->children.push_back(parseBlock());
    return node;
}

ASTNodePtr Parser::parseMathFunc(TokenType t) {
    int line = peek().line;
    std::string fname = advance().value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::MathFunc, line); node->sval = fname;
    while (!check(TokenType::RPAREN) && !atEnd()) { node->children.push_back(parseExpr()); match(TokenType::COMMA); }
    expect(TokenType::RPAREN,"Expected ')'");
    if (check(TokenType::ASSIGN)) { advance(); node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value); }
    return node;
}

ASTNodePtr Parser::parseSystemHardware() {
    int line = peek().line; advance();
    expect(TokenType::SEMICOLON,"Expected ';'");
    advance(); expect(TokenType::SEMICOLON,"Expected ';'"); advance();
    expect(TokenType::ASSIGN,"Expected '='");
    expect(TokenType::STRING,"Expected 'hardware'");
    expect(TokenType::COMMA,"Expected ','");
    std::string hwtype = expect(TokenType::STRING,"Expected hardware type").value;
    auto node = makeNode(NodeType::SystemHardware, line); node->sval = hwtype;
    if (check(TokenType::ASSIGN)) { advance(); if (check(TokenType::IDENTIFIER)) node->tags.push_back(advance().value); }
    return node;
}

ASTNodePtr Parser::parseAppOpen() {
    int line = peek().line; advance();
    expect(TokenType::ASSIGN,"Expected '='");
    expect(TokenType::LBRACKET,"Expected '['");
    std::string appname = expect(TokenType::STRING,"Expected app name").value;
    expect(TokenType::RBRACKET,"Expected ']'");
    auto node = makeNode(NodeType::AppOpen, line); node->sval = appname; return node;
}

ASTNodePtr Parser::parseCallLog() {
    int line = peek().line; advance();
    expect(TokenType::LBRACE,"Expected '{'");
    expect(TokenType::LBRACKET,"Expected '['");
    expect(TokenType::LPAREN,"Expected '('");
    std::string msg = expect(TokenType::STRING,"Expected log message").value;
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::RBRACKET,"Expected ']'");
    expect(TokenType::RBRACE,"Expected '}'");
    expect(TokenType::DOT,"Expected '.'");
    if (!match(TokenType::TO) && !match(TokenType::IDENTIFIER))
        throw TeleosError(ErrorCode::SYNTAX_ERROR,"Expected 'to'",peek().line);
    expect(TokenType::EQ,"Expected '=='");
    std::string path = expect(TokenType::STRING,"Expected file path").value;
    auto node = makeNode(NodeType::CallLog, line); node->sval = msg; node->tags.push_back(path); return node;
}

ASTNodePtr Parser::parseFetch(bool isPost) {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    while (!check(TokenType::RPAREN) && !atEnd()) advance();
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::DO,"Expected 'do'");
    auto node = makeNode(isPost ? NodeType::FetchPost : NodeType::FetchAPI, line);
    while (!atEnd() && !check(TokenType::END)) { if (check(TokenType::COMMENT)){advance();continue;} auto stmt=parseStatement(); if(stmt) node->children.push_back(stmt); }
    match(TokenType::END); return node;
}

ASTNodePtr Parser::parseImport() {
    int line = peek().line; advance();
    std::string modname = expect(TokenType::STRING,"Expected module name").value;
    auto node = makeNode(NodeType::ImportModule, line); node->sval = modname; return node;
}

ASTNodePtr Parser::parseExport() {
    int line = peek().line; advance();
    expect(TokenType::MODULE,"Expected 'module'");
    expect(TokenType::LPAREN,"Expected '('");
    advance();
    expect(TokenType::RPAREN,"Expected ')'");
    return makeNode(NodeType::ExportModule, line);
}

ASTNodePtr Parser::parseRepeat() {
    int line = peek().line; advance();
    if (check(TokenType::LPAREN)) { advance(); if (check(TokenType::IDENTIFIER)) advance(); expect(TokenType::RPAREN,"Expected ')'"); return makeNode(NodeType::RepeatFinish, line); }
    std::string varname = expect(TokenType::IDENTIFIER,"Expected variable name").value;
    expect(TokenType::UNTIL,"Expected 'until'");
    expect(TokenType::EQ,"Expected '=='");
    auto limit = makeNode(NodeType::Literal, line);
    if (check(TokenType::NIL)) { advance(); limit->sval = "nil"; }
    else { limit->nval = std::stod(expect(TokenType::NUMBER,"Expected number or nil").value); limit->sval = "__number__"; }
    expect(TokenType::NUM_KW,"Expected 'num'");
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::RepeatLoop, line); node->sval = varname;
    node->children.push_back(limit); node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseIf() {
    int line = peek().line; match(TokenType::IF);
    auto cond = makeNode(NodeType::BinaryExpr, line);
    if (check(TokenType::IDENTIFIER) && peek().value == "variable") {
        advance(); expect(TokenType::SEMICOLON,"Expected ';'"); match(TokenType::VAR);
        expect(TokenType::ASSIGN,"Expected '='");
        std::string vname = expect(TokenType::IDENTIFIER,"Expected variable name").value;
        auto lhs = makeNode(NodeType::Identifier, line); lhs->sval = vname; cond->children.push_back(lhs);
    } else { cond->children.push_back(parseAddSub()); }
    if      (check(TokenType::EQ))  { cond->sval="=="; advance(); }
    else if (check(TokenType::NEQ)) { cond->sval="!="; advance(); }
    else if (check(TokenType::LT))  { cond->sval="<";  advance(); }
    else if (check(TokenType::GT))  { cond->sval=">";  advance(); }
    else if (check(TokenType::LEQ)) { cond->sval="<="; advance(); }
    else if (check(TokenType::GEQ)) { cond->sval=">="; advance(); }
    cond->children.push_back(parseExpr());
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::IfStatement, line);
    node->children.push_back(cond); node->children.push_back(parseBlock());
    return node;
}

ASTNodePtr Parser::parseUnless() {
    int line = peek().line; advance();
    auto cond = makeNode(NodeType::BinaryExpr, line);
    cond->children.push_back(parsePrimary());
    if (check(TokenType::EQ)){cond->sval="==";advance();}else if(check(TokenType::NEQ)){cond->sval="!=";advance();}else if(check(TokenType::LT)){cond->sval="<";advance();}else if(check(TokenType::GT)){cond->sval=">";advance();}
    cond->children.push_back(parseExpr());
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::UnlessStatement, line);
    node->children.push_back(cond); node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseWhy() {
    int line = peek().line; advance();
    auto cond = makeNode(NodeType::BinaryExpr, line);
    cond->children.push_back(parsePrimary());
    if (check(TokenType::EQ)){cond->sval="==";advance();}else if(check(TokenType::NEQ)){cond->sval="!=";advance();}else if(check(TokenType::LT)){cond->sval="<";advance();}else if(check(TokenType::GT)){cond->sval=">";advance();}
    cond->children.push_back(parseExpr());
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::WhyStatement, line);
    node->children.push_back(cond); node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseCompare() {
    int line = peek().line; advance();
    expect(TokenType::IDENTIFIER,"Expected 'compare'");
    expect(TokenType::LPAREN,"Expected '('");
    std::string v1 = expect(TokenType::IDENTIFIER,"Expected variable").value;
    expect(TokenType::RPAREN,"Expected ')'");
    if (!match(TokenType::TO)) throw TeleosError(ErrorCode::SYNTAX_ERROR,"Expected 'to'",peek().line);
    expect(TokenType::LPAREN,"Expected '('");
    std::string v2 = expect(TokenType::IDENTIFIER,"Expected variable").value;
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::AND,"Expected 'and'");
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::CompareBlock, line);
    node->sval = v1; node->tags.push_back(v2);
    node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseFunctionEnd() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    std::string fname;
    while (!check(TokenType::RPAREN) && !atEnd()) { fname += peek().value; advance(); }
    expect(TokenType::RPAREN,"Expected ')'");
    auto node = makeNode(NodeType::FunctionEndBlock, line); node->sval = fname; return node;
}

ASTNodePtr Parser::parseFuncDef() {
    int line = peek().line; advance();
    std::string name = expect(TokenType::IDENTIFIER,"Expected function name").value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::FuncDef, line); node->sval = name;
    while (!check(TokenType::RPAREN) && !atEnd()) { node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected param").value); match(TokenType::COMMA); }
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::THEN,"Expected 'then'");
    node->children.push_back(parseBlock()); return node;
}

ASTNodePtr Parser::parseTryCatch() {
    int line = peek().line; advance();
    expect(TokenType::THEN,"Expected 'then'");
    auto node = makeNode(NodeType::TryCatch, line);
    node->children.push_back(parseBlock());
    if (check(TokenType::ERROR_CATCH)) { advance(); if(check(TokenType::LPAREN)){advance();if(check(TokenType::IDENTIFIER))node->sval=advance().value;expect(TokenType::RPAREN,"Expected ')'");} expect(TokenType::THEN,"Expected 'then'"); node->children.push_back(parseBlock()); }
    if (check(TokenType::ERROR_FINALLY)) { advance(); expect(TokenType::THEN,"Expected 'then'"); node->children.push_back(parseBlock()); }
    return node;
}

ASTNodePtr Parser::parseThrow() {
    int line = peek().line; advance();
    auto node = makeNode(NodeType::ThrowError, line);
    node->children.push_back(parseExpr()); return node;
}

ASTNodePtr Parser::parseSwitch() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN,"Expected '('");
    auto node = makeNode(NodeType::SwitchBlock, line);
    node->children.push_back(parseExpr());
    expect(TokenType::RPAREN,"Expected ')'");
    expect(TokenType::THEN,"Expected 'then'");
    while (!atEnd() && !check(TokenType::END)) {
        if (check(TokenType::SWITCH_CASE)) {
            int cl=peek().line; advance(); auto c=makeNode(NodeType::SwitchCase,cl); c->children.push_back(parseExpr()); expect(TokenType::THEN,"Expected 'then'"); c->children.push_back(parseBlock()); node->children.push_back(c);
        } else if (check(TokenType::SWITCH_DEFAULT)) {
            int cl=peek().line; advance(); expect(TokenType::THEN,"Expected 'then'"); auto c=makeNode(NodeType::SwitchCase,cl); c->sval="__default__"; c->children.push_back(parseBlock()); node->children.push_back(c);
        } else advance();
    }
    match(TokenType::END); return node;
}

ASTNodePtr Parser::parseStringFunc(TokenType t) {
    int line=peek().line; std::string fname=advance().value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node=makeNode(NodeType::StringFunc,line); node->sval=fname;
    while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}
    expect(TokenType::RPAREN,"Expected ')'");
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseTableFunc(TokenType t) {
    int line=peek().line; std::string fname=advance().value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node=makeNode(NodeType::TableFunc,line); node->sval=fname;
    while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}
    expect(TokenType::RPAREN,"Expected ')'");
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseFileFunc(TokenType t) {
    int line=peek().line; std::string fname=advance().value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node=makeNode(NodeType::FileFunc,line); node->sval=fname;
    while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}
    expect(TokenType::RPAREN,"Expected ')'");
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseOsFunc(TokenType t) {
    int line=peek().line; std::string fname=advance().value;
    auto node=makeNode(NodeType::OsFunc,line); node->sval=fname;
    if(check(TokenType::LPAREN)){advance();while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}expect(TokenType::RPAREN,"Expected ')'");}
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseTypeFunc(TokenType t) {
    int line=peek().line; std::string fname=advance().value;
    expect(TokenType::LPAREN,"Expected '('");
    auto node=makeNode(NodeType::TypeFunc,line); node->sval=fname;
    while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}
    expect(TokenType::RPAREN,"Expected ')'");
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseGenericFunc(NodeType nt) {
    int line=peek().line;
    std::string fname=advance().value;
    auto node=makeNode(nt,line); node->sval=fname;
    if(check(TokenType::LPAREN)){
        advance();
        while(!check(TokenType::RPAREN)&&!atEnd()){node->children.push_back(parseExpr());match(TokenType::COMMA);}
        expect(TokenType::RPAREN,"Expected ')'");
    }
    if(check(TokenType::ASSIGN)){advance();node->tags.push_back(expect(TokenType::IDENTIFIER,"Expected var").value);}
    return node;
}

ASTNodePtr Parser::parseRangeCall() {
    int line=peek().line; advance();
    expect(TokenType::SEMICOLON,"Expected ';'");
    auto node=makeNode(NodeType::RangeCall,line);
    node->children.push_back(parseExpr());
    if(!match(TokenType::TO)) throw TeleosError(ErrorCode::SYNTAX_ERROR,"Expected 'to'",peek().line);
    node->children.push_back(parseExpr());
    if(check(TokenType::SEMICOLON)){advance();node->sval=expect(TokenType::IDENTIFIER,"Expected var").value;}
    return node;
}

ASTNodePtr Parser::parseEnumDef() {
    int line=peek().line; advance();
    std::string name=expect(TokenType::IDENTIFIER,"Expected enum name").value;
    expect(TokenType::THEN,"Expected 'then'");
    auto node=makeNode(NodeType::EnumDef,line); node->sval=name;
    while(!atEnd()&&!check(TokenType::END)){if(check(TokenType::IDENTIFIER))node->tags.push_back(advance().value);match(TokenType::COMMA);}
    match(TokenType::END); return node;
}

ASTNodePtr Parser::parseAssert() {
    int line = peek().line; advance();
    auto node = makeNode(NodeType::AssertStmt, line);
    node->children.push_back(parseExpr());
    if (check(TokenType::COMMA)) { advance(); node->children.push_back(parseExpr()); }
    return node;
}

ASTNodePtr Parser::parseInspect() {
    int line = peek().line; advance();
    expect(TokenType::LPAREN, "Expected '('");
    auto node = makeNode(NodeType::InspectStmt, line);
    node->children.push_back(parseExpr());
    expect(TokenType::RPAREN, "Expected ')'");
    return node;
}

ASTNodePtr Parser::parseWhile() {
    int line = peek().line; advance();
    auto cond = makeNode(NodeType::BinaryExpr, line);
    cond->children.push_back(parseAddSub());
    if      (check(TokenType::EQ))  { cond->sval="=="; advance(); }
    else if (check(TokenType::NEQ)) { cond->sval="!="; advance(); }
    else if (check(TokenType::LT))  { cond->sval="<";  advance(); }
    else if (check(TokenType::GT))  { cond->sval=">";  advance(); }
    else if (check(TokenType::LEQ)) { cond->sval="<="; advance(); }
    else if (check(TokenType::GEQ)) { cond->sval=">="; advance(); }
    cond->children.push_back(parseExpr());
    expect(TokenType::THEN, "Expected 'then'");
    auto node = makeNode(NodeType::WhileLoop, line);
    node->children.push_back(cond);
    node->children.push_back(parseBlock());
    return node;
}

ASTNodePtr Parser::parseIncr() {
    int line = peek().line;
    std::string name = advance().value;
    advance(); // eat ++
    auto node = makeNode(NodeType::IncrStmt, line);
    node->sval = name; return node;
}

ASTNodePtr Parser::parseDecr() {
    int line = peek().line;
    std::string name = advance().value;
    advance(); // eat --
    auto node = makeNode(NodeType::DecrStmt, line);
    node->sval = name; return node;
}

ASTNodePtr Parser::parseBlock() {
    auto block=makeNode(NodeType::Block);
    while(!atEnd()&&!check(TokenType::END)&&!check(TokenType::FUNCTION_END)&&!check(TokenType::ERROR_CATCH)&&!check(TokenType::ERROR_FINALLY)&&!check(TokenType::SWITCH_CASE)&&!check(TokenType::SWITCH_DEFAULT)) {
        if(check(TokenType::COMMENT)){auto n=makeNode(NodeType::Comment,peek().line);n->sval=advance().value;block->children.push_back(n);continue;}
        auto stmt=parseStatement(); if(stmt) block->children.push_back(stmt);
    }
    match(TokenType::END); return block;
}

ASTNodePtr Parser::parseExpr() { return parseOr(); }

ASTNodePtr Parser::parseOr() {
    auto left=parseAnd();
    while(check(TokenType::OR)||check(TokenType::OR_OP)){advance();auto right=parseAnd();auto node=makeNode(NodeType::BinaryExpr,left->line);node->sval="||";node->children.push_back(left);node->children.push_back(right);left=node;}
    return left;
}

ASTNodePtr Parser::parseAnd() {
    auto left=parseComparison();
    while(check(TokenType::AND)||check(TokenType::AND_OP)){advance();auto right=parseComparison();auto node=makeNode(NodeType::BinaryExpr,left->line);node->sval="&&";node->children.push_back(left);node->children.push_back(right);left=node;}
    return left;
}

ASTNodePtr Parser::parseComparison() {
    auto left=parseAddSub();
    while(check(TokenType::EQ)||check(TokenType::NEQ)||check(TokenType::LT)||check(TokenType::GT)||check(TokenType::LEQ)||check(TokenType::GEQ)){std::string op=advance().value;auto right=parseAddSub();auto node=makeNode(NodeType::BinaryExpr,left->line);node->sval=op;node->children.push_back(left);node->children.push_back(right);left=node;}
    return left;
}

ASTNodePtr Parser::parseAddSub() {
    auto left=parseMulDiv();
    while(check(TokenType::PLUS)||check(TokenType::MINUS)){std::string op=advance().value;auto right=parseMulDiv();auto node=makeNode(NodeType::BinaryExpr,left->line);node->sval=op;node->children.push_back(left);node->children.push_back(right);left=node;}
    return left;
}

ASTNodePtr Parser::parseMulDiv() {
    auto left=parseUnary();
    while(check(TokenType::STAR)||check(TokenType::SLASH)||check(TokenType::PERCENT)||check(TokenType::CARET)){std::string op=advance().value;auto right=parseUnary();auto node=makeNode(NodeType::BinaryExpr,left->line);node->sval=op;node->children.push_back(left);node->children.push_back(right);left=node;}
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if(check(TokenType::NOT_OP)||check(TokenType::BANG)){int line=peek().line;advance();auto node=makeNode(NodeType::UnaryExpr,line);node->sval="!";node->children.push_back(parseUnary());return node;}
    if(check(TokenType::MINUS)){int line=peek().line;advance();auto node=makeNode(NodeType::UnaryExpr,line);node->sval="-";node->children.push_back(parseUnary());return node;}
    return parsePrimary();
}

ASTNodePtr Parser::parsePrimary() {
    int line=peek().line;
    if(check(TokenType::STRING)){auto n=makeNode(NodeType::Literal,line);n->sval=advance().value;return n;}
    if(check(TokenType::NUMBER)){auto n=makeNode(NodeType::Literal,line);n->nval=std::stod(advance().value);n->sval="__number__";return n;}
    if(check(TokenType::TRUE_KW)){auto n=makeNode(NodeType::Literal,line);n->bval=true;n->sval="__bool__";advance();return n;}
    if(check(TokenType::FALSE_KW)){auto n=makeNode(NodeType::Literal,line);n->bval=false;n->sval="__bool__";advance();return n;}
    if(check(TokenType::NIL)){auto n=makeNode(NodeType::Literal,line);n->sval="nil";advance();return n;}
    if(check(TokenType::LBRACE)) return parseTableLiteral();
    if(check(TokenType::LPAREN)){advance();auto expr=parseExpr();expect(TokenType::RPAREN,"Expected ')'");return expr;}

    if(check(TokenType::MATH_FLOOR)||check(TokenType::MATH_CEIL)||check(TokenType::MATH_SQRT)||check(TokenType::MATH_POW)||check(TokenType::MATH_ABS)||check(TokenType::MATH_ROUND)||check(TokenType::MATH_MIN)||check(TokenType::MATH_MAX)||check(TokenType::MATH_RANDOM)||check(TokenType::MATH_SIN)||check(TokenType::MATH_COS)||check(TokenType::MATH_TAN)||check(TokenType::MATH_LOG)||check(TokenType::MATH_MOD)) return parseMathFunc(peek().type);
    if(check(TokenType::STRING_LEN)||check(TokenType::STRING_UPPER)||check(TokenType::STRING_LOWER)||check(TokenType::STRING_TRIM)||check(TokenType::STRING_SPLIT)||check(TokenType::STRING_JOIN)||check(TokenType::STRING_CONTAINS)||check(TokenType::STRING_REPLACE)||check(TokenType::STRING_STARTS)||check(TokenType::STRING_ENDS)||check(TokenType::STRING_SUB)||check(TokenType::STRING_FIND)||check(TokenType::STRING_REPEAT)||check(TokenType::STRING_REVERSE)||check(TokenType::STRING_FORMAT)) return parseStringFunc(peek().type);
    if(check(TokenType::TABLE_LEN)||check(TokenType::TABLE_GET)||check(TokenType::TABLE_CONTAINS)||check(TokenType::TABLE_KEYS)||check(TokenType::TABLE_SLICE)||check(TokenType::TABLE_MAP)||check(TokenType::TABLE_FILTER)) return parseTableFunc(peek().type);
    if(check(TokenType::FILE_READ)||check(TokenType::FILE_EXISTS)||check(TokenType::FILE_LINES)||check(TokenType::FILE_SIZE)) return parseFileFunc(peek().type);
    if(check(TokenType::TYPE_OF)||check(TokenType::TYPE_CAST)||check(TokenType::TYPE_IS)) return parseTypeFunc(peek().type);
    if(check(TokenType::CRYPTO_HASH)||check(TokenType::CRYPTO_B64ENC)||check(TokenType::CRYPTO_B64DEC)||check(TokenType::CRYPTO_HEX)) return parseGenericFunc(NodeType::CryptoFunc);
    if(check(TokenType::JSON_PARSE)||check(TokenType::JSON_STRINGIFY)) return parseGenericFunc(NodeType::JsonFunc);
    if(check(TokenType::NET_PING)||check(TokenType::NET_PORT)||check(TokenType::NET_HOST)) return parseGenericFunc(NodeType::NetFunc);
    if(check(TokenType::REGEX_MATCH)||check(TokenType::REGEX_TEST)||check(TokenType::REGEX_REPLACE)||check(TokenType::REGEX_FIND)) return parseGenericFunc(NodeType::RegexFunc);
    if(check(TokenType::TIME_NOW)||check(TokenType::TIME_FORMAT)||check(TokenType::TIME_DIFF)||check(TokenType::TIME_STAMP)) return parseGenericFunc(NodeType::TimeFunc);
    if(check(TokenType::STRING_PAD_LEFT)||check(TokenType::STRING_PAD_RIGHT)||check(TokenType::STRING_COUNT)||check(TokenType::STRING_WRAP)) return parseStringFunc(peek().type);
    if(check(TokenType::MATH_CLAMP)||check(TokenType::MATH_LERP)||check(TokenType::MATH_MAP)) return parseMathFunc(peek().type);
    if(check(TokenType::FETCH_GET)||check(TokenType::FETCH_POST_KW)) return parseGenericFunc(NodeType::FetchSimple);
    if(check(TokenType::OS_TIME)||check(TokenType::OS_DATE)||check(TokenType::OS_CWD)||check(TokenType::OS_PLATFORM)||check(TokenType::OS_ENV)||check(TokenType::OS_ARGS)) return parseOsFunc(peek().type);
    if(check(TokenType::FUNCTION_COMPARE)){advance();if(check(TokenType::LPAREN)){advance();if(!check(TokenType::RPAREN))advance();expect(TokenType::RPAREN,"Expected ')'");}auto n=makeNode(NodeType::Identifier,line);n->sval="function.compare.current";return n;}
    if(check(TokenType::IDENTIFIER)){
        auto n=makeNode(NodeType::Identifier,line);n->sval=advance().value;
        if(check(TokenType::LPAREN)){advance();auto call=makeNode(NodeType::FuncCallUser,line);call->sval=n->sval;while(!check(TokenType::RPAREN)&&!atEnd()){call->children.push_back(parseExpr());match(TokenType::COMMA);}expect(TokenType::RPAREN,"Expected ')'");return call;}
        if(check(TokenType::LBRACKET)){advance();auto idx=makeNode(NodeType::BinaryExpr,line);idx->sval="[]";idx->children.push_back(n);idx->children.push_back(parseExpr());expect(TokenType::RBRACKET,"Expected ']'");return idx;}
        return n;
    }
    auto tok=advance();auto n=makeNode(NodeType::Literal,line);n->sval=tok.value;return n;
}

ASTNodePtr Parser::parseTableLiteral() {
    int line=peek().line;
    expect(TokenType::LBRACE,"Expected '{'");
    auto node=makeNode(NodeType::TableLiteral,line);
    while(!check(TokenType::RBRACE)&&!atEnd()){
        if(check(TokenType::LBRACKET)){advance();node->children.push_back(parseExpr());match(TokenType::RBRACKET);}
        else node->children.push_back(parseExpr());
        match(TokenType::COMMA);
    }
    expect(TokenType::RBRACE,"Expected '}'"); return node;
}

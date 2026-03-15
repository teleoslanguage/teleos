#include "lexer.h"
#include <stdexcept>
#include <unordered_map>
#include <sstream>

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"create", TokenType::CREATE},
    {"var", TokenType::VAR},
    {"call", TokenType::CALL},
    {"function", TokenType::FUNCTION},
    {"do", TokenType::DO},
    {"end", TokenType::END},
    {"then", TokenType::THEN},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"unless", TokenType::UNLESS},
    {"why", TokenType::WHY},
    {"repeat", TokenType::REPEAT},
    {"until", TokenType::UNTIL},
    {"nil", TokenType::NIL},
    {"num", TokenType::NUM_KW},
    {"true", TokenType::TRUE_KW},
    {"false", TokenType::FALSE_KW},
    {"import", TokenType::IMPORT},
    {"export", TokenType::EXPORT},
    {"module", TokenType::MODULE},
    {"global", TokenType::GLOBAL},
    {"local", TokenType::LOCAL_KW},
    {"to", TokenType::TO},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT_OP},
    {"return", TokenType::FUNC_RETURN},
    {"throw", TokenType::ERROR_THROW},
    {"try", TokenType::ERROR_TRY},
    {"catch", TokenType::ERROR_CATCH},
    {"finally", TokenType::ERROR_FINALLY},
    {"switch", TokenType::SWITCH_DO},
    {"case", TokenType::SWITCH_CASE},
    {"default", TokenType::SWITCH_DEFAULT},
    {"const", TokenType::CONST_DEF},
    {"define", TokenType::FUNC_DEF},
    {"assert", TokenType::ASSERT_KW},
    {"inspect", TokenType::INSPECT_KW},
    {"while", TokenType::WHILE_KW},
    {"break", TokenType::BREAK_KW},
    {"continue", TokenType::CONTINUE_KW},
    {"math.clamp", TokenType::MATH_CLAMP},
    {"math.lerp", TokenType::MATH_LERP},
    {"math.map", TokenType::MATH_MAP},
    {"math.pi", TokenType::MATH_PI},
    {"math.e", TokenType::MATH_E},
    {"string.pad_left", TokenType::STRING_PAD_LEFT},
    {"string.pad_right", TokenType::STRING_PAD_RIGHT},
    {"string.count", TokenType::STRING_COUNT},
    {"string.wrap", TokenType::STRING_WRAP},
    {"crypto.hash", TokenType::CRYPTO_HASH},
    {"crypto.b64enc", TokenType::CRYPTO_B64ENC},
    {"crypto.b64dec", TokenType::CRYPTO_B64DEC},
    {"crypto.hex", TokenType::CRYPTO_HEX},
    {"json.parse", TokenType::JSON_PARSE},
    {"json.stringify", TokenType::JSON_STRINGIFY},
    {"clipboard.read", TokenType::CLIP_READ},
    {"clipboard.write", TokenType::CLIP_WRITE},
    {"net.ping", TokenType::NET_PING},
    {"net.port", TokenType::NET_PORT},
    {"net.host", TokenType::NET_HOST},
    {"regex.match", TokenType::REGEX_MATCH},
    {"regex.test", TokenType::REGEX_TEST},
    {"regex.replace", TokenType::REGEX_REPLACE},
    {"regex.find", TokenType::REGEX_FIND},
    {"time.now", TokenType::TIME_NOW},
    {"time.format", TokenType::TIME_FORMAT},
    {"time.diff", TokenType::TIME_DIFF},
    {"time.stamp", TokenType::TIME_STAMP},
    {"proc.pid", TokenType::PROC_PID},
    {"proc.list", TokenType::PROC_LIST},
    {"proc.kill", TokenType::PROC_KILL},
    {"console.table", TokenType::CONSOLE_TABLE},
    {"console.bell", TokenType::CONSOLE_BELL},
    {"fetch.get", TokenType::FETCH_GET},
    {"fetch.post", TokenType::FETCH_POST_KW},
    {"enum", TokenType::ENUM_DEF},
};

static const std::unordered_map<std::string, TokenType> DOTTED = {
    {"console.output",          TokenType::CONSOLE_OUTPUT},
    {"console.print",           TokenType::CONSOLE_PRINT},
    {"console.println",         TokenType::CONSOLE_PRINTLN},
    {"console.ask",             TokenType::CONSOLE_ASK},
    {"console.clear",           TokenType::CONSOLE_CLEAR},
    {"console.color",           TokenType::CONSOLE_COLOR},
    {"console.title",           TokenType::CONSOLE_TITLE},
    {"console.wait",            TokenType::CONSOLE_WAIT},
    {"console.input",           TokenType::CONSOLE_INPUT},
    {"math.call",               TokenType::MATH_CALL},
    {"math.floor",              TokenType::MATH_FLOOR},
    {"math.ceil",               TokenType::MATH_CEIL},
    {"math.sqrt",               TokenType::MATH_SQRT},
    {"math.pow",                TokenType::MATH_POW},
    {"math.abs",                TokenType::MATH_ABS},
    {"math.round",              TokenType::MATH_ROUND},
    {"math.min",                TokenType::MATH_MIN},
    {"math.max",                TokenType::MATH_MAX},
    {"math.random",             TokenType::MATH_RANDOM},
    {"math.sin",                TokenType::MATH_SIN},
    {"math.cos",                TokenType::MATH_COS},
    {"math.tan",                TokenType::MATH_TAN},
    {"math.log",                TokenType::MATH_LOG},
    {"math.mod",                TokenType::MATH_MOD},
    {"system.hardware",         TokenType::SYSTEM_HARDWARE},
    {"app.open",                TokenType::APP_OPEN},
    {"calllog",                 TokenType::CALLLOG},
    {"fetch",                   TokenType::FETCH},
    {"fetch.post",              TokenType::FETCH_POST},
    {"string.len",              TokenType::STRING_LEN},
    {"string.upper",            TokenType::STRING_UPPER},
    {"string.lower",            TokenType::STRING_LOWER},
    {"string.trim",             TokenType::STRING_TRIM},
    {"string.split",            TokenType::STRING_SPLIT},
    {"string.join",             TokenType::STRING_JOIN},
    {"string.contains",         TokenType::STRING_CONTAINS},
    {"string.replace",          TokenType::STRING_REPLACE},
    {"string.starts",           TokenType::STRING_STARTS},
    {"string.ends",             TokenType::STRING_ENDS},
    {"string.sub",              TokenType::STRING_SUB},
    {"string.find",             TokenType::STRING_FIND},
    {"string.repeat",           TokenType::STRING_REPEAT},
    {"string.reverse",          TokenType::STRING_REVERSE},
    {"string.format",           TokenType::STRING_FORMAT},
    {"table.push",              TokenType::TABLE_PUSH},
    {"table.pop",               TokenType::TABLE_POP},
    {"table.len",               TokenType::TABLE_LEN},
    {"table.get",               TokenType::TABLE_GET},
    {"table.set",               TokenType::TABLE_SET},
    {"table.remove",            TokenType::TABLE_REMOVE},
    {"table.contains",          TokenType::TABLE_CONTAINS},
    {"table.keys",              TokenType::TABLE_KEYS},
    {"table.merge",             TokenType::TABLE_MERGE},
    {"table.sort",              TokenType::TABLE_SORT},
    {"table.slice",             TokenType::TABLE_SLICE},
    {"table.map",               TokenType::TABLE_MAP},
    {"table.filter",            TokenType::TABLE_FILTER},
    {"table.each",              TokenType::TABLE_EACH},
    {"file.read",               TokenType::FILE_READ},
    {"file.write",              TokenType::FILE_WRITE},
    {"file.append",             TokenType::FILE_APPEND},
    {"file.exists",             TokenType::FILE_EXISTS},
    {"file.delete",             TokenType::FILE_DELETE},
    {"file.lines",              TokenType::FILE_LINES},
    {"file.copy",               TokenType::FILE_COPY},
    {"file.move",               TokenType::FILE_MOVE},
    {"file.size",               TokenType::FILE_SIZE},
    {"file.mkdir",              TokenType::FILE_MKDIR},
    {"type.of",                 TokenType::TYPE_OF},
    {"type.cast",               TokenType::TYPE_CAST},
    {"type.is",                 TokenType::TYPE_IS},
    {"os.env",                  TokenType::OS_ENV},
    {"os.exit",                 TokenType::OS_EXIT},
    {"os.time",                 TokenType::OS_TIME},
    {"os.date",                 TokenType::OS_DATE},
    {"os.sleep",                TokenType::OS_SLEEP},
    {"os.run",                  TokenType::OS_RUN},
    {"os.cwd",                  TokenType::OS_CWD},
    {"os.platform",             TokenType::OS_PLATFORM},
    {"os.args",                 TokenType::OS_ARGS},
    {"function.end",            TokenType::FUNCTION_END},
    {"function.compare.current",TokenType::FUNCTION_COMPARE},
    {"range.call",              TokenType::RANGE_CALL},
};

static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool isAlNum(char c) { return isAlpha(c) || isDigit(c); }

std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t i = 0;
    int line = 1;
    size_t len = source.size();

    auto peek = [&](size_t off = 0) -> char {
        if (i + off < len) return source[i + off];
        return '\0';
    };

    while (i < len) {
        char c = source[i];

        if (c == '\n') { line++; i++; continue; }
        if (c == '\r' || c == ' ' || c == '\t') { i++; continue; }

        if (c == '`' && peek(1) == '`' && peek(2) == '`') {
            i += 3;
            if (i < len && source[i] == '.') i++;
            std::string para;
            while (i < len) {
                if (source[i] == '`' && i+2 < len && source[i+1] == '`' && source[i+2] == '`') {
                    i += 3;
                    if (i < len && source[i] == ',') i++;
                    break;
                }
                if (source[i] == '\n') line++;
                para += source[i++];
            }
            tokens.emplace_back(TokenType::PARAGRAPH, para, line);
            continue;
        }

        if (c == '`') {
            i++;
            std::string comment;
            while (i < len && source[i] != '`') {
                if (source[i] == '\n') line++;
                comment += source[i++];
            }
            if (i < len) i++;
            tokens.emplace_back(TokenType::COMMENT, comment, line);
            continue;
        }

        if (c == '"') {
            i++;
            std::string s;
            while (i < len && source[i] != '"') {
                if (source[i] == '\\' && i+1 < len) {
                    char esc = source[i+1];
                    if (esc == 'n') s += '\n';
                    else if (esc == 't') s += '\t';
                    else if (esc == '"') s += '"';
                    else if (esc == '\\') s += '\\';
                    else s += esc;
                    i += 2;
                } else {
                    if (source[i] == '\n') line++;
                    s += source[i++];
                }
            }
            if (i < len) i++;
            tokens.emplace_back(TokenType::STRING, s, line);
            continue;
        }

        if (isDigit(c) || (c == '-' && isDigit(peek(1)) && (tokens.empty() || tokens.back().type == TokenType::ASSIGN || tokens.back().type == TokenType::ARROW || tokens.back().type == TokenType::LPAREN || tokens.back().type == TokenType::COMMA))) {
            std::string num;
            if (c == '-') { num += c; i++; }
            while (i < len && (isDigit(source[i]) || source[i] == '.')) num += source[i++];
            tokens.emplace_back(TokenType::NUMBER, num, line);
            continue;
        }

        if (c == '|' && peek(1) == '>') {
            tokens.emplace_back(TokenType::PIPE_OP, "|>", line);
            i += 2;
            continue;
        }

        if (c == '&' && peek(1) == '&') {
            tokens.emplace_back(TokenType::AND_OP, "&&", line);
            i += 2;
            continue;
        }

        if (c == '|' && peek(1) == '|') {
            tokens.emplace_back(TokenType::OR_OP, "||", line);
            i += 2;
            continue;
        }

        if (isAlpha(c)) {
            std::string word;
            while (i < len && (isAlNum(source[i]) || source[i] == '.' || source[i] == '_')) {
                if (source[i] == '.' && (i+1 >= len || (!isAlpha(source[i+1]) && source[i+1] != '_'))) break;
                word += source[i++];
            }

            if (word.rfind("console.check.", 0) == 0) {
                tokens.emplace_back(TokenType::CONSOLE_CHECK, word, line);
                continue;
            }

            auto dit = DOTTED.find(word);
            if (dit != DOTTED.end()) {
                tokens.emplace_back(dit->second, word, line);
                continue;
            }

            auto kit = KEYWORDS.find(word);
            if (kit != KEYWORDS.end()) {
                tokens.emplace_back(kit->second, word, line);
                continue;
            }

            tokens.emplace_back(TokenType::IDENTIFIER, word, line);
            continue;
        }

        if (c == '+' && peek(1) == '+') { tokens.emplace_back(TokenType::INCR, "++", line); i += 2; continue; }
        if (c == '-' && peek(1) == '-') { tokens.emplace_back(TokenType::DECR, "--", line); i += 2; continue; }

        switch (c) {
            case '=':
                if (peek(1) == '=') { tokens.emplace_back(TokenType::EQ, "==", line); i += 2; }
                else { tokens.emplace_back(TokenType::ASSIGN, "=", line); i++; }
                break;
            case '!':
                if (peek(1) == '=') { tokens.emplace_back(TokenType::NEQ, "!=", line); i += 2; }
                else { tokens.emplace_back(TokenType::BANG, "!", line); i++; }
                break;
            case '<':
                if (peek(1) == '=') { tokens.emplace_back(TokenType::LEQ, "<=", line); i += 2; }
                else { tokens.emplace_back(TokenType::LT, "<", line); i++; }
                break;
            case '>':
                if (peek(1) == '=') { tokens.emplace_back(TokenType::GEQ, ">=", line); i += 2; }
                else { tokens.emplace_back(TokenType::GT, ">", line); i++; }
                break;
            case '^': tokens.emplace_back(TokenType::CARET, "^", line); i++; break;
            case '+': tokens.emplace_back(TokenType::PLUS, "+", line); i++; break;
            case '-': tokens.emplace_back(TokenType::MINUS, "-", line); i++; break;
            case '*': tokens.emplace_back(TokenType::STAR, "*", line); i++; break;
            case '/': tokens.emplace_back(TokenType::SLASH, "/", line); i++; break;
            case '%': tokens.emplace_back(TokenType::PERCENT, "%", line); i++; break;
            case '(': tokens.emplace_back(TokenType::LPAREN, "(", line); i++; break;
            case ')': tokens.emplace_back(TokenType::RPAREN, ")", line); i++; break;
            case '[': tokens.emplace_back(TokenType::LBRACKET, "[", line); i++; break;
            case ']': tokens.emplace_back(TokenType::RBRACKET, "]", line); i++; break;
            case '{': tokens.emplace_back(TokenType::LBRACE, "{", line); i++; break;
            case '}': tokens.emplace_back(TokenType::RBRACE, "}", line); i++; break;
            case ';': tokens.emplace_back(TokenType::SEMICOLON, ";", line); i++; break;
            case ':': tokens.emplace_back(TokenType::COLON, ":", line); i++; break;
            case ',': tokens.emplace_back(TokenType::COMMA, ",", line); i++; break;
            case '.': tokens.emplace_back(TokenType::DOT, ".", line); i++; break;
            default: tokens.emplace_back(TokenType::UNKNOWN, std::string(1,c), line); i++; break;
        }
    }
    tokens.emplace_back(TokenType::END_OF_FILE, "", line);
    return tokens;
}

std::string tokenTypeName(TokenType t) {
    switch(t) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "TOKEN";
    }
}

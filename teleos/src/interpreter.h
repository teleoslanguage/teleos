#pragma once
#include "ast.h"
#include "value.h"
#include "errors.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct ReturnSignal { Value val; };
struct TeleosThrow { Value val; };

struct FuncDef {
    std::vector<std::string> params;
    ASTNodePtr body;
};

class Environment {
public:
    std::unordered_map<std::string, Value> vars;
    std::unordered_map<std::string, bool> consts;
    std::shared_ptr<Environment> parent;
    Environment(std::shared_ptr<Environment> p = nullptr) : parent(p) {}
    Value get(const std::string& name) const;
    void set(const std::string& name, const Value& val);
    void define(const std::string& name, const Value& val, bool isConst = false);
    bool has(const std::string& name) const;
};

class Interpreter {
public:
    Interpreter();
    void run(ASTNodePtr node);
    void loadModule(const std::string& filename);
    std::unordered_map<std::string, Value> exportedVars;

private:
    std::shared_ptr<Environment> globalEnv;
    std::shared_ptr<Environment> currentEnv;
    std::unordered_map<std::string, FuncDef> userFuncs;
    std::unordered_map<std::string, Value> enumVals;
    bool repeatFinish = false;
    std::vector<std::string> programArgs;

    Value eval(ASTNodePtr node);
    void execBlock(ASTNodePtr block);
    Value evalExpr(ASTNodePtr node);
    Value evalBinary(ASTNodePtr node);
    Value evalUnary(ASTNodePtr node);
    Value evalLiteral(ASTNodePtr node);
    Value evalIdentifier(ASTNodePtr node);
    bool evalCondition(ASTNodePtr condNode);

    void execVarDecl(ASTNodePtr node, bool isConst);
    void execVarAssign(ASTNodePtr node);
    void execCallVar(ASTNodePtr node);
    void execCallFunction(ASTNodePtr node);
    void execConsoleOutput(ASTNodePtr node);
    void execConsoleAsk(ASTNodePtr node);
    void execConsoleCheck(ASTNodePtr node);
    void execMathCall(ASTNodePtr node);
    Value execMathFunc(ASTNodePtr node);
    void execSystemHardware(ASTNodePtr node);
    void execAppOpen(ASTNodePtr node);
    void execCallLog(ASTNodePtr node);
    void execFetch(ASTNodePtr node);
    void execImport(ASTNodePtr node);
    void execRepeat(ASTNodePtr node);
    void execIf(ASTNodePtr node);
    void execUnless(ASTNodePtr node);
    void execWhy(ASTNodePtr node);
    void execCompare(ASTNodePtr node);
    void execFuncDef(ASTNodePtr node);
    Value execFuncCall(ASTNodePtr node);
    void execTryCatch(ASTNodePtr node);
    void execSwitch(ASTNodePtr node);
    Value execStringFunc(ASTNodePtr node);
    Value execTableFunc(ASTNodePtr node);
    Value execFileFunc(ASTNodePtr node);
    Value execOsFunc(ASTNodePtr node);
    Value execTypeFunc(ASTNodePtr node);
    Value execRangeCall(ASTNodePtr node);
    void execEnumDef(ASTNodePtr node);
    Value execCryptoFunc(ASTNodePtr node);
    Value execJsonFunc(ASTNodePtr node);
    Value execClipFunc(ASTNodePtr node);
    Value execNetFunc(ASTNodePtr node);
    Value execRegexFunc(ASTNodePtr node);
    Value execTimeFunc(ASTNodePtr node);
    Value execProcFunc(ASTNodePtr node);
    void execConsoleTable(ASTNodePtr node);
    Value execFetchSimple(ASTNodePtr node);

    std::string queryHardware(const std::string& type);
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& body);
};

#include "interpreter.h"
#include "obfuscator.h"
#include "lexer.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#else
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#endif

Value Environment::get(const std::string& name) const {
    auto it = vars.find(name);
    if (it != vars.end()) return it->second;
    if (parent) return parent->get(name);
    throw TeleosError(ErrorCode::UNDEFINED_VARIABLE, "Variable '" + name + "' is not defined");
}

void Environment::set(const std::string& name, const Value& val) {
    if (consts.count(name)) throw TeleosError(ErrorCode::RUNTIME_ERROR, "Cannot reassign constant '" + name + "'");
    if (vars.count(name)) { vars[name] = val; return; }
    if (parent && parent->has(name)) { parent->set(name, val); return; }
    vars[name] = val;
}

void Environment::define(const std::string& name, const Value& val, bool isConst) {
    vars[name] = val;
    if (isConst) consts[name] = true;
}

bool Environment::has(const std::string& name) const {
    if (vars.count(name)) return true;
    if (parent) return parent->has(name);
    return false;
}

Interpreter::Interpreter() {
    globalEnv = std::make_shared<Environment>();
    currentEnv = globalEnv;
    std::srand((unsigned)std::time(nullptr));
}

void Interpreter::run(ASTNodePtr node) {
    if (!node) return;
    if (node->type == NodeType::Program || node->type == NodeType::Block) {
        for (auto& child : node->children) {
            if (child) eval(child);
            if (repeatFinish) break;
        }
        return;
    }
    eval(node);
}

Value Interpreter::eval(ASTNodePtr node) {
    if (!node) return nilValue();
    switch (node->type) {
        case NodeType::Comment:
        case NodeType::Paragraph: return nilValue();
        case NodeType::VarDecl: execVarDecl(node, false); return nilValue();
        case NodeType::ConstDecl: execVarDecl(node, true); return nilValue();
        case NodeType::VarAssign: execVarAssign(node); return nilValue();
        case NodeType::CallVar: execCallVar(node); return nilValue();
        case NodeType::CallFunction: execCallFunction(node); return nilValue();
        case NodeType::ConsoleOutput: execConsoleOutput(node); return nilValue();
        case NodeType::ConsoleAsk: execConsoleAsk(node); return nilValue();
        case NodeType::ConsoleCheck: execConsoleCheck(node); return nilValue();
        case NodeType::ConsoleClear:
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif
            return nilValue();
        case NodeType::ConsoleColor: {
            Value col = eval(node->children[0]);
#ifdef _WIN32
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            int c = 7;
            std::string cs = col.toString();
            if (cs=="red")c=12; else if(cs=="green")c=10; else if(cs=="blue")c=9; else if(cs=="yellow")c=14; else if(cs=="white")c=15; else if(cs=="cyan")c=11; else if(cs=="magenta")c=13; else if(cs=="black")c=0; else if(cs=="gray")c=8;
            SetConsoleTextAttribute(h, c);
#else
            std::string cs = col.toString();
            if(cs=="red") std::cout<<"\033[31m"; else if(cs=="green") std::cout<<"\033[32m"; else if(cs=="blue") std::cout<<"\033[34m"; else if(cs=="yellow") std::cout<<"\033[33m"; else if(cs=="white") std::cout<<"\033[37m"; else if(cs=="cyan") std::cout<<"\033[36m"; else if(cs=="magenta") std::cout<<"\033[35m"; else if(cs=="reset") std::cout<<"\033[0m";
#endif
            return nilValue();
        }
        case NodeType::ConsoleTitle: {
            Value t = eval(node->children[0]);
#ifdef _WIN32
            SetConsoleTitleA(t.toString().c_str());
#else
            std::cout << "\033]0;" << t.toString() << "\007";
#endif
            return nilValue();
        }
        case NodeType::ConsoleWait: {
            int ms = 1000;
            if (!node->children.empty()) ms = (int)(eval(node->children[0]).nval);
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            return nilValue();
        }
        case NodeType::ConsoleInput: {
            Value prompt = node->children.empty() ? strValue("") : eval(node->children[0]);
            if (!prompt.toString().empty()) std::cout << prompt.toString();
            std::string input; std::getline(std::cin, input);
            Value v = strValue(input);
            if (!node->sval.empty()) currentEnv->define(node->sval, v);
            return v;
        }
        case NodeType::MathCall: execMathCall(node); return nilValue();
        case NodeType::MathFunc: { Value v = execMathFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::SystemHardware: execSystemHardware(node); return nilValue();
        case NodeType::AppOpen: execAppOpen(node); return nilValue();
        case NodeType::CallLog: execCallLog(node); return nilValue();
        case NodeType::FetchAPI:
        case NodeType::FetchPost: execFetch(node); return nilValue();
        case NodeType::ImportModule: execImport(node); return nilValue();
        case NodeType::ExportModule: for (auto& [k,v]: currentEnv->vars) exportedVars[k]=v; return nilValue();
        case NodeType::RepeatLoop: execRepeat(node); return nilValue();
        case NodeType::RepeatFinish: repeatFinish = true; return nilValue();
        case NodeType::IfStatement: execIf(node); return nilValue();
        case NodeType::UnlessStatement: execUnless(node); return nilValue();
        case NodeType::WhyStatement: execWhy(node); return nilValue();
        case NodeType::CompareBlock: execCompare(node); return nilValue();
        case NodeType::FunctionEndBlock: return nilValue();
        case NodeType::FuncDef: execFuncDef(node); return nilValue();
        case NodeType::FuncCallUser: return execFuncCall(node);
        case NodeType::FuncReturn: { Value v = node->children.empty() ? nilValue() : eval(node->children[0]); throw ReturnSignal{v}; }
        case NodeType::TryCatch: execTryCatch(node); return nilValue();
        case NodeType::ThrowError: { Value v = eval(node->children[0]); throw TeleosThrow{v}; }
        case NodeType::SwitchBlock: execSwitch(node); return nilValue();
        case NodeType::StringFunc: { Value v = execStringFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::TableFunc: { Value v = execTableFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::FileFunc: { Value v = execFileFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::OsFunc: { Value v = execOsFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::TypeFunc: { Value v = execTypeFunc(node); if (!node->tags.empty()) currentEnv->define(node->tags[0], v); return v; }
        case NodeType::RangeCall: { Value v = execRangeCall(node); return v; }
        case NodeType::EnumDef: execEnumDef(node); return nilValue();
        case NodeType::AssertStmt: {
            Value cond = eval(node->children[0]);
            if (!cond.isTruthy()) {
                std::string msg = node->children.size() > 1 ? eval(node->children[1]).toString() : "Assertion failed";
                throw TeleosError(ErrorCode::RUNTIME_ERROR, msg, node->line);
            }
            return nilValue();
        }
        case NodeType::InspectStmt: {
            Value v = eval(node->children[0]);
            std::string tp;
            switch(v.type) {
                case ValueType::Nil:     tp="nil"; break;
                case ValueType::String:  tp="string"; break;
                case ValueType::Number:  tp="number"; break;
                case ValueType::Boolean: tp="boolean"; break;
                case ValueType::Table:   tp="table["+std::to_string(v.table.size())+"]"; break;
            }
            std::cout << "<inspect type=" << tp << " value=" << v.toString() << ">\n";
            return strValue(tp);
        }
        case NodeType::WhileLoop: {
            ASTNodePtr cond = node->children[0];
            ASTNodePtr body = node->children[1];
            int guard = 0;
            while (evalCondition(cond)) {
                try { execBlock(body); }
                catch (...) { break; }
                if (repeatFinish) { repeatFinish = false; break; }
                if (++guard > 100000) throw TeleosError(ErrorCode::RUNTIME_ERROR, "While loop exceeded 100000 iterations", node->line);
            }
            return nilValue();
        }
        case NodeType::BreakStmt:    repeatFinish = true; return nilValue();
        case NodeType::ContinueStmt: return nilValue();
        case NodeType::IncrStmt: {
            Value v = nilValue();
            try { v = currentEnv->get(node->sval); } catch(...) {}
            currentEnv->set(node->sval, numValue(v.nval + 1.0));
            return nilValue();
        }
        case NodeType::DecrStmt: {
            Value v = nilValue();
            try { v = currentEnv->get(node->sval); } catch(...) {}
            currentEnv->set(node->sval, numValue(v.nval - 1.0));
            return nilValue();
        }
        case NodeType::ConsolePrint: {
            Value val = eval(node->children[0]);
            std::cout << val.toString();
            return nilValue();
        }
        case NodeType::CryptoFunc:  { Value v=execCryptoFunc(node);  if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::JsonFunc:    { Value v=execJsonFunc(node);    if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::ClipFunc:    { Value v=execClipFunc(node);    if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::NetFunc:     { Value v=execNetFunc(node);     if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::RegexFunc:   { Value v=execRegexFunc(node);   if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::TimeFunc:    { Value v=execTimeFunc(node);    if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::ProcFunc:    { Value v=execProcFunc(node);    if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::ConsoleTable: execConsoleTable(node); return nilValue();
        case NodeType::ConsoleBell:
#ifdef _WIN32
            MessageBeep(MB_OK);
#else
            std::cout << "" << std::flush;
#endif
            return nilValue();
        case NodeType::FetchSimple: { Value v=execFetchSimple(node); if(!node->tags.empty()) currentEnv->define(node->tags[0],v); return v; }
        case NodeType::Block: execBlock(node); return nilValue();
        case NodeType::BinaryExpr: return evalBinary(node);
        case NodeType::UnaryExpr: return evalUnary(node);
        case NodeType::Literal: return evalLiteral(node);
        case NodeType::Identifier: return evalIdentifier(node);
        case NodeType::TableLiteral: { std::vector<Value> t; for (auto& c: node->children) t.push_back(eval(c)); return Value(t); }
        default: return nilValue();
    }
}

void Interpreter::execBlock(ASTNodePtr block) {
    for (auto& stmt: block->children) { eval(stmt); if (repeatFinish) return; }
}

Value Interpreter::evalLiteral(ASTNodePtr node) {
    if (node->sval == "__number__") return numValue(node->nval);
    if (node->sval == "__bool__") return boolValue(node->bval);
    if (node->sval == "nil") return nilValue();
    return strValue(node->sval);
}

Value Interpreter::evalIdentifier(ASTNodePtr node) {
    auto it = enumVals.find(node->sval);
    if (it != enumVals.end()) return it->second;
    try { return currentEnv->get(node->sval); } catch (...) { return nilValue(); }
}

Value Interpreter::evalUnary(ASTNodePtr node) {
    Value v = eval(node->children[0]);
    if (node->sval == "!") return boolValue(!v.isTruthy());
    if (node->sval == "-") { if (v.type==ValueType::Number) return numValue(-v.nval); throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot negate non-numeric value",node->line); }
    return nilValue();
}

Value Interpreter::evalBinary(ASTNodePtr node) {
    if (node->sval == "[]") {
        Value tbl = eval(node->children[0]);
        Value idx = eval(node->children[1]);
        if (tbl.type == ValueType::Table) {
            int i = (int)idx.nval;
            if (i < 0 || i >= (int)tbl.table.size()) throw TeleosError(ErrorCode::INDEX_OUT_OF_RANGE,"Table index "+std::to_string(i)+" out of range",node->line);
            return tbl.table[i];
        }
        if (tbl.type == ValueType::String) {
            int i = (int)idx.nval;
            if (i < 0 || i >= (int)tbl.sval.size()) throw TeleosError(ErrorCode::INDEX_OUT_OF_RANGE,"String index out of range",node->line);
            return strValue(std::string(1, tbl.sval[i]));
        }
        return nilValue();
    }
    if (node->sval == "&&") { Value l = eval(node->children[0]); if (!l.isTruthy()) return boolValue(false); return boolValue(eval(node->children[1]).isTruthy()); }
    if (node->sval == "||") { Value l = eval(node->children[0]); if (l.isTruthy()) return boolValue(true); return boolValue(eval(node->children[1]).isTruthy()); }

    Value left = eval(node->children[0]);
    Value right = node->children.size() > 1 ? eval(node->children[1]) : nilValue();
    const std::string& op = node->sval;

    if (op=="+") { if(left.type==ValueType::Number&&right.type==ValueType::Number) return numValue(left.nval+right.nval); return strValue(left.toString()+right.toString()); }
    if (op=="-") { if(left.type==ValueType::Number&&right.type==ValueType::Number) return numValue(left.nval-right.nval); throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot subtract non-numeric values",node->line); }
    if (op=="*") { if(left.type==ValueType::Number&&right.type==ValueType::Number) return numValue(left.nval*right.nval); if(left.type==ValueType::String&&right.type==ValueType::Number){std::string r;for(int i=0;i<(int)right.nval;i++)r+=left.sval;return strValue(r);} throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot multiply",node->line); }
    if (op=="/") { if(left.type==ValueType::Number&&right.type==ValueType::Number){if(right.nval==0.0)throw TeleosError(ErrorCode::DIVISION_BY_ZERO,"Division by zero",node->line);return numValue(left.nval/right.nval);}throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot divide",node->line);}
    if (op=="%") { if(left.type==ValueType::Number&&right.type==ValueType::Number){if(right.nval==0.0)throw TeleosError(ErrorCode::DIVISION_BY_ZERO,"Modulo by zero",node->line);return numValue(std::fmod(left.nval,right.nval));}throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot modulo",node->line);}
    if (op=="^") { if(left.type==ValueType::Number&&right.type==ValueType::Number) return numValue(std::pow(left.nval,right.nval)); throw TeleosError(ErrorCode::TYPE_MISMATCH,"Cannot exponentiate",node->line);}
    if (op=="==") return boolValue(left==right);
    if (op=="!=") return boolValue(left!=right);
    if (op=="<")  return boolValue(left<right);
    if (op==">")  return boolValue(left>right);
    if (op=="<=") return boolValue(left<=right);
    if (op==">=") return boolValue(left>=right);
    return nilValue();
}

bool Interpreter::evalCondition(ASTNodePtr condNode) {
    return evalBinary(condNode).isTruthy();
}

void Interpreter::execVarDecl(ASTNodePtr node, bool isConst) {
    Value val = eval(node->children[0]);
    currentEnv->define(node->sval, val, isConst);
}

void Interpreter::execVarAssign(ASTNodePtr node) {
    Value val = eval(node->children[0]);
    currentEnv->set(node->sval, val);
}

void Interpreter::execCallVar(ASTNodePtr node) {
    auto inner = std::make_shared<Environment>(currentEnv);
    auto prev = currentEnv; currentEnv = inner;
    execBlock(node->children[0]);
    currentEnv = prev;
}

void Interpreter::execCallFunction(ASTNodePtr node) {
    if (node->sval == "__do__") { for (auto& c: node->children) eval(c); return; }
    auto inner = std::make_shared<Environment>(currentEnv);
    auto prev = currentEnv; currentEnv = inner;
    if (!node->children.empty()) execBlock(node->children[0]);
    currentEnv = prev;
}

void Interpreter::execConsoleOutput(ASTNodePtr node) {
    Value val = eval(node->children[0]);
    if (node->sval == "print") {
        std::cout << val.toString();
    } else {
        std::cout << val.toString() << "\n";
    }
}

void Interpreter::execConsoleAsk(ASTNodePtr node) {
    Value prompt = eval(node->children[0]);
    std::cout << prompt.toString();
    if (node->children.size() > 1) {
        std::cout << " [";
        for (size_t i = 1; i < node->children.size(); i++) { if (i>1) std::cout << " / "; std::cout << eval(node->children[i]).toString(); }
        std::cout << "] ";
    } else std::cout << " ";
    std::string answer; std::getline(std::cin, answer);
    if (!node->sval.empty()) currentEnv->define(node->sval, strValue(answer));
}

void Interpreter::execConsoleCheck(ASTNodePtr node) {
    auto inner = std::make_shared<Environment>(currentEnv);
    auto prev = currentEnv; currentEnv = inner;
    execBlock(node->children[0]);
    currentEnv = prev;
}

void Interpreter::execMathCall(ASTNodePtr node) {
    Value result = eval(node->children[0]);
    currentEnv->define(node->sval, result);
    if (node->children.size() > 1) execBlock(node->children[1]);
}

Value Interpreter::execMathFunc(ASTNodePtr node) {
    auto args = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="math.floor") return numValue(std::floor(args(0).nval));
    if (fn=="math.ceil")  return numValue(std::ceil(args(0).nval));
    if (fn=="math.sqrt")  return numValue(std::sqrt(args(0).nval));
    if (fn=="math.abs")   return numValue(std::abs(args(0).nval));
    if (fn=="math.round") return numValue(std::round(args(0).nval));
    if (fn=="math.pow")   return numValue(std::pow(args(0).nval, args(1).nval));
    if (fn=="math.min")   return numValue(std::min(args(0).nval, args(1).nval));
    if (fn=="math.max")   return numValue(std::max(args(0).nval, args(1).nval));
    if (fn=="math.sin")   return numValue(std::sin(args(0).nval));
    if (fn=="math.cos")   return numValue(std::cos(args(0).nval));
    if (fn=="math.tan")   return numValue(std::tan(args(0).nval));
    if (fn=="math.log")   return numValue(std::log(args(0).nval));
    if (fn=="math.mod")   { double a=args(0).nval,b=args(1).nval; if(b==0)throw TeleosError(ErrorCode::DIVISION_BY_ZERO,"math.mod by zero",node->line); return numValue(std::fmod(a,b)); }
    if (fn=="math.clamp") { double v=args(0).nval,lo=args(1).nval,hi=args(2).nval; return numValue(std::max(lo,std::min(hi,v))); }
    if (fn=="math.lerp")  { double ma=args(0).nval,mb=args(1).nval,mt=args(2).nval; return numValue(ma+(mb-ma)*mt); }
    if (fn=="math.map")   { double mv=args(0).nval,ma1=args(1).nval,mb1=args(2).nval,ma2=args(3).nval,mb2=args(4).nval; if(mb1-ma1==0)return numValue(ma2); return numValue(ma2+(mv-ma1)/(mb1-ma1)*(mb2-ma2)); }
    if (fn=="math.pi")    { return numValue(3.141592653589793); }
    if (fn=="math.e")     { return numValue(2.718281828459045); }
    if (fn=="math.random") {
        double lo = node->children.size()>=2 ? args(0).nval : 0.0;
        double hi = node->children.size()>=2 ? args(1).nval : (node->children.size()==1 ? args(0).nval : 1.0);
        if (node->children.size()==1){lo=0;hi=args(0).nval;}
        return numValue(lo + ((double)std::rand()/(double)RAND_MAX)*(hi-lo));
    }
    return nilValue();
}

void Interpreter::execSystemHardware(ASTNodePtr node) {
    if (!node->children.empty()) {
        auto inner = std::make_shared<Environment>(currentEnv);
        auto prev = currentEnv; currentEnv = inner;
        execBlock(node->children[0]);
        currentEnv = prev;
        return;
    }
    std::string result = queryHardware(node->sval);
    if (!node->tags.empty()) currentEnv->define(node->tags[0], strValue(result));
    else std::cout << result << "\n";
}

std::string Interpreter::queryHardware(const std::string& type) {
#ifdef _WIN32
    if (type == "cpu") {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        DWORD cores = si.dwNumberOfProcessors;
        char name[256] = {};
        DWORD sz = sizeof(name);
        RegGetValueA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "ProcessorNameString", RRF_RT_REG_SZ, nullptr, name, &sz);
        std::string nameStr(name);
        while (!nameStr.empty() && nameStr.back() == ' ') nameStr.pop_back();
        while (nameStr.size() > 1 && nameStr[0] == ' ') nameStr.erase(nameStr.begin());
        return "CPU: " + (nameStr.empty() ? "Unknown" : nameStr) + " | Cores: " + std::to_string(cores);
    }
    if (type == "cpu_usage") {
        static FILETIME prevIdle={}, prevKernel={}, prevUser={};
        FILETIME idle, kernel, user;
        if (!GetSystemTimes(&idle, &kernel, &user)) return "CPU Usage: unavailable";
        auto toULL = [](FILETIME ft){ return ((ULONGLONG)ft.dwHighDateTime<<32)|ft.dwLowDateTime; };
        ULONGLONG dIdle   = toULL(idle)   - toULL(prevIdle);
        ULONGLONG dKernel = toULL(kernel) - toULL(prevKernel);
        ULONGLONG dUser   = toULL(user)   - toULL(prevUser);
        prevIdle=idle; prevKernel=kernel; prevUser=user;
        ULONGLONG total = dKernel + dUser;
        if (total == 0) return "CPU Usage: 0%";
        double usage = (double)(total - dIdle) / (double)total * 100.0;
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f", usage);
        return std::string("CPU Usage: ") + buf + "%";
    }
    if (type == "ram") {
        MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms);
        GlobalMemoryStatusEx(&ms);
        ULONGLONG total = ms.ullTotalPhys / (1024*1024);
        ULONGLONG avail = ms.ullAvailPhys / (1024*1024);
        ULONGLONG used  = total - avail;
        double pct = total > 0 ? (double)used / (double)total * 100.0 : 0.0;
        char pbuf[16]; snprintf(pbuf, sizeof(pbuf), "%.1f", pct);
        return "RAM: " + std::to_string(total) + " MB total | "
             + std::to_string(used) + " MB used | "
             + std::to_string(avail) + " MB free | "
             + pbuf + "% load";
    }
    if (type == "ram_usage") {
        MEMORYSTATUSEX ms; ms.dwLength = sizeof(ms);
        GlobalMemoryStatusEx(&ms);
        double pct = (double)(ms.ullTotalPhys - ms.ullAvailPhys) / (double)ms.ullTotalPhys * 100.0;
        char buf[16]; snprintf(buf, sizeof(buf), "%.1f", pct);
        return std::string(buf) + "%";
    }
    if (type == "gpu") {
        char gpuName[256] = {};
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Video", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            HKEY subKey;
            char subName[256];
            DWORD idx = 0, subSz = sizeof(subName);
            while (RegEnumKeyExA(hKey, idx++, subName, &subSz, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                subSz = sizeof(subName);
                HKEY vidKey;
                std::string path = std::string(subName) + "\\0000";
                if (RegOpenKeyExA(hKey, path.c_str(), 0, KEY_READ, &vidKey) == ERROR_SUCCESS) {
                    DWORD sz2 = sizeof(gpuName);
                    RegQueryValueExA(vidKey, "DriverDesc", nullptr, nullptr, (LPBYTE)gpuName, &sz2);
                    RegCloseKey(vidKey);
                    if (gpuName[0] != '\0') break;
                }
            }
            RegCloseKey(hKey);
        }
        std::string gn(gpuName);
        return "GPU: " + (gn.empty() ? "Unknown (registry query failed)" : gn);
    }
    if (type == "vram") {
        DWORD vramMB = 0;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Video", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            HKEY subKey;
            char subName[256];
            DWORD idx = 0, subSz = sizeof(subName);
            while (RegEnumKeyExA(hKey, idx++, subName, &subSz, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                subSz = sizeof(subName);
                HKEY vidKey;
                std::string path = std::string(subName) + "\\0000";
                if (RegOpenKeyExA(hKey, path.c_str(), 0, KEY_READ, &vidKey) == ERROR_SUCCESS) {
                    DWORD val = 0, vsz = sizeof(val);
                    if (RegQueryValueExA(vidKey, "HardwareInformation.MemorySize",
                        nullptr, nullptr, (LPBYTE)&val, &vsz) == ERROR_SUCCESS) {
                        vramMB = val / (1024*1024);
                    }
                    RegCloseKey(vidKey);
                    if (vramMB > 0) break;
                }
            }
            RegCloseKey(hKey);
        }
        if (vramMB == 0) return "VRAM: unknown (dedicated GPU may report 0 via registry)";
        return "VRAM: " + std::to_string(vramMB) + " MB";
    }
    if (type == "storage") {
        ULARGE_INTEGER freeBytes, totalBytes;
        if (!GetDiskFreeSpaceExA("C:\\", &freeBytes, &totalBytes, nullptr))
            return "Storage: unavailable";
        ULONGLONG total = totalBytes.QuadPart / (1024ULL*1024*1024);
        ULONGLONG free2 = freeBytes.QuadPart  / (1024ULL*1024*1024);
        ULONGLONG used  = total - free2;
        double pct = total > 0 ? (double)used/(double)total*100.0 : 0.0;
        char pbuf[16]; snprintf(pbuf,sizeof(pbuf),"%.1f",pct);
        return "Storage C: " + std::to_string(total) + " GB total | "
             + std::to_string(used) + " GB used | "
             + std::to_string(free2) + " GB free | "
             + pbuf + "% used";
    }
    if (type == "global") {
        SYSTEM_INFO si; GetSystemInfo(&si);
        MEMORYSTATUSEX ms; ms.dwLength=sizeof(ms); GlobalMemoryStatusEx(&ms);
        char name[256]={}; DWORD sz=sizeof(name);
        RegGetValueA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            "ProcessorNameString",RRF_RT_REG_SZ,nullptr,name,&sz);
        std::string nameStr(name);
        while (!nameStr.empty() && nameStr.back()==' ') nameStr.pop_back();
        ULONGLONG total=ms.ullTotalPhys/(1024*1024);
        ULONGLONG avail=ms.ullAvailPhys/(1024*1024);
        ULONGLONG used=total-avail;
        double pct=total>0?(double)used/(double)total*100.0:0.0;
        char pbuf[16]; snprintf(pbuf,sizeof(pbuf),"%.1f",pct);
        char gpuName[256]={};
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\Video",0,KEY_READ,&hKey)==ERROR_SUCCESS) {
            char subName[256]; DWORD idx=0,subSz=sizeof(subName);
            while(RegEnumKeyExA(hKey,idx++,subName,&subSz,nullptr,nullptr,nullptr,nullptr)==ERROR_SUCCESS) {
                subSz=sizeof(subName);
                HKEY vk; std::string path=std::string(subName)+"\\0000";
                if(RegOpenKeyExA(hKey,path.c_str(),0,KEY_READ,&vk)==ERROR_SUCCESS) {
                    DWORD sz2=sizeof(gpuName);
                    RegQueryValueExA(vk,"DriverDesc",nullptr,nullptr,(LPBYTE)gpuName,&sz2);
                    RegCloseKey(vk);
                    if(gpuName[0]!='\0') break;
                }
            }
            RegCloseKey(hKey);
        }
        ULARGE_INTEGER fb,tb;
        GetDiskFreeSpaceExA("C:\\",&fb,&tb,nullptr);
        return "CPU: " + (nameStr.empty()?"Unknown":nameStr)
             + " (" + std::to_string(si.dwNumberOfProcessors) + " cores)"
             + " | RAM: " + std::to_string(used) + "/" + std::to_string(total) + " MB (" + pbuf + "%)"
             + " | GPU: " + (gpuName[0]?std::string(gpuName):"Unknown")
             + " | Storage C: " + std::to_string(tb.QuadPart/(1024ULL*1024*1024)) + " GB";
    }
    return "Unknown hardware type: " + type;
#else
    if (type == "cpu") {
        std::ifstream f("/proc/cpuinfo");
        std::string line, name;
        int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
        while (std::getline(f, line)) {
            if (line.find("model name") != std::string::npos) {
                size_t p = line.find(':');
                if (p != std::string::npos) { name = line.substr(p+2); break; }
            }
        }
        while (!name.empty() && name.back() == '\n') name.pop_back();
        return "CPU: " + (name.empty() ? "Unknown" : name) + " | Cores: " + std::to_string(cores);
    }
    if (type == "cpu_usage") {
        std::ifstream f("/proc/stat");
        std::string line; std::getline(f, line);
        std::istringstream ss(line);
        std::string cpu; long long u,n,s,i,w=0,hi=0,si2=0,st=0;
        ss >> cpu >> u >> n >> s >> i >> w >> hi >> si2 >> st;
        long long idle1 = i + w;
        long long total1 = u+n+s+i+w+hi+si2+st;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::ifstream f2("/proc/stat");
        std::getline(f2, line);
        std::istringstream ss2(line);
        ss2 >> cpu >> u >> n >> s >> i >> w >> hi >> si2 >> st;
        long long idle2 = i + w;
        long long total2 = u+n+s+i+w+hi+si2+st;
        double pct = total2-total1 > 0 ? (double)(total2-total1-(idle2-idle1))/(double)(total2-total1)*100.0 : 0.0;
        char buf[16]; snprintf(buf,sizeof(buf),"%.1f",pct);
        return std::string("CPU Usage: ") + buf + "%";
    }
    if (type == "ram" || type == "ram_usage") {
        std::ifstream f("/proc/meminfo");
        std::string line;
        long long total=0, free2=0, buffers=0, cached=0, available=0;
        while(std::getline(f,line)) {
            if(line.find("MemTotal")==0)     { std::istringstream s(line); std::string k; s>>k>>total; }
            if(line.find("MemFree")==0)      { std::istringstream s(line); std::string k; s>>k>>free2; }
            if(line.find("Buffers")==0)      { std::istringstream s(line); std::string k; s>>k>>buffers; }
            if(line.find("Cached")==0 && line.find("SwapCached")==std::string::npos) { std::istringstream s(line); std::string k; s>>k>>cached; }
            if(line.find("MemAvailable")==0) { std::istringstream s(line); std::string k; s>>k>>available; }
        }
        long long used = total - available;
        double pct = total > 0 ? (double)used/(double)total*100.0 : 0.0;
        char pbuf[16]; snprintf(pbuf,sizeof(pbuf),"%.1f",pct);
        if (type == "ram_usage") return std::string(pbuf) + "%";
        return "RAM: " + std::to_string(total/1024) + " MB total | "
             + std::to_string(used/1024) + " MB used | "
             + std::to_string(available/1024) + " MB free | "
             + pbuf + "% load";
    }
    if (type == "gpu") {
        std::ifstream f("/proc/driver/nvidia/gpus");
        if (f.good()) return "GPU: NVIDIA (check nvidia-smi for details)";
        std::ifstream f2("/sys/class/drm/card0/device/vendor");
        std::string vendor; std::getline(f2, vendor);
        if (!vendor.empty()) return "GPU: vendor " + vendor;
        return "GPU: unknown (no /proc/driver/nvidia or DRM entry found)";
    }
    if (type == "vram") {
        std::ifstream f("/sys/class/drm/card0/device/mem_info_vram_total");
        if (f.good()) {
            long long vram=0; f>>vram;
            return "VRAM: " + std::to_string(vram/(1024*1024)) + " MB";
        }
        return "VRAM: unavailable (check /sys/class/drm)";
    }
    if (type == "storage") {
        struct statvfs sv;
        if (statvfs("/", &sv) == 0) {
            long long total = (long long)sv.f_blocks * sv.f_frsize / (1024*1024*1024);
            long long free2 = (long long)sv.f_bfree  * sv.f_frsize / (1024*1024*1024);
            long long used  = total - free2;
            double pct = total > 0 ? (double)used/(double)total*100.0 : 0.0;
            char pbuf[16]; snprintf(pbuf,sizeof(pbuf),"%.1f",pct);
            return "Storage /: " + std::to_string(total) + " GB total | "
                 + std::to_string(used) + " GB used | "
                 + std::to_string(free2) + " GB free | "
                 + pbuf + "% used";
        }
        return "Storage: unavailable";
    }
    if (type == "global") {
        std::ifstream fi("/proc/cpuinfo");
        std::string line, cpuname;
        int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
        while(std::getline(fi,line)) {
            if(line.find("model name")!=std::string::npos) {
                size_t p=line.find(':'); if(p!=std::string::npos){cpuname=line.substr(p+2);break;}
            }
        }
        while(!cpuname.empty()&&cpuname.back()=='\n') cpuname.pop_back();
        std::ifstream fm("/proc/meminfo");
        long long total=0,avail=0;
        while(std::getline(fm,line)){
            if(line.find("MemTotal")==0){std::istringstream s(line);std::string k;s>>k>>total;}
            if(line.find("MemAvailable")==0){std::istringstream s(line);std::string k;s>>k>>avail;}
        }
        long long used=total-avail;
        double pct=total>0?(double)used/(double)total*100.0:0.0;
        char pbuf[16]; snprintf(pbuf,sizeof(pbuf),"%.1f",pct);
        struct statvfs sv;
        std::string stor="unknown";
        if(statvfs("/",&sv)==0){
            long long st=(long long)sv.f_blocks*sv.f_frsize/(1024*1024*1024);
            long long sf=(long long)sv.f_bfree*sv.f_frsize/(1024*1024*1024);
            stor=std::to_string(st)+" GB ("+std::to_string(sf)+" GB free)";
        }
        return "CPU: "+(cpuname.empty()?"Unknown":cpuname)+" ("+std::to_string(cores)+" cores)"
             +" | RAM: "+std::to_string(used/1024)+"/"+std::to_string(total/1024)+" MB ("+pbuf+"%)"
             +" | Storage: "+stor;
    }
    struct utsname buf; uname(&buf);
    return std::string("System: ")+buf.sysname+" "+buf.release;
#endif
}

void Interpreter::execAppOpen(ASTNodePtr node) {
#ifdef _WIN32
    HINSTANCE r = ShellExecuteA(nullptr,"open",node->sval.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
    if ((INT_PTR)r<=32) throw TeleosError(ErrorCode::APP_LAUNCH_ERROR,"Failed to launch: "+node->sval,node->line);
#else
    std::string cmd = "xdg-open \""+node->sval+"\" &";
    if (system(cmd.c_str())!=0) throw TeleosError(ErrorCode::APP_LAUNCH_ERROR,"Failed to launch: "+node->sval,node->line);
#endif
}

void Interpreter::execCallLog(ASTNodePtr node) {
    std::string path = node->tags.empty() ? "log.txt" : node->tags[0];
    std::ofstream ofs(path, std::ios::app);
    if (!ofs.is_open()) throw TeleosError(ErrorCode::FILE_IO_ERROR,"Cannot open log file: "+path,node->line);
    time_t now = time(nullptr);
    char tbuf[64]; strftime(tbuf,sizeof(tbuf),"%Y-%m-%d %H:%M:%S",localtime(&now));
    ofs << "[" << tbuf << "] " << node->sval << "\n";
}

void Interpreter::execFetch(ASTNodePtr node) {
    std::string url, varname;
    for (auto& child: node->children) {
        if (child->type==NodeType::VarDecl||child->type==NodeType::ConstDecl) { varname=child->sval; currentEnv->define(varname,strValue("pending")); }
    }
    for (auto& child: node->children) {
        if (child->type==NodeType::VarAssign) { url=eval(child->children[0]).toString(); }
        if (child->type==NodeType::Identifier && child->sval.find("call.fetch")!=std::string::npos) { }
    }
    if (!url.empty()) {
        try {
            std::string result = (node->type==NodeType::FetchPost) ? httpPost(url,"") : httpGet(url);
            if (!varname.empty()) currentEnv->define(varname, strValue(result));
        } catch (...) { if (!varname.empty()) currentEnv->define(varname, strValue("error")); }
    }
    for (auto& child: node->children) { if (child->type==NodeType::ConsoleOutput) execConsoleOutput(child); }
}

std::string Interpreter::httpGet(const std::string& url) {
#ifdef _WIN32
    std::wstring wurl(url.begin(),url.end());
    URL_COMPONENTS uc={}; uc.dwStructSize=sizeof(uc);
    wchar_t host[256]={},path[2048]={};
    uc.lpszHostName=host; uc.dwHostNameLength=256;
    uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
    if (!WinHttpCrackUrl(wurl.c_str(),0,0,&uc)) throw TeleosError(ErrorCode::FETCH_ERROR,"Invalid URL: "+url);
    HINTERNET hSess=WinHttpOpen(L"Teleos/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0);
    if (!hSess) throw TeleosError(ErrorCode::FETCH_ERROR,"WinHTTP session failed");
    HINTERNET hConn=WinHttpConnect(hSess,host,uc.nPort,0);
    if (!hConn){WinHttpCloseHandle(hSess);throw TeleosError(ErrorCode::FETCH_ERROR,"Connection failed");}
    DWORD flags=(uc.nScheme==INTERNET_SCHEME_HTTPS)?WINHTTP_FLAG_SECURE:0;
    HINTERNET hReq=WinHttpOpenRequest(hConn,L"GET",path,nullptr,nullptr,nullptr,flags);
    if (!hReq){WinHttpCloseHandle(hConn);WinHttpCloseHandle(hSess);throw TeleosError(ErrorCode::FETCH_ERROR,"Request failed");}
    if (!WinHttpSendRequest(hReq,nullptr,0,nullptr,0,0,0)||!WinHttpReceiveResponse(hReq,nullptr)){WinHttpCloseHandle(hReq);WinHttpCloseHandle(hConn);WinHttpCloseHandle(hSess);throw TeleosError(ErrorCode::FETCH_ERROR,"Send failed");}
    std::string resp; DWORD avail=0;
    while(WinHttpQueryDataAvailable(hReq,&avail)&&avail>0){std::vector<char>buf(avail+1,0);DWORD read=0;WinHttpReadData(hReq,buf.data(),avail,&read);resp.append(buf.data(),read);}
    WinHttpCloseHandle(hReq);WinHttpCloseHandle(hConn);WinHttpCloseHandle(hSess);
    return resp;
#else
    return "[HTTP fetch: WinHTTP only available on Windows. Use os.run with curl on Linux.]";
#endif
}

std::string Interpreter::httpPost(const std::string& url, const std::string& body) {
    return httpGet(url);
}

void Interpreter::execImport(ASTNodePtr node) {
    std::string filename = node->sval;
    // Accept .tsl, .tslc, or bare name (try .tslc first, then .tsl)
    bool hasTsl  = filename.size() >= 4 && filename.substr(filename.size()-4) == ".tsl";
    bool hasTslc = filename.size() >= 5 && filename.substr(filename.size()-5) == ".tslc";
    if (!hasTsl && !hasTslc) {
        std::ifstream tryc(filename + ".tslc");
        if (tryc.good()) { filename += ".tslc"; }
        else { filename += ".tsl"; }
    }
    loadModule(filename);
}

void Interpreter::loadModule(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) throw TeleosError(ErrorCode::IMPORT_ERROR,"Cannot open module: "+filename);
    std::stringstream ss; ss << ifs.rdbuf();
    std::string source = ss.str();
    // Deobfuscate if compiled
    if (isCompiled(source)) source = deobfuscateSource(source);
    auto tokens = tokenize(source);
    Parser p(tokens); auto ast = p.parse();
    auto modEnv = std::make_shared<Environment>(globalEnv);
    auto prev = currentEnv; currentEnv = modEnv;
    run(ast);
    currentEnv = prev;
    for (auto& [k,v]: modEnv->vars) globalEnv->define(k,v);
    for (auto& [k,v]: exportedVars) globalEnv->define(k,v);
}

void Interpreter::execRepeat(ASTNodePtr node) {
    double counter = 0.0;
    bool infinite = (node->children[0]->sval == "nil");
    double limit = infinite ? 0.0 : node->children[0]->nval;
    ASTNodePtr body = node->children[1];
    currentEnv->define(node->sval, numValue(0));
    repeatFinish = false;
    while (true) {
        if (!infinite && counter >= limit) break;
        try { execBlock(body); } catch (ReturnSignal&) { break; }
        if (repeatFinish) { repeatFinish = false; break; }
        counter++;
        currentEnv->set(node->sval, numValue(counter));
    }
}

void Interpreter::execIf(ASTNodePtr node) { if (evalCondition(node->children[0])) execBlock(node->children[1]); }
void Interpreter::execUnless(ASTNodePtr node) { if (!evalCondition(node->children[0])) execBlock(node->children[1]); }
void Interpreter::execWhy(ASTNodePtr node) { if (evalCondition(node->children[0])) execBlock(node->children[1]); }

void Interpreter::execCompare(ASTNodePtr node) {
    Value v1 = nilValue(), v2 = nilValue();
    try { v1 = currentEnv->get(node->sval); } catch (...) {}
    try { if (!node->tags.empty()) v2 = currentEnv->get(node->tags[0]); } catch (...) {}
    currentEnv->define("function.compare.current", v1);
    currentEnv->define("__compare_v1__", v1);
    currentEnv->define("__compare_v2__", v2);
    execBlock(node->children[0]);
}

void Interpreter::execFuncDef(ASTNodePtr node) {
    FuncDef fd; fd.params = node->tags; fd.body = node->children[0];
    userFuncs[node->sval] = fd;
}

Value Interpreter::execFuncCall(ASTNodePtr node) {
    auto it = userFuncs.find(node->sval);
    if (it == userFuncs.end()) throw TeleosError(ErrorCode::UNDEFINED_VARIABLE,"Function '"+node->sval+"' not defined",node->line);
    auto inner = std::make_shared<Environment>(globalEnv);
    auto& fd = it->second;
    for (size_t i = 0; i < fd.params.size(); i++) {
        Value v = i < node->children.size() ? eval(node->children[i]) : nilValue();
        inner->define(fd.params[i], v);
    }
    auto prev = currentEnv; currentEnv = inner;
    Value ret = nilValue();
    try { execBlock(fd.body); }
    catch (ReturnSignal& r) { ret = r.val; }
    currentEnv = prev;
    return ret;
}

void Interpreter::execTryCatch(ASTNodePtr node) {
    try {
        auto inner = std::make_shared<Environment>(currentEnv);
        auto prev = currentEnv; currentEnv = inner;
        execBlock(node->children[0]);
        currentEnv = prev;
    } catch (TeleosThrow& t) {
        if (node->children.size() >= 2) {
            auto inner = std::make_shared<Environment>(currentEnv);
            if (!node->sval.empty()) inner->define(node->sval, t.val);
            auto prev = currentEnv; currentEnv = inner;
            execBlock(node->children[1]);
            currentEnv = prev;
        }
    } catch (TeleosError& e) {
        if (node->children.size() >= 2) {
            auto inner = std::make_shared<Environment>(currentEnv);
            if (!node->sval.empty()) inner->define(node->sval, strValue(e.message));
            auto prev = currentEnv; currentEnv = inner;
            execBlock(node->children[1]);
            currentEnv = prev;
        }
    }
    if (node->children.size() >= 3) {
        auto inner = std::make_shared<Environment>(currentEnv);
        auto prev = currentEnv; currentEnv = inner;
        execBlock(node->children[2]);
        currentEnv = prev;
    }
}

void Interpreter::execSwitch(ASTNodePtr node) {
    Value subject = eval(node->children[0]);
    bool matched = false;
    for (size_t i = 1; i < node->children.size(); i++) {
        auto& c = node->children[i];
        if (c->sval == "__default__") {
            if (!matched) { execBlock(c->children[0]); matched = true; }
        } else {
            Value cval = eval(c->children[0]);
            if (subject == cval) { execBlock(c->children[1]); matched = true; break; }
        }
    }
}

Value Interpreter::execStringFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="string.len") return numValue(arg(0).toString().size());
    if (fn=="string.upper") { std::string s=arg(0).toString(); std::transform(s.begin(),s.end(),s.begin(),::toupper); return strValue(s); }
    if (fn=="string.lower") { std::string s=arg(0).toString(); std::transform(s.begin(),s.end(),s.begin(),::tolower); return strValue(s); }
    if (fn=="string.trim") {
        std::string s=arg(0).toString();
        size_t a=s.find_first_not_of(" \t\n\r");
        size_t b=s.find_last_not_of(" \t\n\r");
        return strValue(a==std::string::npos?"":s.substr(a,b-a+1));
    }
    if (fn=="string.split") {
        std::string s=arg(0).toString(), delim=arg(1).toString();
        std::vector<Value> parts;
        size_t pos=0, found;
        if (delim.empty()) { for (char c: s) parts.push_back(strValue(std::string(1,c))); }
        else { while ((found=s.find(delim,pos))!=std::string::npos){parts.push_back(strValue(s.substr(pos,found-pos)));pos=found+delim.size();}parts.push_back(strValue(s.substr(pos))); }
        return Value(parts);
    }
    if (fn=="string.join") {
        Value tbl=arg(0); std::string delim=arg(1).toString(); std::string result;
        for (size_t i=0;i<tbl.table.size();i++){if(i)result+=delim;result+=tbl.table[i].toString();}
        return strValue(result);
    }
    if (fn=="string.contains") { std::string s=arg(0).toString(),sub=arg(1).toString(); return boolValue(s.find(sub)!=std::string::npos); }
    if (fn=="string.replace") {
        std::string s=arg(0).toString(),from=arg(1).toString(),to=arg(2).toString();
        size_t pos=0;
        while((pos=s.find(from,pos))!=std::string::npos){s.replace(pos,from.size(),to);pos+=to.size();}
        return strValue(s);
    }
    if (fn=="string.starts") { std::string s=arg(0).toString(),pre=arg(1).toString(); return boolValue(s.rfind(pre,0)==0); }
    if (fn=="string.ends")   { std::string s=arg(0).toString(),suf=arg(1).toString(); return boolValue(s.size()>=suf.size()&&s.substr(s.size()-suf.size())==suf); }
    if (fn=="string.sub")    { std::string s=arg(0).toString(); int st=(int)arg(1).nval,len_=(int)arg(2).nval; if(st<0)st=0; if(st>=(int)s.size())return strValue(""); return strValue(s.substr(st,len_)); }
    if (fn=="string.find")   { std::string s=arg(0).toString(),sub=arg(1).toString(); size_t f=s.find(sub); return f==std::string::npos?numValue(-1):numValue(f); }
    if (fn=="string.repeat") { std::string s=arg(0).toString(); int n=(int)arg(1).nval; std::string r; for(int i=0;i<n;i++)r+=s; return strValue(r); }
    if (fn=="string.reverse"){ std::string s=arg(0).toString(); std::reverse(s.begin(),s.end()); return strValue(s); }
    if (fn=="string.pad_left")  { std::string s=arg(0).toString(); int w=(int)arg(1).nval; char p=arg(2).type==ValueType::String&&!arg(2).sval.empty()?arg(2).sval[0]:' '; while((int)s.size()<w) s=p+s; return strValue(s); }
    if (fn=="string.pad_right") { std::string s=arg(0).toString(); int w=(int)arg(1).nval; char p=arg(2).type==ValueType::String&&!arg(2).sval.empty()?arg(2).sval[0]:' '; while((int)s.size()<w) s+=p; return strValue(s); }
    if (fn=="string.count") { std::string s=arg(0).toString(),sub=arg(1).toString(); int cnt=0; size_t pos=0; while((pos=s.find(sub,pos))!=std::string::npos){cnt++;pos+=sub.size();} return numValue(cnt); }
    if (fn=="string.wrap") { std::string s=arg(0).toString(); int w=(int)arg(1).nval; std::string r; for(size_t i=0;i<s.size();i+=w){if(i)r+="\n";r+=s.substr(i,w);} return strValue(r); }
    if (fn=="string.format") {
        std::string fmt=arg(0).toString(); int ai=1;
        std::string result; size_t i=0;
        while(i<fmt.size()){if(fmt[i]=='{'&&i+1<fmt.size()&&fmt[i+1]=='}'){result+=arg(ai++).toString();i+=2;}else result+=fmt[i++];}
        return strValue(result);
    }
    return nilValue();
}

Value Interpreter::execTableFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="table.push") {
        Value tbl=arg(0); std::string varname=node->children[0]->sval;
        tbl.table.push_back(arg(1));
        currentEnv->set(varname, tbl); return tbl;
    }
    if (fn=="table.pop") {
        Value tbl=arg(0); std::string varname=node->children[0]->sval;
        if (tbl.table.empty()) throw TeleosError(ErrorCode::INDEX_OUT_OF_RANGE,"Cannot pop from empty table",node->line);
        Value last=tbl.table.back(); tbl.table.pop_back();
        currentEnv->set(varname, tbl); return last;
    }
    if (fn=="table.len")      return numValue(arg(0).table.size());
    if (fn=="table.get")      { Value t=arg(0); int i=(int)arg(1).nval; if(i<0||i>=(int)t.table.size())throw TeleosError(ErrorCode::INDEX_OUT_OF_RANGE,"Index out of range",node->line); return t.table[i]; }
    if (fn=="table.set")      { Value t=arg(0); int i=(int)arg(1).nval; Value v=arg(2); std::string varname=node->children[0]->sval; if(i>=0&&i<(int)t.table.size())t.table[i]=v; currentEnv->set(varname,t); return t; }
    if (fn=="table.remove")   { Value t=arg(0); int i=(int)arg(1).nval; std::string varname=node->children[0]->sval; if(i>=0&&i<(int)t.table.size())t.table.erase(t.table.begin()+i); currentEnv->set(varname,t); return t; }
    if (fn=="table.contains") { Value t=arg(0),v=arg(1); for(auto&e:t.table)if(e==v)return boolValue(true); return boolValue(false); }
    if (fn=="table.keys")     { Value t=arg(0); std::vector<Value> keys; for(size_t i=0;i<t.table.size();i++) keys.push_back(numValue(i)); return Value(keys); }
    if (fn=="table.merge")    { Value a=arg(0),b=arg(1); for(auto&e:b.table)a.table.push_back(e); return a; }
    if (fn=="table.sort")     { Value t=arg(0); std::sort(t.table.begin(),t.table.end(),[](const Value&a,const Value&b){return a<b;}); return t; }
    if (fn=="table.slice")    { Value t=arg(0); int st=(int)arg(1).nval,en=(int)arg(2).nval; if(st<0)st=0; if(en>(int)t.table.size())en=t.table.size(); std::vector<Value> sl(t.table.begin()+st,t.table.begin()+en); return Value(sl); }
    if (fn=="table.each") {
        Value t=arg(0); std::string varname=node->children[1]->sval; std::string bodyVar=node->children[0]->sval;
        for (auto& e: t.table) { currentEnv->define(varname,e); }
        return t;
    }
    return nilValue();
}

Value Interpreter::execFileFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="file.read")   { std::ifstream f(arg(0).toString()); if(!f.is_open())throw TeleosError(ErrorCode::FILE_IO_ERROR,"Cannot read file: "+arg(0).toString(),node->line); std::stringstream ss; ss<<f.rdbuf(); return strValue(ss.str()); }
    if (fn=="file.write")  { std::ofstream f(arg(0).toString()); if(!f.is_open())throw TeleosError(ErrorCode::FILE_IO_ERROR,"Cannot write file",node->line); f<<arg(1).toString(); return boolValue(true); }
    if (fn=="file.append") { std::ofstream f(arg(0).toString(),std::ios::app); if(!f.is_open())throw TeleosError(ErrorCode::FILE_IO_ERROR,"Cannot append file",node->line); f<<arg(1).toString(); return boolValue(true); }
    if (fn=="file.exists") { std::ifstream f(arg(0).toString()); return boolValue(f.good()); }
    if (fn=="file.delete") { return boolValue(std::remove(arg(0).toString().c_str())==0); }
    if (fn=="file.lines")  {
        std::ifstream f(arg(0).toString()); if(!f.is_open())throw TeleosError(ErrorCode::FILE_IO_ERROR,"Cannot read file",node->line);
        std::vector<Value> lines; std::string line;
        while(std::getline(f,line)) lines.push_back(strValue(line));
        return Value(lines);
    }
    if (fn=="file.copy")   { std::ifstream src(arg(0).toString(),std::ios::binary); std::ofstream dst(arg(1).toString(),std::ios::binary); if(!src.is_open()||!dst.is_open())return boolValue(false); dst<<src.rdbuf(); return boolValue(true); }
    if (fn=="file.move")   { if(std::rename(arg(0).toString().c_str(),arg(1).toString().c_str())==0)return boolValue(true); return boolValue(false); }
    if (fn=="file.size")   { std::ifstream f(arg(0).toString(),std::ios::ate|std::ios::binary); if(!f.is_open())return numValue(-1); return numValue((double)f.tellg()); }
    if (fn=="file.mkdir")  {
#ifdef _WIN32
        return boolValue(CreateDirectoryA(arg(0).toString().c_str(),nullptr)!=0);
#else
        return boolValue(mkdir(arg(0).toString().c_str(),0755)==0);
#endif
    }
    return nilValue();
}

Value Interpreter::execOsFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="os.time")     { return numValue((double)std::time(nullptr)); }
    if (fn=="os.date")     { time_t now=std::time(nullptr); char buf[64]; strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&now)); return strValue(buf); }
    if (fn=="os.sleep")    { std::this_thread::sleep_for(std::chrono::milliseconds((int)arg(0).nval)); return nilValue(); }
    if (fn=="os.run")      { int r=system(arg(0).toString().c_str()); return numValue(r); }
    if (fn=="os.exit")     { int code=node->children.empty()?0:(int)arg(0).nval; exit(code); }
    if (fn=="os.env")      { const char* v=std::getenv(arg(0).toString().c_str()); return v?strValue(v):nilValue(); }
    if (fn=="os.cwd")      { char buf[4096]; return strValue(
#ifdef _WIN32
        GetCurrentDirectoryA(sizeof(buf),buf)?buf:"."
#else
        getcwd(buf,sizeof(buf))?buf:"."
#endif
    ); }
    if (fn=="os.platform") {
#ifdef _WIN32
        return strValue("windows");
#elif __APPLE__
        return strValue("macos");
#else
        return strValue("linux");
#endif
    }
    if (fn=="os.args")     { std::vector<Value> a; for(auto&s:programArgs)a.push_back(strValue(s)); return Value(a); }
    return nilValue();
}

Value Interpreter::execTypeFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="type.of") {
        Value v=arg(0);
        switch(v.type){
            case ValueType::Nil:     return strValue("nil");
            case ValueType::String:  return strValue("string");
            case ValueType::Number:  return strValue("number");
            case ValueType::Boolean: return strValue("boolean");
            case ValueType::Table:   return strValue("table");
        }
    }
    if (fn=="type.cast") {
        Value v=arg(0); std::string t=arg(1).toString();
        if(t=="string") return strValue(v.toString());
        if(t=="number") { try{return numValue(std::stod(v.toString()));}catch(...){return numValue(0);} }
        if(t=="boolean") return boolValue(v.isTruthy());
        return v;
    }
    if (fn=="type.is") {
        Value v=arg(0); std::string t=arg(1).toString();
        switch(v.type){
            case ValueType::Nil:     return boolValue(t=="nil");
            case ValueType::String:  return boolValue(t=="string");
            case ValueType::Number:  return boolValue(t=="number");
            case ValueType::Boolean: return boolValue(t=="boolean");
            case ValueType::Table:   return boolValue(t=="table");
        }
    }
    return nilValue();
}

Value Interpreter::execRangeCall(ASTNodePtr node) {
    double start = eval(node->children[0]).nval;
    double end   = eval(node->children[1]).nval;
    std::vector<Value> result;
    for (double i = start; i <= end; i++) result.push_back(numValue(i));
    Value tbl(result);
    if (!node->sval.empty()) currentEnv->define(node->sval, tbl);
    return tbl;
}

void Interpreter::execEnumDef(ASTNodePtr node) {
    for (size_t i = 0; i < node->tags.size(); i++) {
        enumVals[node->tags[i]] = numValue((double)i);
        globalEnv->define(node->tags[i], numValue((double)i));
    }
}

#include <regex>
#include <numeric>
#include <iomanip>

#ifdef _WIN32
#include <tlhelp32.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#endif

static std::string simpleHash(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    std::ostringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << h;
    return ss.str();
}

static std::string toHex(const std::string& s) {
    std::ostringstream ss;
    for (unsigned char c : s) ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    return ss.str();
}

Value Interpreter::execCryptoFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="crypto.hash")   return strValue(simpleHash(arg(0).toString()));
    if (fn=="crypto.b64enc") return strValue(base64Encode(arg(0).toString()));
    if (fn=="crypto.b64dec") return strValue(base64Decode(arg(0).toString()));
    if (fn=="crypto.hex")    return strValue(toHex(arg(0).toString()));
    return nilValue();
}

Value Interpreter::execJsonFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="json.stringify") {
        Value v = arg(0);
        std::function<std::string(const Value&)> toJson = [&](const Value& val) -> std::string {
            if (val.type == ValueType::Nil)     return "null";
            if (val.type == ValueType::Boolean) return val.bval ? "true" : "false";
            if (val.type == ValueType::Number) {
                if (val.nval == std::floor(val.nval)) return std::to_string((long long)val.nval);
                std::ostringstream ss; ss << val.nval; return ss.str();
            }
            if (val.type == ValueType::String) {
                std::string s = "\"";
                for (char c : val.sval) {
                    if (c=='"') s+="\\\""; else if(c=='\\') s+="\\\\";
                    else if(c=='\n') s+="\\n"; else if(c=='\t') s+="\\t";
                    else s+=c;
                }
                return s + "\"";
            }
            if (val.type == ValueType::Table) {
                std::string r = "[";
                for (size_t i = 0; i < val.table.size(); i++) {
                    if (i) r += ",";
                    r += toJson(val.table[i]);
                }
                return r + "]";
            }
            return "null";
        };
        return strValue(toJson(v));
    }
    if (fn=="json.parse") {
        std::string s = arg(0).toString();
        s.erase(std::remove_if(s.begin(),s.end(),[](char c){return c=='\n'||c=='\r';}),s.end());
        if (s.empty()) return nilValue();
        if (s=="null") return nilValue();
        if (s=="true") return boolValue(true);
        if (s=="false") return boolValue(false);
        if (s[0]=='"'&&s.back()=='"') return strValue(s.substr(1,s.size()-2));
        if (!s.empty() && (std::isdigit(s[0])||s[0]=='-')) {
            try { return numValue(std::stod(s)); } catch(...) {}
        }
        if (s[0]=='[') {
            std::vector<Value> tbl;
            std::string inner = s.substr(1, s.size()-2);
            size_t i=0; int depth=0; std::string cur;
            while (i<=inner.size()) {
                char c = i<inner.size()?inner[i]:',';
                if(c=='['||c=='{') depth++;
                if(c==']'||c=='}') depth--;
                if((c==','&&depth==0)||i==inner.size()) {
                    if (!cur.empty()) {
                        Value sub = execJsonFunc([&](){
                            auto n=makeNode(NodeType::JsonFunc,0); n->sval="json.parse";
                            auto lit=makeNode(NodeType::Literal,0); lit->sval=cur; n->children.push_back(lit);
                            return n;
                        }());
                        tbl.push_back(sub);
                    }
                    cur=""; i++; continue;
                }
                cur+=c; i++;
            }
            return Value(tbl);
        }
        return strValue(s);
    }
    return nilValue();
}

Value Interpreter::execClipFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
#ifdef _WIN32
    if (fn=="clipboard.write") {
        std::string text = arg(0).toString();
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size()+1);
            if (hg) {
                memcpy(GlobalLock(hg), text.c_str(), text.size()+1);
                GlobalUnlock(hg);
                SetClipboardData(CF_TEXT, hg);
            }
            CloseClipboard();
            return boolValue(true);
        }
        return boolValue(false);
    }
    if (fn=="clipboard.read") {
        if (OpenClipboard(nullptr)) {
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (hData) {
                char* pszText = static_cast<char*>(GlobalLock(hData));
                std::string text = pszText ? pszText : "";
                GlobalUnlock(hData);
                CloseClipboard();
                return strValue(text);
            }
            CloseClipboard();
        }
        return strValue("");
    }
#else
    if (fn=="clipboard.write") {
        std::string cmd = "echo '" + arg(0).toString() + "' | xclip -selection clipboard 2>/dev/null || echo '" + arg(0).toString() + "' | xsel --clipboard --input 2>/dev/null";
        system(cmd.c_str());
        return boolValue(true);
    }
    if (fn=="clipboard.read") {
        FILE* pipe = popen("xclip -selection clipboard -o 2>/dev/null || xsel --clipboard --output 2>/dev/null","r");
        if (!pipe) return strValue("");
        char buf[4096]={};
        std::string result;
        while(fgets(buf,sizeof(buf),pipe)) result+=buf;
        pclose(pipe);
        return strValue(result);
    }
#endif
    return nilValue();
}

Value Interpreter::execNetFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
#ifdef _WIN32
    if (fn=="net.ping") {
        std::string host = arg(0).toString();
        std::string cmd = "ping -n 1 -w 1000 " + host + " >nul 2>&1";
        int r = system(cmd.c_str());
        return boolValue(r==0);
    }
    if (fn=="net.port") {
        std::string host = arg(0).toString();
        int port = (int)arg(1).nval;
        WSADATA wd; WSAStartup(MAKEWORD(2,2),&wd);
        SOCKET s = socket(AF_INET,SOCK_STREAM,0);
        sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
        inet_pton(AF_INET,host.c_str(),&addr.sin_addr);
        bool open = connect(s,(sockaddr*)&addr,sizeof(addr))==0;
        closesocket(s); WSACleanup();
        return boolValue(open);
    }
    if (fn=="net.host") {
        WSADATA wd; WSAStartup(MAKEWORD(2,2),&wd);
        char buf[256]={}; gethostname(buf,sizeof(buf));
        WSACleanup();
        return strValue(std::string(buf));
    }
#else
    if (fn=="net.ping") {
        std::string host=arg(0).toString();
        int r=system(("ping -c 1 -W 1 "+host+" >/dev/null 2>&1").c_str());
        return boolValue(r==0);
    }
    if (fn=="net.port") {
        return strValue("[net.port: Linux build — use os.run with netcat]");
    }
    if (fn=="net.host") {
        char buf[256]={}; gethostname(buf,sizeof(buf)); return strValue(buf);
    }
#endif
    return nilValue();
}

Value Interpreter::execRegexFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    try {
        std::string input = arg(0).toString();
        std::string pattern = arg(1).toString();
        std::regex rx(pattern);
        if (fn=="regex.test")    return boolValue(std::regex_search(input, rx));
        if (fn=="regex.find") {
            std::smatch m;
            if (std::regex_search(input, m, rx)) return strValue(m[0].str());
            return nilValue();
        }
        if (fn=="regex.match") {
            std::vector<Value> matches;
            std::sregex_iterator it(input.begin(), input.end(), rx), end;
            for (; it != end; ++it) matches.push_back(strValue((*it)[0].str()));
            return Value(matches);
        }
        if (fn=="regex.replace") {
            std::string repl = arg(2).toString();
            return strValue(std::regex_replace(input, rx, repl));
        }
    } catch (std::regex_error& e) {
        throw TeleosError(ErrorCode::INVALID_ARGUMENT, "Regex error: " + std::string(e.what()), node->line);
    }
    return nilValue();
}

Value Interpreter::execTimeFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    if (fn=="time.now")   { return numValue((double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); }
    if (fn=="time.stamp") { return numValue((double)std::time(nullptr)); }
    if (fn=="time.format") {
        std::time_t t = node->children.empty() ? std::time(nullptr) : (std::time_t)arg(0).nval;
        std::string fmt = node->children.size()>1 ? arg(1).toString() : "%Y-%m-%d %H:%M:%S";
        char buf[128]={};
        std::strftime(buf, sizeof(buf), fmt.c_str(), std::localtime(&t));
        return strValue(buf);
    }
    if (fn=="time.diff") {
        double a = arg(0).nval, b = arg(1).nval;
        return numValue(std::abs(a - b));
    }
    return nilValue();
}

Value Interpreter::execProcFunc(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
#ifdef _WIN32
    if (fn=="proc.pid") { return numValue((double)GetCurrentProcessId()); }
    if (fn=="proc.list") {
        std::vector<Value> procs;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 pe; pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe)) {
            do { procs.push_back(strValue(std::string(pe.szExeFile))); }
            while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        return Value(procs);
    }
    if (fn=="proc.kill") {
        std::string name = arg(0).toString();
        std::string cmd = "taskkill /F /IM \"" + name + "\" >nul 2>&1";
        return boolValue(system(cmd.c_str()) == 0);
    }
#else
    if (fn=="proc.pid")  { return numValue((double)getpid()); }
    if (fn=="proc.list") {
        FILE* pipe = popen("ps -e -o comm= 2>/dev/null","r");
        std::vector<Value> procs; char buf[256]={};
        while (pipe && fgets(buf,sizeof(buf),pipe)) {
            std::string s=buf;
            while(!s.empty()&&(s.back()=='\n'||s.back()==' '))s.pop_back();
            if (!s.empty()) procs.push_back(strValue(s));
        }
        if (pipe) pclose(pipe);
        return Value(procs);
    }
    if (fn=="proc.kill") {
        std::string name = arg(0).toString();
        return boolValue(system(("pkill -f \""+name+"\" 2>/dev/null").c_str())==0);
    }
#endif
    return nilValue();
}

void Interpreter::execConsoleTable(ASTNodePtr node) {
    if (node->children.empty()) return;
    Value tbl = eval(node->children[0]);
    if (tbl.type != ValueType::Table) { std::cout << tbl.toString() << "\n"; return; }
    size_t colW = 5;
    for (auto& row : tbl.table) {
        if (row.type == ValueType::Table) {
            for (auto& cell : row.table) colW = std::max(colW, cell.toString().size()+2);
        } else {
            colW = std::max(colW, row.toString().size()+2);
        }
    }
    auto pad = [&](const std::string& s, size_t w) {
        std::string r = " " + s;
        while (r.size() < w) r += " ";
        return r;
    };
    std::string sep = "+";
    for (size_t i = 0; i < 2; i++) { for(size_t j=0;j<colW;j++) sep+="-"; sep+="+"; }
    std::cout << "+"; for(size_t j=0;j<colW;j++) std::cout<<"-"; std::cout<<"+"; for(size_t j=0;j<colW;j++) std::cout<<"-"; std::cout<<"+"<<"\n";
    std::cout << "|" << pad("index",colW) << "|" << pad("value",colW) << "|\n";
    std::cout << sep << "\n";
    for (size_t i = 0; i < tbl.table.size(); i++) {
        std::cout << "|" << pad(std::to_string(i),colW) << "|" << pad(tbl.table[i].toString(),colW) << "|\n";
    }
    std::cout << sep << "\n";
}

Value Interpreter::execFetchSimple(ASTNodePtr node) {
    auto arg = [&](int i) { return i < (int)node->children.size() ? eval(node->children[i]) : nilValue(); };
    std::string fn = node->sval;
    std::string url = arg(0).toString();
    if (fn=="fetch.get")  return strValue(httpGet(url));
    if (fn=="fetch.post") {
        std::string body = node->children.size()>1 ? arg(1).toString() : "";
        return strValue(httpPost(url, body));
    }
    return nilValue();
}

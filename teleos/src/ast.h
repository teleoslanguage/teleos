#pragma once
#include <string>
#include <vector>
#include <memory>

enum class NodeType {
    Program, Comment, Paragraph,
    VarDecl, ConstDecl, VarAssign,
    CallVar, CallFunction,
    ConsoleOutput, ConsoleAsk, ConsoleCheck, ConsoleClear,
    ConsoleColor, ConsoleTitle, ConsoleWait, ConsoleInput,
    MathCall, MathFunc,
    SystemHardware, AppOpen, CallLog,
    FetchAPI, FetchPost,
    ImportModule, ExportModule,
    RepeatLoop, RepeatFinish,
    IfStatement, UnlessStatement, WhyStatement,
    CompareBlock, FunctionEndBlock,
    FuncDef, FuncReturn, FuncCallUser,
    TryCatch, ThrowError,
    SwitchBlock, SwitchCase,
    StringFunc, TableFunc, FileFunc, OsFunc,
    TypeFunc, RangeCall,
    BinaryExpr, UnaryExpr, Literal, Identifier,
    TableLiteral, Block,
    EnumDef, PipeExpr,
    AssertStmt, InspectStmt, WhileLoop, BreakStmt, ContinueStmt,
    IncrStmt, DecrStmt, ConsolePrint,
    CryptoFunc, JsonFunc, ClipFunc, NetFunc, RegexFunc, TimeFunc, ProcFunc,
    ConsoleTable, ConsoleBell, FetchSimple,
};

struct ASTNode;
using ASTNodePtr = std::shared_ptr<ASTNode>;

struct ASTNode {
    NodeType type;
    std::string sval;
    double nval = 0.0;
    bool bval = false;
    int line = 0;
    std::vector<ASTNodePtr> children;
    std::vector<std::string> tags;
    ASTNode(NodeType t, int l = 0) : type(t), line(l) {}
};

inline ASTNodePtr makeNode(NodeType t, int line = 0) {
    return std::make_shared<ASTNode>(t, line);
}

#pragma once
#include <string>
#include <stdexcept>

enum class ErrorCode {
    SYNTAX_ERROR,
    UNKNOWN_STATEMENT,
    UNDEFINED_VARIABLE,
    TYPE_MISMATCH,
    DIVISION_BY_ZERO,
    MODULE_NOT_FOUND,
    FILE_IO_ERROR,
    HARDWARE_QUERY_ERROR,
    APP_LAUNCH_ERROR,
    FETCH_ERROR,
    RUNTIME_ERROR,
    OVERFLOW_ERROR,
    IMPORT_ERROR,
    INDEX_OUT_OF_RANGE,
    INVALID_ARGUMENT,
};

inline std::string errorCodeName(ErrorCode code) {
    switch (code) {
        case ErrorCode::SYNTAX_ERROR: return "SyntaxError";
        case ErrorCode::UNKNOWN_STATEMENT: return "UnknownStatement";
        case ErrorCode::UNDEFINED_VARIABLE: return "UndefinedVariable";
        case ErrorCode::TYPE_MISMATCH: return "TypeMismatch";
        case ErrorCode::DIVISION_BY_ZERO: return "DivisionByZero";
        case ErrorCode::MODULE_NOT_FOUND: return "ModuleNotFound";
        case ErrorCode::FILE_IO_ERROR: return "FileIOError";
        case ErrorCode::HARDWARE_QUERY_ERROR: return "HardwareQueryError";
        case ErrorCode::APP_LAUNCH_ERROR: return "AppLaunchError";
        case ErrorCode::FETCH_ERROR: return "FetchError";
        case ErrorCode::RUNTIME_ERROR: return "RuntimeError";
        case ErrorCode::OVERFLOW_ERROR: return "OverflowError";
        case ErrorCode::IMPORT_ERROR: return "ImportError";
        case ErrorCode::INDEX_OUT_OF_RANGE: return "IndexOutOfRange";
        case ErrorCode::INVALID_ARGUMENT: return "InvalidArgument";
        default: return "UnknownError";
    }
}

class TeleosError : public std::exception {
public:
    ErrorCode code;
    std::string message;
    int line;

    TeleosError(ErrorCode c, std::string msg, int l = 0)
        : code(c), message(std::move(msg)), line(l) {}

    const char* what() const noexcept override {
        return message.c_str();
    }

    std::string format() const {
        std::string out;
        out += "\n[Teleos " + errorCodeName(code) + "]";
        if (line > 0) out += " at line " + std::to_string(line);
        out += ": " + message + "\n";
        return out;
    }
};

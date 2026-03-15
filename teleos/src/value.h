#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <sstream>
#include <cmath>

enum class ValueType {
    Nil,
    String,
    Number,
    Boolean,
    Table,
};

struct Value {
    ValueType type = ValueType::Nil;
    std::string sval;
    double nval = 0.0;
    bool bval = false;
    std::vector<Value> table;

    Value() : type(ValueType::Nil) {}
    explicit Value(const std::string& s) : type(ValueType::String), sval(s) {}
    explicit Value(double n) : type(ValueType::Number), nval(n) {}
    explicit Value(bool b) : type(ValueType::Boolean), bval(b) {}
    explicit Value(std::vector<Value> t) : type(ValueType::Table), table(std::move(t)) {}

    bool isTruthy() const {
        if (type == ValueType::Nil) return false;
        if (type == ValueType::Boolean) return bval;
        if (type == ValueType::Number) return nval != 0.0;
        if (type == ValueType::String) return !sval.empty();
        return true;
    }

    std::string toString() const {
        switch (type) {
            case ValueType::Nil: return "nil";
            case ValueType::String: return sval;
            case ValueType::Boolean: return bval ? "true" : "false";
            case ValueType::Number: {
                if (nval == std::floor(nval)) {
                    return std::to_string((long long)nval);
                }
                std::ostringstream ss;
                ss << nval;
                return ss.str();
            }
            case ValueType::Table: {
                std::string r = "{";
                for (size_t i = 0; i < table.size(); i++) {
                    if (i) r += ", ";
                    r += "[" + table[i].toString() + "]";
                }
                return r + "}";
            }
        }
        return "nil";
    }

    bool operator==(const Value& o) const {
        if (type != o.type) return false;
        switch (type) {
            case ValueType::Nil: return true;
            case ValueType::String: return sval == o.sval;
            case ValueType::Number: return nval == o.nval;
            case ValueType::Boolean: return bval == o.bval;
            default: return false;
        }
    }
    bool operator!=(const Value& o) const { return !(*this == o); }
    bool operator<(const Value& o) const {
        if (type == ValueType::Number && o.type == ValueType::Number) return nval < o.nval;
        return toString() < o.toString();
    }
    bool operator>(const Value& o) const { return o < *this; }
    bool operator<=(const Value& o) const { return !(o < *this); }
    bool operator>=(const Value& o) const { return !(*this < o); }
};

inline Value nilValue() { return Value(); }
inline Value numValue(double n) { return Value(n); }
inline Value strValue(const std::string& s) { return Value(s); }
inline Value boolValue(bool b) { return Value(b); }

#pragma once
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

static std::string base64Encode(const std::string& in) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val=0, valb=-6;
    for (unsigned char c : in) {
        val=(val<<8)+c; valb+=8;
        while(valb>=0){out.push_back(b64[(val>>valb)&0x3F]);valb-=6;}
    }
    if(valb>-6) out.push_back(b64[((val<<8)>>(valb+8))&0x3F]);
    while(out.size()%4) out.push_back('=');
    return out;
}

static std::string base64Decode(const std::string& in) {
    static const int lookup[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    std::string out; int val=0, valb=-8;
    for (unsigned char c : in) {
        if(lookup[c]==-1) break;
        val=(val<<6)+lookup[c]; valb+=6;
        if(valb>=0){out.push_back(char((val>>valb)&0xFF));valb-=8;}
    }
    return out;
}

static std::string xorObfuscate(const std::string& data, uint8_t key) {
    std::string out = data;
    for (auto& c : out) c ^= key;
    return out;
}

static std::string generateVarName(int seed) {
    static const char* chars = "OI1lоΟ";
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(8, 16);
    int len = dist(rng);
    std::string name = "_";
    for (int i = 0; i < len; i++) {
        int idx = rng() % 3;
        if (idx == 0) name += 'O';
        else if (idx == 1) name += 'l';
        else name += '1';
    }
    return name;
}

struct ObfuscateResult {
    std::string code;
    uint8_t key;
};

static ObfuscateResult obfuscateSource(const std::string& source) {
    std::mt19937 rng(std::random_device{}());
    uint8_t key = (rng() % 200) + 20;

    std::string stripped;
    bool inComment = false;
    for (size_t i = 0; i < source.size(); i++) {
        if (source[i] == '`') {
            inComment = !inComment;
            continue;
        }
        if (!inComment) stripped += source[i];
    }

    std::string noBlank;
    std::istringstream iss(stripped);
    std::string line;
    while (std::getline(iss, line)) {
        std::string trimmed = line;
        while (!trimmed.empty() && (trimmed.front()==' '||trimmed.front()=='\t')) trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && (trimmed.back()==' '||trimmed.back()=='\r')) trimmed.pop_back();
        if (!trimmed.empty()) noBlank += trimmed + "\n";
    }

    std::string xored = xorObfuscate(noBlank, key);
    std::string encoded = base64Encode(xored);

    std::ostringstream out;
    out << "TSLC:1:" << (int)key << ":" << encoded;

    ObfuscateResult res;
    res.code = out.str();
    res.key = key;
    return res;
}

static std::string deobfuscateSource(const std::string& compiled) {
    if (compiled.substr(0, 5) != "TSLC:") return compiled;
    size_t p1 = compiled.find(':', 5);
    if (p1 == std::string::npos) return compiled;
    size_t p2 = compiled.find(':', p1 + 1);
    if (p2 == std::string::npos) return compiled;
    uint8_t key = (uint8_t)std::stoi(compiled.substr(p1+1, p2-p1-1));
    std::string encoded = compiled.substr(p2+1);
    std::string xored = base64Decode(encoded);
    return xorObfuscate(xored, key);
}

static bool isCompiled(const std::string& source) {
    return source.substr(0, 5) == "TSLC:";
}

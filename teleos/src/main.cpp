#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "errors.h"
#include "obfuscator.h"

static void printBanner() {
    std::cout << "Teleos Language Runtime v1.1\n";
    std::cout << "Usage: teleos <script.tsl>          - run a script\n";
    std::cout << "       teleos compile <script.tsl>  - compile/obfuscate to .tslc\n";
    std::cout << "       teleos run <script.tslc>     - run compiled script\n\n";
}

static std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "\n[Teleos FileIOError]: Cannot open file: " << path << "\n";
        exit(1);
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

static void runSource(const std::string& source, const std::string& filepath) {
    std::string code = source;
    if (isCompiled(source)) {
        code = deobfuscateSource(source);
    }
    try {
        auto tokens = tokenize(code);
        Parser parser(tokens);
        auto ast = parser.parse();
        Interpreter interp;
        interp.run(ast);
    } catch (const TeleosError& e) {
        std::cerr << e.format();
        exit(1);
    } catch (const std::exception& e) {
        std::cerr << "\n[Teleos RuntimeError]: " << e.what() << "\n";
        exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) { printBanner(); return 0; }

    std::string cmd = argv[1];

    if (cmd == "compile" && argc >= 3) {
        std::string inpath = argv[2];
        std::string source = readFile(inpath);
        if (isCompiled(source)) {
            std::cerr << "[Teleos] File is already compiled.\n";
            return 1;
        }
        auto result = obfuscateSource(source);
        std::string outpath = inpath;
        if (outpath.size() >= 4 && outpath.substr(outpath.size()-4) == ".tsl")
            outpath = outpath.substr(0, outpath.size()-4) + ".tslc";
        else
            outpath += "c";
        std::ofstream ofs(outpath);
        if (!ofs.is_open()) {
            std::cerr << "[Teleos] Cannot write output: " << outpath << "\n";
            return 1;
        }
        ofs << result.code;
        std::cout << "[Teleos Compiler]\n";
        std::cout << "  Input  : " << inpath << "\n";
        std::cout << "  Output : " << outpath << "\n";
        std::cout << "  Key    : " << (int)result.key << " (embedded)\n";
        std::cout << "  Status : OK - source obfuscated and unreadable\n";
        return 0;
    }

    if (cmd == "run" && argc >= 3) {
        std::string filepath = argv[2];
        std::string source = readFile(filepath);
        runSource(source, filepath);
        return 0;
    }

    std::string filepath = argv[1];
    std::string source = readFile(filepath);
    runSource(source, filepath);
    return 0;
}

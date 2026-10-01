//main.cpp

#include <fstream>
#include <iostream>
#include <sstream>


#include "include/dfa.hpp"
#include "include/scanner.hpp"
#include "include/token.hpp"

static void dumpTable(std::ostream&out){
    out<< "| state |";
    for(int c = 0;c< NUM_CLASSES;c++){
        out<< " " << c << ' ' << CLASS_NAMES[c] << " |";
    }
    out<< "\n|---|";
    for(int c = 0;c< NUM_CLASSES;c++){
        out<< "---|";
    }
    out<< "\n";
    
    for(int s = 0;s< NUM_STATES;s++){
       out << "| " << s << ' ' << STATE_NAMES[s]
        << (ACTION[s] != A_NONE ? "*" : "") << " |";
        for(int c = 0;c< NUM_CLASSES;c++){
            out<< " " << TRANSITION[s][c] << " |";
        }
        out<< "\n";
    }
}





static int usage() {
    std::cerr << "usage: lexer <file.oc> [-o <out>]  |  lexer --dump-table\n";
    return 2;
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--dump-table") {
        dumpTable(std::cout);
        return 0;
    }

    const char* inPath = nullptr;
    const char* outPath = nullptr;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-o") {
            if (i + 1 >= argc || outPath) return usage();
            outPath = argv[++i];
        } else if (!inPath) {
            inPath = argv[i];
        } else {
            return usage();
        }
    }
    if (!inPath) return usage();

    std::ifstream in(inPath);
    if (!in) { std::cerr << "cannot open " << inPath << '\n'; return 2; }
    std::stringstream buf;
    buf << in.rdbuf();

    std::ofstream outFile;
    if (outPath) {
        outFile.open(outPath);
        if (!outFile) { std::cerr << "cannot open " << outPath << '\n'; return 2; }
    }
    std::ostream& out = outPath ? static_cast<std::ostream&>(outFile) : std::cout;

    Scanner scanner(buf.str());
    for (;;) {
        Token t = scanner.next();
        out << '<' << tokenTypeName(t.type) << ',';
        if (hasValuePart(t.type)) out << ' ' << t.value;
        out << ">\n";
        if (t.type == COMPILER_ERROR)
            std::cerr << inPath << ':' << t.line << ':' << t.column
                      << ": error: unrecognized lexeme '" << t.value << "'\n";
        if (t.type == COMPILER_EOF) break;
    }

    out.flush();
    if (!out) { std::cerr << "cannot write " << (outPath ? outPath : "standard output") << '\n'; return 2; }
    return 0;
}
//scanner.cpp
#include "scanner.hpp"
#include "dfa.hpp"
#include <utility>


Scanner::Scanner(std::string source) : src(std::move(source)) {}

void Scanner::advance(size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (src[pos] == '\n') { line++; column = 1; }
        else                  { column++; }
        pos++;
    }
}


Token Scanner::next() {
    for (;;) {   
        if (pos >= src.size())
            return Token{COMPILER_EOF, "", line, column};

        size_t start     = pos;
        int    startLine = line;
        int    startCol  = column;

        
        State  state         = S_START;
        State  lastAccept    = S_DEAD;      
        size_t lastAcceptEnd = start;       
        size_t cur           = start;

        while (cur < src.size()) {
            State nxt = TRANSITION[state][charClassOf(src[cur])];
            if (nxt == S_DEAD) break;       
            state = nxt;
            cur++;
            if (ACTION[state] != A_NONE) {  
                lastAccept    = state;     
                lastAcceptEnd = cur;
            }
        }

        if (lastAccept == S_DEAD) {
            size_t end = (cur == start) ? start + 1 : cur;
            std::string lexeme = src.substr(start, end - start);
            advance(end - start);
            return Token{COMPILER_ERROR, lexeme, startLine, startCol};
        }

       
        std::string lexeme = src.substr(start, lastAcceptEnd - start);
        advance(lastAcceptEnd - start);

       
        TokenType type;
        switch (ACTION[lastAccept]) {
            case A_WORD:   type = lookupKeyword(lexeme);    break;
            case A_INT:    type = INT;    break;
            case A_FLOAT:  type = FLOAT;  break;
            case A_OP:     type = lookupOperator(lexeme);   break;
            case A_STRING: type = STRING_LIT; break;
            case A_SKIP:   continue;      
            default:       type = COMPILER_ERROR; break;    
        }
        return Token{type, lexeme, startLine, startCol};
    }
}

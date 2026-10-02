#include "scanner.hpp"

#include <string>

namespace octac::lexer {

    Scanner::Scanner(std::string_view text, utils::DiagnosticReporter &diagnostics)
        : src(text), reporter(diagnostics) {
    }

    void Scanner::advance(std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            if (src[pos] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
            ++pos;
        }
    }

    Scanner::Match Scanner::longestMatch() const {
        State state = State::Start;
        State lastAccept = State::Dead;
        std::size_t lastAcceptEnd = pos;
        std::size_t cur = pos;

        while (cur < src.size()) {
            const State next = nextState(state, charClassOf(static_cast<unsigned char>(src[cur])));
            if (next == State::Dead) {
                break;
            }
            state = next;
            ++cur;
            if (actionOf(state) != Action::None) {
                lastAccept = state;
                lastAcceptEnd = cur;
            }
        }

        if (lastAccept == State::Dead) {
            // Nothing matched: the bytes consumed so far, and at least one, form the bad lexeme.
            return {State::Dead, cur == pos ? pos + 1 : cur};
        }
        return {lastAccept, lastAcceptEnd};
    }

    Token Scanner::next() {
        while (pos < src.size()) {
            const std::size_t startLine = line;
            const std::size_t startColumn = column;
            const Match match = longestMatch();
            const std::string lexeme(src.substr(pos, match.end - pos));
            advance(match.end - pos);

            TokenType type = TokenType::CompilerError;
            switch (actionOf(match.accept)) {
            case Action::Word:   type = lookupKeyword(lexeme); break;
            case Action::Int:    type = TokenType::Int; break;
            case Action::Float:  type = TokenType::Float; break;
            case Action::Op:     type = lookupOperator(lexeme); break;
            case Action::String: type = TokenType::StringLit; break;
            case Action::Skip:   continue;
            case Action::None:   break;
            }

            if (type == TokenType::CompilerError) {
                reporter.error(startLine, startColumn, lexeme.size(),
                               "unrecognized lexeme '" + lexeme + "'");
            }
            return Token{type, lexeme, startLine, startColumn};
        }
        return Token{TokenType::CompilerEof, "", line, column};
    }

    std::vector<Token> Scanner::tokenize() {
        std::vector<Token> tokens;
        for (;;) {
            tokens.push_back(next());
            if (tokens.back().type == TokenType::CompilerEof) {
                return tokens;
            }
        }
    }

}  // namespace octac::lexer

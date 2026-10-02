#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "dfa.hpp"
#include "diagnostics.hpp"
#include "token.hpp"

namespace octac::lexer {

    /**
     * @brief Splits OctaC source text into tokens.
     *
     * The scanner drives a DFA with maximal munch: it consumes input for as long as the DFA can
     * continue, then falls back to the last accepting state. Whitespace and comments are skipped.
     * A lexeme that matches nothing becomes a TokenType::CompilerError token and is reported to the
     * utils::DiagnosticReporter, after which scanning resumes so one run finds every lexical error.
     */
    class Scanner {
    public:
        /**
         * @brief Creates a scanner over a source text.
         *
         * @param text The text to scan. The caller must keep it alive for the lifetime of the
         *        scanner.
         * @param diagnostics Receives one error for every unrecognized lexeme.
         */
        Scanner(std::string_view text, utils::DiagnosticReporter &diagnostics);

        /**
         * @brief Scans the next token.
         *
         * Once the input is exhausted every call returns a TokenType::CompilerEof token.
         *
         * @return The next token.
         */
        Token next();

        /**
         * @brief Scans the rest of the input.
         *
         * @return Every remaining token in order, ending with the TokenType::CompilerEof token.
         */
        std::vector<Token> tokenize();

    private:
        /**
         * @brief The result of running the DFA from the current position.
         */
        struct Match {
            State accept;     ///< the last accepting state reached, State::Dead if none was
            std::size_t end;  ///< one past the last byte of the lexeme
        };

        std::string_view src;                 ///< the text being scanned
        utils::DiagnosticReporter &reporter;  ///< receives lexical errors
        std::size_t pos = 0;                  ///< offset of the next unread byte
        std::size_t line = 1;                 ///< 1-based line of the next unread byte
        std::size_t column = 1;               ///< 1-based column of the next unread byte

        /**
         * @brief Runs the DFA from the current position with maximal munch.
         *
         * @return The longest accepted lexeme, or for unmatched input the span of the bad lexeme
         *         with State::Dead as the accepting state.
         */
        Match longestMatch() const;

        /**
         * @brief Moves forward over bytes, keeping the line and column up to date.
         *
         * @param count The number of bytes to consume.
         */
        void advance(std::size_t count);
    };

}  // namespace octac::lexer

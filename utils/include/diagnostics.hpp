#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>

namespace octac::utils {

    /**
     * @brief How serious a diagnostic is.
     */
    enum class Severity {
        Note,     ///< extra information attached to another diagnostic
        Warning,  ///< suspicious code that still compiles
        Error     ///< code that cannot be compiled
    };

    /**
     * @brief Prints compiler diagnostics for one source file.
     *
     * Each diagnostic is written as a header line followed by the offending source line and a
     * caret marker under the reported span:
     *
     * @code
     * main.oc:3:10: error: unrecognized lexeme '..'
     *  3 | i32 y = 3..4;
     *    |          ^~
     * @endcode
     *
     * When colour is enabled the output uses ANSI escape sequences. Lines and columns are 1-based,
     * and a column counts bytes.
     */
    class DiagnosticReporter {
    public:
        /**
         * @brief Creates a reporter.
         *
         * @param name The name printed in every diagnostic header.
         * @param text The text of the file. The caller must keep it alive for the lifetime of the
         *        reporter.
         * @param stream The stream the diagnostics are written to.
         * @param color Whether to emit ANSI colour sequences.
         */
        DiagnosticReporter(std::string name, std::string_view text, std::ostream &stream,
                           bool color);

        /**
         * @brief Prints one diagnostic and counts it.
         *
         * @param severity How serious the diagnostic is.
         * @param line The 1-based line the span starts on.
         * @param column The 1-based column the span starts at.
         * @param length The number of bytes the caret marker covers, at least 1.
         * @param message The text of the diagnostic.
         */
        void report(Severity severity, std::size_t line, std::size_t column, std::size_t length,
                    const std::string &message);

        /**
         * @brief Reports a diagnostic with Severity::Error.
         *
         * @param line The 1-based line the span starts on.
         * @param column The 1-based column the span starts at.
         * @param length The number of bytes the caret marker covers, at least 1.
         * @param message The text of the diagnostic.
         */
        void error(std::size_t line, std::size_t column, std::size_t length,
                   const std::string &message);

        /**
         * @brief Reports a diagnostic with Severity::Warning.
         *
         * @param line The 1-based line the span starts on.
         * @param column The 1-based column the span starts at.
         * @param length The number of bytes the caret marker covers, at least 1.
         * @param message The text of the diagnostic.
         */
        void warning(std::size_t line, std::size_t column, std::size_t length,
                     const std::string &message);

        /**
         * @brief Reports a diagnostic with Severity::Note.
         *
         * @param line The 1-based line the span starts on.
         * @param column The 1-based column the span starts at.
         * @param length The number of bytes the caret marker covers, at least 1.
         * @param message The text of the diagnostic.
         */
        void note(std::size_t line, std::size_t column, std::size_t length,
                  const std::string &message);

        /**
         * @brief Returns how many errors have been reported.
         *
         * @return The error count.
         */
        std::size_t errorCount() const;

        /**
         * @brief Returns how many warnings have been reported.
         *
         * @return The warning count.
         */
        std::size_t warningCount() const;

    private:
        std::string fileName;      ///< the name printed in every diagnostic header
        std::string_view source;   ///< the text of the file, used to show source lines
        std::ostream &out;         ///< where diagnostics are written
        bool useColor;             ///< whether ANSI colour sequences are emitted
        std::size_t errors = 0;    ///< errors reported so far
        std::size_t warnings = 0;  ///< warnings reported so far

        /**
         * @brief Extracts one line of the source, without its line terminator.
         *
         * @param line The 1-based line number.
         * @return The line text, or an empty view when the line does not exist or is empty.
         */
        std::string_view lineText(std::size_t line) const;

        /**
         * @brief Wraps text in an ANSI style when colour is enabled.
         *
         * @param text The text to style.
         * @param style The ANSI escape sequence that starts the style.
         * @return The styled text, or @p text unchanged when colour is disabled.
         */
        std::string paint(const std::string &text, const char *style) const;
    };

}  // namespace octac::utils

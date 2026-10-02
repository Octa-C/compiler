#include "diagnostics.hpp"

#include <algorithm>
#include <ostream>
#include <utility>

namespace octac::utils {

    namespace {

        constexpr const char *kReset = "\033[0m";
        constexpr const char *kBold = "\033[1m";
        constexpr const char *kRed = "\033[1;31m";
        constexpr const char *kYellow = "\033[1;33m";
        constexpr const char *kCyan = "\033[1;36m";
        constexpr const char *kBlue = "\033[1;34m";
        constexpr const char *kGreen = "\033[1;32m";

        const char *severityLabel(Severity severity) {
            switch (severity) {
            case Severity::Note:    return "note";
            case Severity::Warning: return "warning";
            case Severity::Error:   return "error";
            }
            return "";
        }

        const char *severityStyle(Severity severity) {
            switch (severity) {
            case Severity::Note:    return kCyan;
            case Severity::Warning: return kYellow;
            case Severity::Error:   return kRed;
            }
            return kBold;
        }

        std::size_t digitCount(std::size_t n) {
            std::size_t digits = 1;
            while (n >= 10) {
                n /= 10;
                ++digits;
            }
            return digits;
        }

    }  // namespace

    DiagnosticReporter::DiagnosticReporter(std::string name, std::string_view text,
                                           std::ostream &stream, bool color)
        : fileName(std::move(name)), source(text), out(stream), useColor(color) {
    }

    std::string DiagnosticReporter::paint(const std::string &text, const char *style) const {
        if (!useColor) {
            return text;
        }
        return std::string(style) + text + kReset;
    }

    std::string_view DiagnosticReporter::lineText(std::size_t line) const {
        std::size_t start = 0;
        for (std::size_t current = 1; current < line; ++current) {
            const std::size_t newline = source.find('\n', start);
            if (newline == std::string_view::npos) {
                return {};
            }
            start = newline + 1;
        }
        if (start >= source.size()) {
            return {};
        }
        std::size_t end = source.find('\n', start);
        if (end == std::string_view::npos) {
            end = source.size();
        }
        std::string_view text = source.substr(start, end - start);
        if (!text.empty() && text.back() == '\r') {
            text.remove_suffix(1);
        }
        return text;
    }

    void DiagnosticReporter::report(Severity severity, std::size_t line, std::size_t column,
                                    std::size_t length, const std::string &message) {
        if (severity == Severity::Error) {
            ++errors;
        } else if (severity == Severity::Warning) {
            ++warnings;
        }

        const std::string location =
            fileName + ":" + std::to_string(line) + ":" + std::to_string(column) + ":";
        out << paint(location, kBold) << ' '
            << paint(std::string(severityLabel(severity)) + ":", severityStyle(severity)) << ' '
            << paint(message, kBold) << '\n';

        const std::string_view text = lineText(line);
        if (text.empty()) {
            return;
        }

        const std::size_t gutter = digitCount(line);
        const std::string bar = paint("|", kBlue);
        out << ' ' << paint(std::to_string(line), kBlue) << ' ' << bar << ' ' << text << '\n';

        // Tabs in the prefix are kept so the caret lines up under the reported column.
        std::string padding;
        for (std::size_t i = 0; i + 1 < column && i < text.size(); ++i) {
            padding += text[i] == '\t' ? '\t' : ' ';
        }
        if (column > text.size() + 1) {
            padding.append(column - 1 - text.size(), ' ');
        }
        const std::size_t span = std::max<std::size_t>(length, 1);
        out << ' ' << std::string(gutter, ' ') << ' ' << bar << ' ' << padding
            << paint(std::string("^") + std::string(span - 1, '~'), kGreen) << '\n';
    }

    void DiagnosticReporter::error(std::size_t line, std::size_t column, std::size_t length,
                                   const std::string &message) {
        report(Severity::Error, line, column, length, message);
    }

    void DiagnosticReporter::warning(std::size_t line, std::size_t column, std::size_t length,
                                     const std::string &message) {
        report(Severity::Warning, line, column, length, message);
    }

    void DiagnosticReporter::note(std::size_t line, std::size_t column, std::size_t length,
                                  const std::string &message) {
        report(Severity::Note, line, column, length, message);
    }

    std::size_t DiagnosticReporter::errorCount() const {
        return errors;
    }

    std::size_t DiagnosticReporter::warningCount() const {
        return warnings;
    }

}  // namespace octac::utils

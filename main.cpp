#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "arg_parser.hpp"
#include "diagnostics.hpp"
#include "file_io.hpp"
#include "scanner.hpp"
#include "terminal.hpp"

using octac::lexer::Scanner;
using octac::lexer::Token;
using octac::utils::ArgParser;
using octac::utils::DiagnosticReporter;

namespace {

    constexpr int kExitErrors = 1;
    constexpr int kExitUsage = 2;

    /**
     * @brief Prints a driver level error, one that is not tied to a position in the source.
     */
    void fatal(const std::string &message) {
        std::cerr << "octacc: error: " << message << '\n';
    }

}  // namespace

int main(int argc, char **argv) {
    ArgParser args("octacc", "The OctaC compiler.");
    args.addFlag("emit-tokens", '\0', "Stop after lexical analysis and print the token stream.");
    args.addOption("output", 'o', "file", "Write the output to <file> instead of standard output.");
    args.addPositional("input", "The OctaC source file to compile.");

    switch (args.parse(argc, argv)) {
    case ArgParser::Status::Ok:   break;
    case ArgParser::Status::Help: std::cout << args.help(); return 0;
    case ArgParser::Status::Error:
        fatal(args.error());
        std::cerr << args.usage() << '\n';
        return kExitUsage;
    }

    const std::string &inputPath = args.positional("input");
    const std::optional<std::string> outputPath = args.option("output");
    const bool emitTokens = args.flag("emit-tokens");

    if (outputPath && !emitTokens) {
        fatal("option '--output' requires '--emit-tokens'");
        return kExitUsage;
    }

    const std::optional<std::string> source = octac::utils::readFile(inputPath);
    if (!source) {
        fatal("cannot read '" + inputPath + "'");
        return kExitUsage;
    }

    std::ofstream outputFile;
    if (outputPath) {
        outputFile.open(*outputPath);
        if (!outputFile) {
            fatal("cannot open '" + *outputPath + "' for writing");
            return kExitUsage;
        }
    }
    std::ostream &out = outputPath ? static_cast<std::ostream &>(outputFile) : std::cout;

    DiagnosticReporter reporter(inputPath, *source, std::cerr, octac::utils::stderrSupportsColor());
    Scanner scanner(*source, reporter);
    const std::vector<Token> tokens = scanner.tokenize();

    if (emitTokens) {
        for (const Token &token : tokens) {
            out << token << '\n';
        }
        out.flush();
        if (!out) {
            fatal("cannot write '" + outputPath.value_or("standard output") + "'");
            return kExitUsage;
        }
    }
    return reporter.errorCount() > 0 ? kExitErrors : 0;
}

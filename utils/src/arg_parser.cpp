#include "arg_parser.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace octac::utils {

    namespace {

        constexpr std::size_t kHelpIndent = 2;
        constexpr std::size_t kHelpGap = 2;

        /**
         * @brief Writes one aligned row of the help text.
         */
        void appendRow(std::string &out, const std::string &left, const std::string &right,
                       std::size_t width) {
            out.append(kHelpIndent, ' ');
            out += left;
            out.append(width - left.size() + kHelpGap, ' ');
            out += right;
            out += '\n';
        }

    }  // namespace

    ArgParser::ArgParser(std::string programName, std::string summary)
        : program(std::move(programName)), description(std::move(summary)) {
        declare("help", 'h', "", "Show this help and exit.", false);
    }

    void ArgParser::declare(const std::string &name, char shortName, const std::string &valueName,
                            const std::string &help, bool takesValue) {
        if (findLong(name) != nullptr) {
            throw std::invalid_argument("option '--" + name + "' is declared twice");
        }
        if (shortName != '\0' && findShort(shortName) != nullptr) {
            throw std::invalid_argument(std::string("option '-") + shortName +
                                        "' is declared twice");
        }
        options.push_back({name, shortName, valueName, help, takesValue});
    }

    void ArgParser::addFlag(const std::string &name, char shortName, const std::string &help) {
        declare(name, shortName, "", help, false);
    }

    void ArgParser::addOption(const std::string &name, char shortName, const std::string &valueName,
                              const std::string &help) {
        declare(name, shortName, valueName, help, true);
    }

    void ArgParser::addPositional(const std::string &name, const std::string &help) {
        const bool taken = std::any_of(positionals.begin(), positionals.end(),
                                       [&](const PositionalSpec &p) { return p.name == name; });
        if (taken) {
            throw std::invalid_argument("argument '" + name + "' is declared twice");
        }
        positionals.push_back({name, help});
    }

    const ArgParser::OptionSpec *ArgParser::findLong(const std::string &name) const {
        for (const OptionSpec &spec : options) {
            if (spec.name == name) {
                return &spec;
            }
        }
        return nullptr;
    }

    const ArgParser::OptionSpec *ArgParser::findShort(char shortName) const {
        if (shortName == '\0') {
            return nullptr;
        }
        for (const OptionSpec &spec : options) {
            if (spec.shortName == shortName) {
                return &spec;
            }
        }
        return nullptr;
    }

    const ArgParser::OptionSpec &ArgParser::require(const std::string &name,
                                                    bool takesValue) const {
        const OptionSpec *spec = findLong(name);
        if (spec == nullptr || spec->takesValue != takesValue) {
            throw std::invalid_argument("no such " + std::string(takesValue ? "option" : "flag") +
                                        ": '" + name + "'");
        }
        return *spec;
    }

    void ArgParser::reset() {
        flagsSeen.clear();
        optionValues.clear();
        positionalValues.clear();
        errorMessage.clear();
    }

    ArgParser::Status ArgParser::fail(std::string message) {
        errorMessage = std::move(message);
        return Status::Error;
    }

    ArgParser::Status ArgParser::parse(int argc, const char *const *argv) {
        reset();
        bool optionsEnded = false;
        std::size_t nextPositional = 0;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (!optionsEnded && arg == "--") {
                optionsEnded = true;
                continue;
            }
            if (optionsEnded || arg.size() < 2 || arg[0] != '-') {
                if (nextPositional >= positionals.size()) {
                    return fail("unexpected argument '" + arg + "'");
                }
                positionalValues[positionals[nextPositional++].name] = arg;
                continue;
            }

            const bool isLong = arg[1] == '-';
            const OptionSpec *spec = nullptr;
            std::optional<std::string> attached;
            std::string shown;
            if (isLong) {
                const std::size_t eq = arg.find('=');
                shown = arg.substr(0, eq);
                spec = findLong(shown.substr(2));
                if (eq != std::string::npos) {
                    attached = arg.substr(eq + 1);
                }
            } else {
                shown = arg.substr(0, 2);
                spec = findShort(arg[1]);
                if (arg.size() > 2) {
                    attached = arg.substr(2);
                }
            }

            if (spec == nullptr || (!isLong && attached && !spec->takesValue)) {
                return fail("unknown option '" + arg + "'");
            }
            if (spec->name == "help") {
                return Status::Help;
            }

            if (!spec->takesValue) {
                if (attached) {
                    return fail("option '" + shown + "' does not take a value");
                }
                flagsSeen.insert(spec->name);
                continue;
            }

            std::string value;
            if (attached) {
                value = *attached;
            } else if (i + 1 < argc) {
                value = argv[++i];
            } else {
                return fail("option '" + shown + "' requires a value");
            }
            if (!optionValues.emplace(spec->name, std::move(value)).second) {
                return fail("option '" + shown + "' given more than once");
            }
        }

        if (nextPositional < positionals.size()) {
            return fail("missing argument '<" + positionals[nextPositional].name + ">'");
        }
        return Status::Ok;
    }

    bool ArgParser::flag(const std::string &name) const {
        require(name, false);
        return flagsSeen.count(name) != 0;
    }

    std::optional<std::string> ArgParser::option(const std::string &name) const {
        require(name, true);
        const auto it = optionValues.find(name);
        if (it == optionValues.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    const std::string &ArgParser::positional(const std::string &name) const {
        const auto it = positionalValues.find(name);
        if (it == positionalValues.end()) {
            throw std::invalid_argument("no such argument: '" + name + "'");
        }
        return it->second;
    }

    const std::string &ArgParser::error() const {
        return errorMessage;
    }

    std::string ArgParser::usage() const {
        std::string text = "usage: " + program;
        if (!options.empty()) {
            text += " [options]";
        }
        for (const PositionalSpec &p : positionals) {
            text += " <" + p.name + ">";
        }
        return text;
    }

    std::string ArgParser::help() const {
        std::vector<std::pair<std::string, std::string>> optionRows;
        std::vector<std::pair<std::string, std::string>> positionalRows;
        std::size_t width = 0;

        for (const OptionSpec &spec : options) {
            std::string left =
                spec.shortName != '\0' ? std::string("-") + spec.shortName + ", " : "    ";
            left += "--" + spec.name;
            if (spec.takesValue) {
                left += " <" + spec.valueName + ">";
            }
            width = std::max(width, left.size());
            optionRows.emplace_back(std::move(left), spec.help);
        }
        for (const PositionalSpec &p : positionals) {
            std::string left = "<" + p.name + ">";
            width = std::max(width, left.size());
            positionalRows.emplace_back(std::move(left), p.help);
        }

        std::string text = usage() + "\n\n" + description + "\n";
        if (!positionalRows.empty()) {
            text += "\nArguments:\n";
            for (const auto &row : positionalRows) {
                appendRow(text, row.first, row.second, width);
            }
        }
        text += "\nOptions:\n";
        for (const auto &row : optionRows) {
            appendRow(text, row.first, row.second, width);
        }
        return text;
    }

}  // namespace octac::utils

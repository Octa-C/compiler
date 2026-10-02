#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace octac::utils {

    /**
     * @brief A small command line parser for flags, valued options and positional arguments.
     *
     * Arguments are declared first with addFlag(), addOption() and addPositional(), then parse()
     * reads the command line. The parser understands:
     *  - long options: `--name`, `--name value` and `--name=value`
     *  - short options: `-n` and `-n value`, and `-nvalue` for options that take a value
     *  - `--` which ends option parsing, so the remaining arguments are positional
     *  - a lone `-`, which is a positional argument
     *
     * The options `--help` and `-h` are declared by the parser itself.
     */
    class ArgParser {
    public:
        /**
         * @brief The outcome of parse().
         */
        enum class Status {
            Ok,    ///< the command line is valid
            Help,  ///< help was requested, the caller should print help() and stop
            Error  ///< the command line is invalid, error() describes why
        };

        /**
         * @brief Creates a parser.
         *
         * @param programName The program name shown in usage and help text.
         * @param summary One line describing what the program does.
         */
        ArgParser(std::string programName, std::string summary);

        /**
         * @brief Declares a flag, an option that takes no value.
         *
         * @param name The long name, written without the leading dashes.
         * @param shortName The single character short name, or '\0' for none.
         * @param help The description shown in help().
         * @throws std::invalid_argument If the name or short name is already declared.
         */
        void addFlag(const std::string &name, char shortName, const std::string &help);

        /**
         * @brief Declares an option that takes a value.
         *
         * @param name The long name, written without the leading dashes.
         * @param shortName The single character short name, or '\0' for none.
         * @param valueName The placeholder for the value shown in help(), such as "file".
         * @param help The description shown in help().
         * @throws std::invalid_argument If the name or short name is already declared.
         */
        void addOption(const std::string &name, char shortName, const std::string &valueName,
                       const std::string &help);

        /**
         * @brief Declares a required positional argument.
         *
         * Positional arguments are matched in the order they are declared.
         *
         * @param name The name shown in usage and help text, and used to read the value back.
         * @param help The description shown in help().
         * @throws std::invalid_argument If the name is already declared.
         */
        void addPositional(const std::string &name, const std::string &help);

        /**
         * @brief Parses a command line.
         *
         * @param argc The argument count, as received by main().
         * @param argv The arguments, as received by main(). argv[0] is skipped.
         * @return Status::Ok when every declared argument is valid, Status::Help when help was
         *         requested, Status::Error otherwise. In the error case error() holds the reason.
         */
        Status parse(int argc, const char *const *argv);

        /**
         * @brief Tells whether a flag was given.
         *
         * @param name The long name the flag was declared with.
         * @return true if the flag was given.
         * @throws std::invalid_argument If no flag has that name.
         */
        bool flag(const std::string &name) const;

        /**
         * @brief Returns the value of an option.
         *
         * @param name The long name the option was declared with.
         * @return The value, or std::nullopt when the option was not given.
         * @throws std::invalid_argument If no option has that name.
         */
        std::optional<std::string> option(const std::string &name) const;

        /**
         * @brief Returns the value of a positional argument.
         *
         * Only meaningful after parse() returned Status::Ok.
         *
         * @param name The name the positional argument was declared with.
         * @return The value given on the command line.
         * @throws std::invalid_argument If no positional argument has that name.
         */
        const std::string &positional(const std::string &name) const;

        /**
         * @brief Returns the message describing the last parse() failure.
         *
         * @return The message, or an empty string if the last parse() did not fail.
         */
        const std::string &error() const;

        /**
         * @brief Builds the one line usage summary, for example `usage: octacc [options] <input>`.
         *
         * @return The usage line, without a trailing newline.
         */
        std::string usage() const;

        /**
         * @brief Builds the full help text: usage, description, and every declared argument.
         *
         * @return The help text, ending with a newline.
         */
        std::string help() const;

    private:
        /**
         * @brief A declared flag or option.
         */
        struct OptionSpec {
            std::string name;       ///< the long name, without dashes
            char shortName;         ///< the short name, or '\0' for none
            std::string valueName;  ///< the value placeholder in help text, empty for flags
            std::string help;       ///< the description shown in help text
            bool takesValue;        ///< true for options, false for flags
        };

        /**
         * @brief A declared positional argument.
         */
        struct PositionalSpec {
            std::string name;  ///< the name shown in usage and help text
            std::string help;  ///< the description shown in help text
        };

        std::string program;                      ///< the program name shown in usage and help text
        std::string description;                  ///< the line describing what the program does
        std::vector<OptionSpec> options;          ///< every declared flag and option
        std::vector<PositionalSpec> positionals;  ///< every declared positional argument, in order

        std::set<std::string> flagsSeen;  ///< flags given on the last command line
        std::map<std::string, std::string>
            optionValues;  ///< option values from the last command line
        std::map<std::string, std::string>
            positionalValues;      ///< positional values from the last command line
        std::string errorMessage;  ///< the reason the last parse() failed

        /**
         * @brief Registers a flag or option after checking its names are free.
         *
         * @param name The long name, without dashes.
         * @param shortName The short name, or '\0' for none.
         * @param valueName The value placeholder, empty for flags.
         * @param help The description shown in help().
         * @param takesValue true for an option, false for a flag.
         * @throws std::invalid_argument If the name or short name is already declared.
         */
        void declare(const std::string &name, char shortName, const std::string &valueName,
                     const std::string &help, bool takesValue);

        /**
         * @brief Finds an option by long name.
         *
         * @param name The long name, without dashes.
         * @return The declaration, or nullptr if there is none.
         */
        const OptionSpec *findLong(const std::string &name) const;

        /**
         * @brief Finds an option by short name.
         *
         * @param shortName The short name, without the dash.
         * @return The declaration, or nullptr if there is none or @p shortName is '\0'.
         */
        const OptionSpec *findShort(char shortName) const;

        /**
         * @brief Finds an option by long name and checks it is a flag or a valued option.
         *
         * @param name The long name.
         * @param takesValue true to require a valued option, false to require a flag.
         * @return The declaration.
         * @throws std::invalid_argument If there is no such declaration.
         */
        const OptionSpec &require(const std::string &name, bool takesValue) const;

        /**
         * @brief Clears the results of the previous parse().
         */
        void reset();

        /**
         * @brief Records a parse failure.
         *
         * @param message The reason, stored for error().
         * @return Status::Error.
         */
        Status fail(std::string message);
    };

}  // namespace octac::utils

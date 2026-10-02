#pragma once

#include <optional>
#include <string>

namespace octac::utils {

    /**
     * @brief Reads a whole file into memory.
     *
     * @param path The path of the file to read.
     * @return The file contents, or std::nullopt if the file cannot be opened or read.
     */
    std::optional<std::string> readFile(const std::string &path);

}  // namespace octac::utils

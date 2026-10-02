#include "file_io.hpp"

#include <fstream>
#include <sstream>

namespace octac::utils {

    std::optional<std::string> readFile(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return std::nullopt;
        }
        std::ostringstream contents;
        contents << in.rdbuf();
        if (in.bad()) {
            return std::nullopt;
        }
        return contents.str();
    }

}  // namespace octac::utils

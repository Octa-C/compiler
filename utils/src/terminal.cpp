#include "terminal.hpp"

#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace octac::utils {

    bool stderrSupportsColor() {
#ifdef _WIN32
        return false;
#else
        const char *noColor = std::getenv("NO_COLOR");
        if (noColor != nullptr && noColor[0] != '\0') {
            return false;
        }
        const char *term = std::getenv("TERM");
        if (term != nullptr && std::strcmp(term, "dumb") == 0) {
            return false;
        }
        return isatty(STDERR_FILENO) != 0;
#endif
    }

}  // namespace octac::utils

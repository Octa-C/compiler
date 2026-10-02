#pragma once

namespace octac::utils {

    /**
     * @brief Tells whether diagnostics written to standard error should use ANSI colours.
     *
     * Colour is enabled when standard error is a terminal, the NO_COLOR variable is unset or empty,
     * and TERM is not "dumb". It is always disabled on Windows.
     */
    bool stderrSupportsColor();

}  // namespace octac::utils

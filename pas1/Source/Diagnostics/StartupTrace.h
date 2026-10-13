#pragma once

#include <string_view>

#if JUCE_WINDOWS
#include <filesystem>
#include <fstream>

namespace pas1::diagnostics {

inline void startupTrace(std::string_view marker) noexcept {
    try {
        const auto path = std::filesystem::temp_directory_path() / "PAS1-startup.log";
        std::ofstream log(path, std::ios::app);
        if (!log)
            return;

        log.write(marker.data(), static_cast<std::streamsize>(marker.size()));
        log.put('\n');
    } catch (...) {
        // Diagnostic logging must never prevent the plugin UI from opening.
    }
}

} // namespace pas1::diagnostics

#else

namespace pas1::diagnostics {
inline void startupTrace(std::string_view) noexcept {}
} // namespace pas1::diagnostics

#endif

#pragma once
#include <string>
#include <optional>

namespace vshalygin::example {
    void write_to_console(const std::string &msg);
    std::optional<std::string> read_line_from_console();
}

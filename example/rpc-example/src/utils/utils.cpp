#include "utils.h"

#include <iostream>
#include <mutex>

namespace vshalygin::example {
    namespace {
        std::mutex s_console_mtx;
    }

    void write_to_console(const std::string &msg)
    {
        std::lock_guard g(s_console_mtx);
        std::cout << msg;
    }

    std::optional<std::string> read_line_from_console()
    {
        std::string line;
        if(!std::getline(std::cin, line)) {
            return std::nullopt;
        }

        return line;
    }
}

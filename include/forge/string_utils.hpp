#pragma once

#include <string>
#include <vector>

namespace forge {

std::string trim(
    const std::string& text
);


bool starts_with(
    const std::string& text,
    const std::string& prefix
);

std::vector<std::string> split(
    const std::string& str,
    char delimiter
);

void replace_all(
    std::string& str,
    const std::string& from,
    const std::string& to
);

}

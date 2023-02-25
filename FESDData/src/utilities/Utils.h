#pragma once
#include <string>
#include <ranges>
#include <algorithm>

static std::string getFileSafeSessionName(std::string sessionName) {
    std::ranges::replace(sessionName, ' ', '_');
    std::ranges::replace(sessionName, ':', '.');

    return sessionName;
}

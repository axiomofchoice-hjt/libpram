#pragma once

#include <format>

#include "pramsim/pramsim.hpp"

template <std::ranges::range R>
std::string str(const R& range) {
    std::string result = "[";
    bool first = true;
    for (const auto& i : range) {
        if (!first) {
            result += ", ";
        }
        result += std::format("{}", i);
        first = false;
    }
    result += "]";
    return result;
}

template <typename T>
std::string str(const pram::SharedArray<T>& vec) {
    return str(vec._data);
}

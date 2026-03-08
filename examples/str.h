#pragma once

#include <format>
#include <vector>

#include "pramsim/pramsim.h"

template <typename T>
std::string str(const std::vector<T>& vec) {
    std::string result = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += std::format("{}", vec[i]);
    }
    result += "]";
    return result;
}

template <typename T>
std::string str(const pram::SharedArray<T>& vec) {
    std::string result = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += std::format("{}", vec.data[i]);
    }
    result += "]";
    return result;
}

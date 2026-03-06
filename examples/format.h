#pragma once

#include <format>

#include "pram/pram.h"

template <typename T>
struct std::formatter<pram::SharedArray<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const pram::SharedArray<T>& array, format_context& ctx) const {
        auto out = ctx.out();
        *out++ = '[';
        for (size_t i = 0; i < array.data.size(); ++i) {
            if (i != 0) {
                out = std::format_to(out, ", ");
            }
            out = std::format_to(out, "{}", array.data[i]);
        }
        *out++ = ']';
        return out;
    }
};

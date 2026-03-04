#include <print>

#include "base/assert.h"

int main() try {
} catch (const assertion_error& e) {
    std::println("Assertion error: {}", e.what());
    return 1;
} catch (const std::exception& e) {
    std::println("Exception: {}", e.what());
    return 1;
}

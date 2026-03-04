set_project("libpram")
set_version("0.1.0")
set_languages("c++23")

add_rules("mode.debug", "mode.release")

target("pram")
    set_kind("static")
    add_files("src/**/*.cpp")
    add_includedirs("src", {private = true})
    add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror")

target("example")
    set_kind("binary")
    add_files("example/main.cpp")
    add_deps("pram")
    add_includedirs("src")
    add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror")

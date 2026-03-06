set_project("libpram")
set_version("0.1.0")
set_languages("c++23")

add_rules("mode.debug", "mode.release")

target("pram")
    set_kind("headeronly")
    add_includedirs("src", {public = true})

for _, file in ipairs(os.files("examples/*.cpp")) do
    local name = path.basename(file)
    target("pram_" .. name)
        set_kind("binary")
        add_files(file)
        add_deps("pram")
        add_cxxflags("-Wall", "-Wextra", "-Wpedantic", "-Werror")
end

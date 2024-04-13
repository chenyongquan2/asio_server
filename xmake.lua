-- vs project file
-- xmake project -k vsxmake -m "release,debug"

------------ sdk start ------------

if is_mode("debug") then
	set_runtimes("MDd")
else
	set_runtimes("MD")
end

-- set_runtimes("MD")
set_exceptions "cxx"
set_encodings "utf-8"
------------ sdk end ------------

--- 必须开启这个才能编译debug生成pdb符号文件
add_rules("mode.debug", "mode.release")
-- add_rules("mode.profile", "mode.coverage", "mode.asan", "mode.tsan", "mode.lsan", "mode.ubsan")

--- 必须开启这个clangd，才能正常更新编译数据库，方便代码补全以及代码跳转
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build", lsp = "clangd"})
-- -- add_rules("plugin.vsxmake.autoupdate")
-- add_rules("utils.install.pkgconfig_importfiles")
-- add_rules("utils.install.cmake_importfiles")

add_requireconfs("*", { configs = { debug = is_mode("debug") } })
add_runenvs("PATH", "$(projectdir)/bin")

-- set_runtimes("MD")
set_exceptions "cxx"
set_encodings "utf-8"


add_requires("conan::asio/1.24.0", { alias = "asio" })
add_requires("spdlog")
add_requires("fmt", { configs = {header_only = true}})

target "asio_srever"

add_cxxflags ("cl::/Zc:__cplusplus")
-- temp no warning
add_cxxflags ("/w")
add_cxxflags ("/bigobj")

set_languages "c++20"



add_packages("asio","spdlog","fmt")


add_files "src/*.cpp"
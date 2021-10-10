cmake_minimum_required (VERSION 3.8)

project (DINPUT8)

add_Library (config "config.cpp")

add_library (DINPUT8 SHARED "binary-domaim.cpp" "defines.def" "ui.cpp")
target_link_libraries (DINPUT8 config)

add_executable (domaim WIN32 "ui.cpp")
target_link_libraries (domaim config)
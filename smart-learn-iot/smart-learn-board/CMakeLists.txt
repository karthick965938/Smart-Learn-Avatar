cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS
    ../components/bsp-custom
)
add_compile_options(-fdiagnostics-color=always -Wno-ignored-qualifiers)
project(smart-learn-board)

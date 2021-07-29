cmake_minimum_required(VERSION 3.3)

project(ktl)

file(GLOB_RECURSE INCLUDES CONFIGURE_DEPENDS "include/*.hpp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${INCLUDES})

add_library(ktl INTERFACE)
add_library(ktl::ktl ALIAS ktl)
target_compile_features(ktl INTERFACE cxx_std_17)
target_include_directories(ktl INTERFACE include)

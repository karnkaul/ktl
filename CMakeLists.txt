cmake_minimum_required(VERSION 3.14)

enable_language(CXX)

project(ktl)

# cmake-utils
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
  cmake-utils
  GIT_REPOSITORY https://github.com/karnkaul/cmake-utils
  GIT_TAG v1.0.1
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(cmake-utils)
FetchContent_GetProperties(cmake-utils)

set(${PROJECT_NAME}_version 1.0.0)
set(${PROJECT_NAME}_soversion 1)
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${${PROJECT_NAME}_version})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} PROPERTY INTERFACE_${PROJECT_NAME}_MAJOR_VERSION ${${PROJECT_NAME}_soversion})
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
  target_sources(${PROJECT_NAME} PRIVATE
    include/ktl/async/async_queue.hpp
    include/ktl/async/kasync.hpp
    include/ktl/async/kfunction.hpp
    include/ktl/async/kfuture.hpp
    include/ktl/async/kthread.hpp
    include/ktl/async/shared_kmutex.hpp
    include/ktl/async/kmutex.hpp

    include/ktl/enum_flags/bitflags.hpp
    include/ktl/enum_flags/enum_flags.hpp
    include/ktl/enum_flags/enum_traits.hpp
    include/ktl/enum_flags/enumerate_enum.hpp

    include/ktl/debug_trap.hpp
    include/ktl/delegate.hpp
    include/ktl/either.hpp
    include/ktl/erased_semantics.hpp
    include/ktl/expected.hpp
    include/ktl/fixed_any.hpp
    include/ktl/fixed_vector.hpp
    include/ktl/n_tree.hpp
    include/ktl/not_null.hpp
    include/ktl/stack_string.hpp
    include/ktl/str_format.hpp
    include/ktl/tagged_store.hpp
  )
  get_target_property(sources ${PROJECT_NAME} SOURCES)
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
endif()

# version
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_version.hpp.in")
  message(STATUS "Configuring ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
  source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
endif()

# install and export
include("${cmake-utils_SOURCE_DIR}/cmake-utils.cmake")
install_and_export_target(TARGET ${PROJECT_NAME})

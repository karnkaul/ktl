cmake_minimum_required(VERSION 3.14)

enable_language(CXX)

set(project_name ktl)
set(${project_name}_version 1.0.5)
set(cxx_standard cxx_std_20)

project(${project_name} VERSION ${${project_name}_version})
set(is_root_project OFF) # indicate if this is the top-level project

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(is_root_project ON)
  message(STATUS "${PROJECT_NAME} is top level")
endif()

# options
option(KTL_INSTALL "Install ktl headers" ${is_root_project})

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

set(${project_name}_soversion ${PROJECT_VERSION_MAJOR})
add_library(${project_name} INTERFACE)
add_library(${project_name}::${project_name} ALIAS ${project_name})
set_property(TARGET ${project_name} PROPERTY VERSION ${${project_name}_version})
set_property(TARGET ${project_name} PROPERTY SOVERSION ${${project_name}_soversion})
set_property(TARGET ${project_name} PROPERTY INTERFACE_${project_name}_MAJOR_VERSION ${${project_name}_soversion})
set_property(TARGET ${project_name} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING ${project_name}_MAJOR_VERSION)

target_compile_features(${project_name} INTERFACE cxx_std_20)
target_include_directories(${project_name} INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
  target_sources(${project_name} PRIVATE
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
    include/ktl/fixed_pimpl.hpp
    include/ktl/fixed_vector.hpp
    include/ktl/koverloaded.hpp
    include/ktl/kvariant.hpp
    include/ktl/n_tree.hpp
    include/ktl/not_null.hpp
    include/ktl/ring_buffer.hpp
    include/ktl/ring_counter.hpp
    include/ktl/stack_string.hpp
    include/ktl/str_format.hpp
    include/ktl/str_formatter.hpp
    include/ktl/tagged_store.hpp
  )
  get_target_property(sources ${project_name} SOURCES)
  source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
endif()

# version
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_version.hpp.in")
  set(ktl_version ${PROJECT_VERSION})
  message(STATUS "Configuring ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_version.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
  source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES "${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/${PROJECT_NAME}_version.hpp")
endif()

# install and export
if(KTL_INSTALL)
  include("${cmake-utils_SOURCE_DIR}/cmake-utils.cmake")
  install_and_export_target(TARGET ${PROJECT_NAME})
endif()

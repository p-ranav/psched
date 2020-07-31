cmake_minimum_required(VERSION 3.8)

if(DEFINED PROJECT_NAME)
  set(PSCHED_SUBPROJECT ON)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.12")
  project(psched VERSION 1.9.0 LANGUAGES CXX
    HOMEPAGE_URL "https://github.com/p-ranav/psched"
    DESCRIPTION "Priority-based Task Scheduling for Modern C++")
elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.9")
  project(psched VERSION 1.9.0 LANGUAGES CXX
    DESCRIPTION "Priority-based Task Scheduling for Modern C++")
else()
  project(psched VERSION 1.9.0 LANGUAGES CXX)
endif()

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
    conan_basic_setup()
endif()

option(PSCHED_SAMPLES "Build psched samples")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(Threads REQUIRED)

add_library(psched INTERFACE)
add_library(psched::psched ALIAS psched)

target_compile_features(psched INTERFACE cxx_std_17)
target_include_directories(psched INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)
target_link_libraries(psched INTERFACE Threads::Threads)

if(PSCHED_SAMPLES)
  add_subdirectory(samples)
endif()

if(NOT PSCHED_SUBPROJECT)
  configure_package_config_file(pschedConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/pschedConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/psched)
  write_basic_package_version_file(pschedConfigVersion.cmake
                                   COMPATIBILITY AnyNewerVersion)

  configure_file(psched.pc.in psched.pc @ONLY)

  install(TARGETS psched EXPORT pschedTargets)
  install(EXPORT pschedTargets
          FILE pschedTargets.cmake
          NAMESPACE psched::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/psched)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pschedConfig.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/pschedConfigVersion.cmake
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/psched)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/psched.pc
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/psched
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          USE_SOURCE_PERMISSIONS
          PATTERN "*.h")

  if(EXISTS "${PROJECT_SOURCE_DIR}/.gitignore")
    # Simple glob to regex conversion (.gitignore => CPACK_SOURCE_IGNORE_FILES)
    file(READ ".gitignore" DOT_GITIGNORE)
    string(REPLACE ";" "RANDOMSEQUENCE" DOT_GITIGNORE "${DOT_GITIGNORE}")
    string(REPLACE "\n" ";" DOT_GITIGNORE "${DOT_GITIGNORE}")
    string(REPLACE "RANDOMSEQUENCE" "\\;" DOT_GITIGNORE "${DOT_GITIGNORE}")
    foreach(IGNORE_LINE ${DOT_GITIGNORE})
      if(NOT IGNORE_LINE OR IGNORE_LINE MATCHES "^#")
        continue()
      endif()
      string(REPLACE "\\" "\\\\" IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "." "\\\\." IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "*" ".*" IGNORE_LINE "${IGNORE_LINE}")
      string(REPLACE "+" "\\\\+" IGNORE_LINE "${IGNORE_LINE}")
      list(APPEND CPACK_SOURCE_IGNORE_FILES "${IGNORE_LINE}")
    endforeach()
  endif()

  # extra ignored files
  list(APPEND CPACK_SOURCE_IGNORE_FILES
    .editorconfig
    .git
    .gitignore
    .travis.yml
    .appveyor.yml
  )
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
  set(CPACK_GENERATOR "TGZ;TXZ")
  set(CPACK_SOURCE_GENERATOR "TGZ;TXZ")
  include(CPack)
endif()

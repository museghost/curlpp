cmake_minimum_required(VERSION 3.7)

cmake_policy(SET CMP0074 NEW)

project(curlpp)


# In response to CMake 3.0 generating warnings regarding policy CMP0042,
# the OSX RPATH settings have been updated per recommendations found
# in the CMake Wiki:
#  http://www.cmake.org/Wiki/CMake_RPATH_handling#Mac_OS_X_and_the_RPATH
#if(POLICY CMP0042)
#    cmake_policy(SET CMP0042 NEW) # MACOSX_RPATH
#    set(CMAKE_MACOSX_RPATH TRUE)
#endif()

option(CURLPP_BUILD_SHARED_LIBS "build shared libs" ON)
option(CURLPP_BUILD_STATIC_LIBS "build static libs" ON)
option(CURLPP_CURL_PATH "additional path to find curl" "")

# for unix platform, define install directories.
include(GNUInstallDirs)

if(WIN32)
    # cmake 3.4 is required for CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS
    cmake_minimum_required(VERSION 3.7)

    # c++ 14 support from cmake 3.4 or newer
    set(CMAKE_CXX_STANDARD 14) # C++14...
    set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
    set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    # c++ 14 support from cmake 3.1 or newer
    set(CMAKE_CXX_STANDARD 14) # C++14...
    set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
    set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
endif()


# Conan.io integration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

# extra (pkg-config-related files)
add_subdirectory(extras)

#########################################################################################
# Look for dependencies

# Documented at https://cmake.org/cmake/help/v3.0/module/FindCURL.html?highlight=curlpp
# Seems simple.

if (CURLPP_CURL_PATH STREQUAL "")
    message(STATUS "Looking for CURL")
    include(FindCURL)
    find_package(CURL REQUIRED)

    if(CURL_FOUND)
        message(STATUS "Found CURL version: ${CURL_VERSION_STRING}")
        message(STATUS "Using CURL include dir(s): ${CURL_INCLUDE_DIRS}")
        message(STATUS "Using CURL lib(s): ${CURL_LIBRARIES}")
    else()
        message(FATAL_ERROR "Could not find CURL")
    endif()
else()
    message(STATUS "Manually set CURL path: " ${CURLPP_CURL_PATH})
    set(CURL_INCLUDE_DIRS ${CURLPP_CURL_PATH}/include)
    set(CURL_LIBRARIES ${CURLPP_CURL_PATH}/lib/libcurl.a)
endif()

# All following targets should search these directories for headers
include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CURL_INCLUDE_DIRS}
)

#########################################################################################
# Define Targets

# If building on windows, install path will be in build/winbuild
if(CMAKE_SYSTEM MATCHES "Windows")
   if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      set(CMAKE_INSTALL_PREFIX "winbuild")
   endif()
endif()

file(GLOB_RECURSE HeaderFileList "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
file(GLOB_RECURSE SourceFileList "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

if (CURLPP_BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED ${HeaderFileList} ${SourceFileList})
    target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 0.8.2)
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 0.8.2)

endif()

if (CURLPP_BUILD_STATIC_LIBS)
    add_library(${PROJECT_NAME}_static STATIC ${HeaderFileList} ${SourceFileList})

    # Make sure that on unix-platforms shared and static libraries have
    # the same root name, but different suffixes.
    #
    #  (solution taken from https://cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F)
    #
    # Making shared and static libraries have the same root name, but different suffixes
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
    # Now the library target "curlpp_static" will be named "curlpp.lib" with MS tools.
    # This conflicts with the "curlpp.lib" import library corresponding to "curlpp.dll",
    # so we add a "lib" prefix (which is default on other platforms anyway):
    if (NOT (CMAKE_SYSTEM MATCHES "Windows"))
        SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES PREFIX "lib")
    endif()
    target_link_libraries(${PROJECT_NAME}_static ${CONAN_LIBS})
endif()

# install headers
install(DIRECTORY include/utilspp/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/utilspp")
install(DIRECTORY include/curlpp/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/curlpp")

if (CURLPP_BUILD_SHARED_LIBS)
    install(TARGETS ${PROJECT_NAME} EXPORT "${PROJECT_NAME}-targets"
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    # exporting targets
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            "${CMAKE_BINARY_DIR}/extras/${PROJECT_NAME}-config-version.cmake"
            VERSION "0.8.2"
            COMPATIBILITY AnyNewerVersion
    )

    #export(EXPORT "curlpp-targets"
    #      FILE "${CMAKE_BINARY_DIR}/curlpp/curlpp-targets.cmake"
    #      NAMESPACE "CURLPP::"
    #)

    configure_file("extras/${PROJECT_NAME}-config.cmake"
        "${CMAKE_BINARY_DIR}/extras/${PROJECT_NAME}-config.cmake"
        @ONLY
    )

    # curlpp-targets.cmake
    install(
    EXPORT "${PROJECT_NAME}-targets"
    FILE
        "${PROJECT_NAME}-targets.cmake"
    NAMESPACE
        "CURLPP::"
    DESTINATION
        "${CMAKE_INSTALL_LIBDIR}/cmake/CURLPP"
    )

    install(
        FILES
        "${CMAKE_BINARY_DIR}/extras/${PROJECT_NAME}-config.cmake"
        "${CMAKE_BINARY_DIR}/extras/${PROJECT_NAME}-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/CURLPP"
        )
endif()

if (CURLPP_BUILD_STATIC_LIBS)
    install(TARGETS ${PROJECT_NAME}_static
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

cmake_minimum_required(VERSION 3.7)

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
option(CURLPP_NGHTTP2 "supported nghttp2" ON)

# Append our module directory to CMake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-macro)

# for unix platform, define install directories.
include(GNUInstallDirs)

if(WIN32)
    # cmake 3.4 is required for CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS
    cmake_minimum_required(VERSION 3.4)

    # c++ 14 support from cmake 3.4 or newer
    set(CMAKE_CXX_STANDARD 14) # C++14...
    set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
    set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_LESS 3.1)
        cmake_minimum_required(VERSION 2.8)
        # c++11 support for cmake 2.8.12 - 3.0.x
        #
        # for non-windows platform we try to keep cmake 2.8 support
        # since entreprise distribution tends to have 2.8 version.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        # c++ 14 support from cmake 3.1 or newer
        set(CMAKE_CXX_STANDARD 14) # C++14...
        set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
        set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
    endif()
endif()


# Conan.io integration
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_CURRENT_SOURCE_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

# extra (pkg-config-related files)
add_subdirectory(extras)

include(FindCustomCURL)
# find libcurl
find_custom_curl(CURL 7.62.0)

# find openssl
include(FindCustomOpenSSL)
if(APPLE)
    message(STATUS "APPLE try to find openssl with brew")
    find_custom_openssl(OPENSSL 1.0.2)
elseif(UNIX AND NOT APPLE)
    message(STATUS "LINUX try to find openssl in /usr/local")
    find_custom_openssl(OPENSSL 1.1.0i)
elseif(MINGW)
    message(STATUS "MinGW try to find openssl in mingw")
    find_custom_openssl(OPENSSL 1.0.2)
endif()

# find nghttp2
if(CURLPP_NGHTTP2)
    message(STATUS "nghttp2 ${CURLPP_NGHTTP2}")
    include(FindCustomNghttp2)
    find_custom_nghttp2(NGHTTP2 1.34.0)
endif()

# All following targets should search these directories for headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
    target_link_libraries(${PROJECT_NAME}
            ${CURL_LIBRARIES}
            ${CONAN_LIBS}
            ${NGHTTP2_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            )
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 0)
    set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 0.8.1)

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
    target_link_libraries(${PROJECT_NAME}_static
            ${CURL_LIBRARIES}
            ${CONAN_LIBS}
            ${NGHTTP2_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            )
endif()

# install headers
install(DIRECTORY include/utilspp/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/utilspp")
install(DIRECTORY include/curlpp/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/curlpp")

if (CURLPP_BUILD_SHARED_LIBS)
    install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if (CURLPP_BUILD_STATIC_LIBS)
    install(TARGETS ${PROJECT_NAME}_static
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

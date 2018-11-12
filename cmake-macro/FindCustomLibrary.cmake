macro(set_custom_lib_vars _NAME)
    string(TOUPPER ${_NAME} _NAME_UPPER)
    string(TOLOWER ${_NAME} _NAME_LOWER)
endmacro()


macro(set_min_library_version _NAME)
    if(${ARGC} GREATER 1)
        if(${ARGV1} STREQUAL "")
            set(${_NAME}_CUSTOM_MIN_VER "0.0.0")
        else()
            set(${_NAME}_CUSTOM_MIN_VER "${ARGV1}")
        endif()
    else()
        set(${_NAME}_CUSTOM_MIN_VER "0.0.0")
    endif()
    message(STATUS "${_NAME} minimum version: ${${_NAME}_CUSTOM_MIN_VER}")
endmacro()


function(find_custom_library _NAME _LIBS _VER)
    message(STATUS "Looking for ${_NAME} (${_VER})")
    set(OLD_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}")
    unset(ENV{PKG_CONFIG_PATH})

    if (NOT ${_NAME}_CUSTOM_PATH)
        find_package(${_NAME} ${_VER} QUIET)
        if(${_NAME}_FOUND)
            set(${_NAME}_VERSION ${${_NAME}_VERSION_STRING})
        else()
            include(FindPkgConfig)
            message(STATUS "pkg_config_path : $ENV{PKG_CONFIG_PATH}")
            pkg_check_modules(${_NAME} ${_LIBS}>=${_VER})
        endif()
    else()
        message(STATUS "Manually set ${_NAME} path: ${${_NAME}_CUSTOM_PATH}")
        set(ENV{PKG_CONFIG_PATH} "${${_NAME}_CUSTOM_PATH}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
        message(STATUS "pkg_config_path : $ENV{PKG_CONFIG_PATH}")

        include(FindPkgConfig)
        pkg_check_modules(${_NAME} ${_LIBS}>=${_VER})
    endif()

    if(${_NAME}_FOUND)
        message(STATUS "Found ${_NAME} version: ${${_NAME}_VERSION}")
        message(STATUS "Using ${_NAME} include dir(s): ${${_NAME}_INCLUDE_DIRS}")
        message(STATUS "Using ${_NAME} lib dir(s): ${${_NAME}_LIBRARY_DIRS}")
        message(STATUS "Using ${_NAME} lib(s): ${${_NAME}_LIBRARIES}")

        include_directories(${${_NAME}_INCLUDE_DIRS})
        link_directories(${${_NAME}_LIBRARY_DIRS})
        message(STATUS "added it into INCLUDE_DIRECTORIES and LINK_DIRECTORIES")
    else()
        message(FATAL_ERROR "Could not find ${_NAME}")
        set(ENV{PKG_CONFIG_PATH} "${OLD_PKG_CONFIG_PATH}")
        exit()
    endif()

    set(ENV{PKG_CONFIG_PATH} "${OLD_PKG_CONFIG_PATH}")
endfunction()
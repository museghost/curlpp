function(find_custom_curl _NAME _VER)
    include(FindCustomLibrary)

    # ${_NAME_UPPER}, ${_NAME_LOWER} available
    set_custom_lib_vars(${_NAME})

    set_min_library_version(${_NAME_UPPER} ${_VER})

    find_custom_library(${_NAME_UPPER} libcurl ${${_NAME_UPPER}_CUSTOM_MIN_VER})
   
endfunction()

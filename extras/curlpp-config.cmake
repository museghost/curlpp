include(CMakeFindDependencyMacro)
find_dependency(CURL 7.62.0 REQUIRED MODULE)
include("${CMAKE_CURRENT_LIST_DIR}/curlpp-targets.cmake")

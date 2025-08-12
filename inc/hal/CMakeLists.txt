#hal_lib_public
HEADER_DIRECTORIES(hal_public_inc_directory ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${hal_public_inc_directory})
target_include_directories(halLib ${halLib_public_type} ${hal_public_inc_directory})
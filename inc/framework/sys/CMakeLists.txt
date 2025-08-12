HEADER_DIRECTORIES(lib_sysm_header_externel_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sysmLib ${sysmLib_public_type} ${lib_sysm_header_externel_dir})
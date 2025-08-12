file(GLOB_RECURSE  frameworkd_lib_hal_source_internal_dir  ./*.c)
target_sources(frameworkLib ${frameworkLib_private_type} ${frameworkd_lib_hal_source_internal_dir})

HEADER_DIRECTORIES(frameworkd_lib_hal_header_internal_dir ./)
target_include_directories(frameworkLib ${frameworkLib_private_type} ${frameworkd_lib_hal_header_internal_dir})

message(STATUS "[frameworkLib]frameworkd_lib_hal_source_internal_dir src append:")
foreach(ITEM ${frameworkd_lib_hal_source_internal_dir})
   message(${ITEM})
endforeach()
message(STATUS "[frameworkLib]frameworkd_lib_hal_source_internal_dir src append end")

target_include_directories(halLib ${halLib_public_type} ${frameworkd_lib_hal_header_internal_dir})


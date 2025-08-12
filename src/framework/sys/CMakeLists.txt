HEADER_DIRECTORIES(lib_sysm_header_internal_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(sysmLib ${sysmLib_public_type} ${sysm_header_internal_dir})

file(GLOB_RECURSE  lib_sysm_source_internal_dir  ./*.c)
target_sources(sysmLib ${sysmLib_private_type} ${lib_sysm_source_internal_dir})
target_include_directories(halLib ${halLib_public_type} ./)

message(STATUS "[sysmLib]SYSM src append:")
foreach(ITEM ${ble_mgr_src})
   message(${ITEM})
endforeach()
message(STATUS "[sysmLib]SYSM src append end------------")
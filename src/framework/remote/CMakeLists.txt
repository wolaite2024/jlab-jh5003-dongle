HEADER_DIRECTORIES(lib_remote_header_internal_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(remoteLib ${remoteLib_public_type} ${lib_remote_header_internal_dir})

file(GLOB_RECURSE  lib_remote_source_internal_dir  ./*.c)
target_sources(remoteLib ${remoteLib_private_type} ${lib_remote_source_internal_dir})

message(STATUS "[remoteLib]lib_remote_source_internal_dir src append:")
foreach(ITEM ${lib_remote_source_internal_dir})
   message(${ITEM})
endforeach()
message(STATUS "[remoteLib]lib_remote_source_internal_dir src append end")
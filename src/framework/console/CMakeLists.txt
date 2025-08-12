HEADER_DIRECTORIES(console_header_internal_dir ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(consoleLib ${consoleLib_public_type} ${console_header_internal_dir})

file(GLOB_RECURSE  console_source_internal_dir  ./*.c)
target_sources(consoleLib ${consoleLib_private_type} ${console_source_internal_dir})

message(STATUS "[consoleLib]console_source_internal_dir src append:")
foreach(ITEM ${console_source_internal_dir})
   message(${ITEM})
endforeach()
message(STATUS "[consoleLib]console_source_internal_dir src append end")
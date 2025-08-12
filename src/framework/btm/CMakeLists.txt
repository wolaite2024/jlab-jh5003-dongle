file(GLOB_RECURSE  framework_btm_all_sources  ./*.c)
file(GLOB_RECURSE  framework_btm_remove_sources  ./bt_roleswap_relay.c)
list(REMOVE_ITEM framework_btm_all_sources ${framework_btm_remove_sources})
foreach(ITEM ${framework_btm_all_sources})
   message(${ITEM})
endforeach()

target_sources(frameworkLib ${frameworkLib_private_type} ${framework_btm_all_sources})

#remote lib include bt_mgr.h
HEADER_DIRECTORIES(frameworkd_btm_header_internal_dir ./)

target_include_directories(frameworkLib ${frameworkLib_private_type} ${frameworkd_btm_header_internal_dir})
target_include_directories(remoteLib ${remoteLib_private_type} ${frameworkd_btm_header_internal_dir})

message(STATUS "[frameworkLib]framework_btm_source_internal_dir src append")
foreach(ITEM ${framework_btm_source_internal_dir})
   message(${ITEM})
endforeach()
message(STATUS "[LIB]framework_btm_source_internal_dir src append end")
file(GLOB_RECURSE  framework_audio_mgr_source_internal_dir  ./*.c)
target_sources(frameworkLib ${frameworkLib_private_type} ${framework_audio_mgr_source_internal_dir})

HEADER_DIRECTORIES(frameworkd_audio_mgr_header_internal_dir ./)
target_include_directories(frameworkLib ${frameworkLib_private_type} ${frameworkd_audio_mgr_header_internal_dir})

target_include_directories(halLib ${halLib_public_type} ./)

message(STATUS "[frameworkLib]framework_audio_mgr_source_internal_dir src append:")
foreach(ITEM ${framework_audio_mgr_source_internal_dir})
   message(${ITEM})
endforeach()
message(STATUS "[frameworkLib]framework_audio_mgr_source_internal_dir src append end")
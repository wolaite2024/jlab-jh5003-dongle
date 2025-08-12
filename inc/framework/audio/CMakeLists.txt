
HEADER_DIRECTORIES(framework_audio_externel_dir ./)
target_include_directories(frameworkLib ${frameworkLib_public_type} ${framework_audio_externel_dir})

message("---------[LIB]framework_audio_externel_dir :")
foreach(ITEM ${framework_audio_externel_dir})
   message(${ITEM})
endforeach()
message("---------[LIB]framework_audio_externel_dir ------------")
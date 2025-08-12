HEADER_DIRECTORIES(ble_audio_lib_header_dir ./)
target_include_directories(bleAudioLib ${bleAudioLib_public_type} ${ble_audio_lib_header_dir})

target_precompile_headers(bleAudioLib ${bleAudioLib_public_type}  ble_audio_flags.h)
target_include_directories(upperstackLib INTERFACE ./)
target_include_directories(upperstackLib INTERFACE ./server)

if(NOT compile_lib_only AND CONFIG_REALTEK_APP_BT_AUDIO_TRX)
target_include_directories(btAudioTrx PRIVATE ./)
target_include_directories(btAudioTrx PRIVATE ./server)
endif()
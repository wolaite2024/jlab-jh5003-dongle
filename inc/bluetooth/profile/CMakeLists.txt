target_include_directories(upperstackLib INTERFACE ./)
target_include_directories(upperstackLib INTERFACE ./server)

if(NOT compile_lib_only)
target_include_directories(btAudioTrx PRIVATE ./)
target_include_directories(btAudioTrx PRIVATE ./server)
endif()
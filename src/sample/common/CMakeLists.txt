#to do: max 
#!!!!some error because of such search way
if(NOT compile_lib_only)
HEADER_DIRECTORIES(bt_audio_trx_common_src_dir .)
target_include_directories(btAudioTrx PRIVATE ${bt_audio_trx_common_src_dir})

message("---------app_common:")
foreach(ITEM ${bt_audio_trx_common_src_dir})
   message(${ITEM})
endforeach()
message("---------app_common end------------")

#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
target_include_directories(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/findmy/crypto)
target_include_directories(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/findmy/crypto/wolfssl)
target_include_directories(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/findmy/crypto/wolfssl/wolfssl/wolfcrypt)
target_include_directories(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/findmy/crypto/third-party)

#app findmy sources
file(GLOB_RECURSE  bt_audio_trx_common_src_findmy  ./findmy/*.c)

#app_ble_service srcs
#use absolute path or relative path?
set(app_ble_service_src "${CMAKE_CURRENT_SOURCE_DIR}/app_ble_service/transmit_service.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/app_ble_service/ota_service.c")

if(CONFIG_REALTEK_APP_CFU_TRANSFER_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_cfu_transfer.c")
endif()

if(CONFIG_REALTEK_APP_COMMON_CFU_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_common_cfu.c")
endif()

if(CONFIG_REALTEK_APP_DATA_CAPTURE_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_data_capture.c")
endif()

if(CONFIG_REALTEK_APP_DSP_CONFIG_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_dsp_cfg.c")
endif()

if(CONFIG_REALTEK_APP_NREC_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_nrec.c")
endif()

if(CONFIG_REALTEK_APP_SIDETONE_ENABLE)
target_sources(btAudioTrx  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app_sidetone.c")
endif()

#app_ble_client srcs			
set(app_ble_client_src  "${CMAKE_CURRENT_SOURCE_DIR}/app_ble_client/transmit_gatt_client.c")

target_sources(btAudioTrx  PRIVATE ${bt_audio_trx_common_src_findmy})
target_sources(btAudioTrx  PRIVATE ${app_ble_client_src})	
target_sources(btAudioTrx  PRIVATE ${app_ble_service_src})	
endif()
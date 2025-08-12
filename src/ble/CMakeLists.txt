
if(NOT compile_lib_only)
target_include_directories(btAudioTrx PRIVATE ./bt_gatt_client)
target_include_directories(btAudioTrx PRIVATE ./gatt_client)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gatt_client/bt_ext_ftl.c)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gatt_client/gattc_tbl_storage.c)

target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_gatt_client/hids_gatt_client.c)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_gatt_client/ams_client.c)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_gatt_client/ancs_client.c)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bt_gatt_client/bas_gatt_client.c)

target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/profile/server/bas.c)
target_sources(btAudioTrx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/profile/server/dis.c)
endif()
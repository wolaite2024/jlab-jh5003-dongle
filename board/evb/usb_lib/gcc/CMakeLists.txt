#set OUTPUT_NAME for different target
add_library(usbLib "")
set_target_properties(usbLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME ${CONFIG_REALTEK_USB_TARGET_NAME}
					  PREFIX "")
#add_dependencies(usbLib romLib)
target_link_libraries(usbLib 
					   PUBLIC usbHalLib
					   PUBLIC halLib
					   PUBLIC romLib)
set(Usb_Version_Path "${REALTEK_SDK_BASE_PATH1}/inc/usb/usb_ver.h")					  
generate_version("usb" ${Usb_Version_Path})
#to do
target_include_directories(usbLib PRIVATE ../../../../src/sample/rws/usb/audio)

target_precompile_headers(usbLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(usbLib PUBLIC ${kconfig_header_dir_path})


add_custom_command(TARGET usbLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:usbLib>  ${lib_output_path_temp_dirt}
    COMMENT "Copying usbLib to  ${lib_output_path}/../temp"
)

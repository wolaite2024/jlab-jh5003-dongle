HEADER_DIRECTORIES(usb_inc_header_dir ./)
target_include_directories(usbLib ${usbLib_public_type} ${usb_inc_header_dir})
target_include_directories(usbHalLib ${usbHalLib_public_type} ${usb_inc_header_dir})
target_precompile_headers(usbHalLib ${usbHalLib_private_type}  ./port_macro.h)
target_precompile_headers(usbLib ${usbLib_private_type}  ./port_macro.h)

if(NOT compile_lib_only)
target_include_directories(btAudioTrx ${usbLib_public_type} ${usb_inc_header_dir})
endif()
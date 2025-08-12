HEADER_DIRECTORIES(usb_hal_inc_header_dir ./)
target_include_directories(usbHalLib ${usbHalLib_private_type} ${usb_hal_inc_header_dir})
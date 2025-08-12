HEADER_DIRECTORIES(usb_header_dir ./)
target_include_directories(usbLib ${usbLib_public_type} ${usb_header_dir})

file(GLOB_RECURSE  usb_source_dir  ./*.c)
target_sources(usbLib ${usbLib_private_type} ${usb_source_dir})
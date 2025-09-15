add_library(sysmLib "")
set_target_properties(sysmLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME ${CONFIG_REALTEK_SYSM_TARGET_NAME}
					  PREFIX ""
					  )
					  
set(Sysm_Version_Path "${REALTEK_SDK_BASE_PATH1}/src/framework/sys/sys_ver.h")
generate_version("sysm" ${Sysm_Version_Path})
#add_dependencies(sysmLib romLib)
target_link_libraries(sysmLib 
					 PUBLIC romLib 
					 PUBLIC halLib)
					 
target_precompile_headers(sysmLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(sysmLib PUBLIC ${kconfig_header_dir_path})

add_custom_command(TARGET sysmLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sysmLib>  ${lib_output_path_temp_dirt}
    COMMENT "Copying sysmLib to  ${lib_output_path}/../temp"
)
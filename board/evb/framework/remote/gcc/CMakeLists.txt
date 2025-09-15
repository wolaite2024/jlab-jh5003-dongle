add_library(remoteLib STATIC "")
set_target_properties(remoteLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  PREFIX ""
					  OUTPUT_NAME ${CONFIG_REALTEK_REMOTE_TARGET_NAME})
#add_dependencies(remoteLib halLib sysmLib frameworkLib)
target_link_libraries(remoteLib 
						PUBLIC romLib
						PUBLIC halLib 
						PUBLIC sysmLib 
						PUBLIC frameworkLib
						PUBLIC gapLib)
						
target_precompile_headers(remoteLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(remoteLib PUBLIC ${kconfig_header_dir_path})

add_custom_command(TARGET remoteLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:remoteLib>  ${lib_output_path_temp_dirt}
    COMMENT "Copying remoteLib to  ${lib_output_path}/../temp"
)
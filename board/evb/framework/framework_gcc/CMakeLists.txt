add_library(frameworkLib STATIC "")
set_target_properties(frameworkLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME ${CONFIG_REALTEK_FRAMEWORK_TARGET_NAME}
					  PREFIX "")

target_link_libraries(frameworkLib 
					    PUBLIC halLib
						PUBLIC remoteLib
						PUBLIC sysmLib
						PUBLIC upperstackLib
						PUBLIC gapLib
						PUBLIC romLib
						PUBLIC m)
						
target_precompile_headers(frameworkLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(frameworkLib PUBLIC ${kconfig_header_dir_path})

add_custom_command(TARGET frameworkLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:frameworkLib>  ${lib_output_path_temp_dirt}
    COMMENT "Copying frameworkLib to  ${lib_output_path}/../temp"
)

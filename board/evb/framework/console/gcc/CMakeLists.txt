add_library(consoleLib STATIC "")
set_target_properties(consoleLib PROPERTIES 
					  ARCHIVE_OUTPUT_DIRECTORY ${lib_output_path}
					  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
					  OUTPUT_NAME ${CONFIG_REALTEK_CONSOLE_TARGET_NAME}
					  PREFIX ""
					  )
#add_dependencies(consoleLib halLib)
target_link_libraries(consoleLib PUBLIC romLib)
target_precompile_headers(consoleLib PUBLIC  ${kconfig_header_dir_path}/config.h)
target_include_directories(consoleLib PUBLIC ${kconfig_header_dir_path})

add_custom_command(TARGET consoleLib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:consoleLib>  ${lib_output_path_temp_dirt}
    COMMENT "Copying consoleLib to  ${lib_output_path}/../temp"
)

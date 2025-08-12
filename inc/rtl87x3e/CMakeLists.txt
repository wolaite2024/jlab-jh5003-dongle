HEADER_DIRECTORIES(rom_header_dir ./)
include_directories(${rom_header_dir})

file(GLOB_RECURSE  rom_lib_header  ./*.h)
target_sources(romLib INTERFACE ${rom_lib_header})
target_include_directories(romLib INTERFACE ${rom_header_dir})
#target_include_directories(halLib PUBLIC ${rom_header_dir})


HEADER_DIRECTORIES(framework_bt_externel_dir ./)
target_include_directories(frameworkLib ${frameworkLib_public_type} ${framework_bt_externel_dir})

message("---------[LIB]framework_bt_externel_dir :")
foreach(ITEM ${framework_bt_externel_dir})
   message(${ITEM})
endforeach()
message("---------[LIB]framework_bt_externel_dir ------------")
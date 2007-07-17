set(SRC clients.cpp db.cpp dblist.cpp files.cpp filters.cpp paths.cpp
	cvs_parser.cpp)

add_library(hbackup-static STATIC ${SRC})
set_target_properties(hbackup-static
	PROPERTIES COMPILE_FLAGS "-Wall -O2 -ansi")
set_target_properties(hbackup-static
	PROPERTIES OUTPUT_NAME hbackup)

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found.  Any number of sources could be listed here.
add_library (hbackup files.cpp paths.cpp db.cpp clients.cpp cvs_parser.cpp dblist.cpp filters.cpp)

set_target_properties(hbackup
	PROPERTIES COMPILE_FLAGS "-Wall -O2 -ansi")

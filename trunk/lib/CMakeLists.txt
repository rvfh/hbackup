# Set source files
set(SRC clients.cpp db.cpp dblist.cpp files.cpp filters.cpp paths.cpp
	cvs_parser.cpp)

# Add library called that is built from the source files defined above.
# add_library(hbackup-lib STATIC ${SRC})
add_library(hbackup-lib SHARED ${SRC})

# Set compile flags
set_target_properties(hbackup-lib
	PROPERTIES COMPILE_FLAGS "-Wall -O2 -ansi")

# Change real name to 'libhbackup.a'
set_target_properties(hbackup-lib
	PROPERTIES OUTPUT_NAME hbackup)

# Install in $PREFIX/lib
install(TARGETS hbackup-lib
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
# Include test subdir
add_subdirectory(test)

# Set source files
set(SRC clients.cpp db.cpp dblist.cpp files.cpp filters.cpp paths.cpp
	cvs_parser.cpp)

# Add library called that is built from the source files defined above.
# add_library(hbackup-lib STATIC ${SRC})
add_library(hbackup-lib SHARED ${SRC})
set_target_properties(hbackup-lib
	PROPERTIES
		# Set compile flags
		COMPILE_FLAGS "-Wall -O2 -ansi"
		# Change real name to 'libhbackup.a'
		OUTPUT_NAME "hbackup"
		# Set library [so]version
		SOVERSION ${MAJOR}.${MINOR}.${BUGFIX}
		VERSION ${MAJOR})

# Install in $PREFIX/lib
install(TARGETS hbackup-lib
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)
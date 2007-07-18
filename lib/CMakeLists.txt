# Include test subdir
ADD_SUBDIRECTORY(test)

# Set source files
SET(SRC clients.cpp db.cpp dblist.cpp files.cpp filters.cpp paths.cpp
	cvs_parser.cpp)

# Add library called that is built from the source files defined above.
# add_library(hbackup-lib STATIC ${SRC})
ADD_LIBRARY(hbackup-lib SHARED ${SRC})
SET_TARGET_PROPERTIES(hbackup-lib
	PROPERTIES
		# Set compile flags
		COMPILE_FLAGS "-Wall -O2 -ansi"
		# Change real name to 'libhbackup.a'
		OUTPUT_NAME "hbackup"
		# Set library [so]version
		SOVERSION ${MAJOR}.${MINOR}.${BUGFIX}
		VERSION ${MAJOR})

# Install in $PREFIX/lib
INSTALL(TARGETS hbackup-lib
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

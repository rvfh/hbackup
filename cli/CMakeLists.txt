# Make sure the compiler can find include files from our library.
INCLUDE_DIRECTORIES(${HBACKUP_SOURCE_DIR}/lib)

# Make sure the linker can find the Hello library once it is built.
LINK_DIRECTORIES(${HBACKUP_BINARY_DIR}/lib)

# Add executable called "hbackup" that is built from the source file
# "hbackup.cpp".  The extensions are automatically found.
ADD_EXECUTABLE(hbackup hbackup.cpp)

# Set compile flags
SET_TARGET_PROPERTIES(hbackup
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi -DVERSION_MAJOR=${MAJOR} -DVERSION_MINOR=${MINOR} -DVERSION_BUGFIX=${BUGFIX} -DBUILD=0")

# add openssl library
TARGET_LINK_LIBRARIES(hbackup ssl)

# add zlib library
TARGET_LINK_LIBRARIES(hbackup z)

# Link the executable to the Hello library.
TARGET_LINK_LIBRARIES(hbackup hbackup-lib)

# Install in $PREFIX/bin
INSTALL(TARGETS hbackup
	RUNTIME DESTINATION bin)
# Make sure the compiler can find include files from our library.
include_directories (${HBACKUP_SOURCE_DIR}/lib)

# Make sure the linker can find the Hello library once it is built.
link_directories (${HBACKUP_BINARY_DIR}/lib)

# Add executable called "hbackup" that is built from the source file
# "hbackup.cpp".  The extensions are automatically found.
add_executable (hbackup hbackup.cpp)

# Set compile flags
set_target_properties(hbackup
	PROPERTIES COMPILE_FLAGS "-Wall -O2 -ansi -DVERSION_MAJOR=${MAJOR} -DVERSION_MINOR=${MINOR} -DVERSION_BUGFIX=${BUGFIX} -DBUILD=0")

# add openssl library
target_link_libraries(hbackup ssl)

# add zlib library
target_link_libraries(hbackup z)

# Link the executable to the Hello library.
target_link_libraries (hbackup hbackup-lib)

# Install in $PREFIX/bin
install(TARGETS hbackup
	RUNTIME DESTINATION bin)
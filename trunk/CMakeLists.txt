# The name of our project is "HBACKUP". CMakeLists files in this project can
# refer to the root source directory of the project as ${HBACKUP_SOURCE_DIR}
# and to the root binary directory of the project as ${HBACKUP_BINARY_DIR}.
project(HBACKUP)

# Recurse into the "lib" and "cli" subdirectories.
add_subdirectory(lib)
add_subdirectory(cli)

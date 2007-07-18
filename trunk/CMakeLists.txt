# The name of our project is "HBACKUP". CMakeLists files in this project can
# refer to the root source directory of the project as ${HBACKUP_SOURCE_DIR}
# and to the root binary directory of the project as ${HBACKUP_BINARY_DIR}.
PROJECT(Hbackup)

# Enable testing
ENABLE_TESTING()

# Version numbers
SET(MAJOR 0)
SET(MINOR 2)
SET(BUGFIX 0)

# Test tools directory
SET(HBACKUP_TEST_TOOLS_DIR ${HBACKUP_SOURCE_DIR}/test_tools)

# Recurse into the "lib" and "cli" subdirectories.
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(cli)

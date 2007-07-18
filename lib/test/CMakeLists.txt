# Our test project
PROJECT(HbackupLibTest)

# Make sure the compiler can find include files from our library.
INCLUDE_DIRECTORIES(${HBACKUP_SOURCE_DIR}/lib)

# Make sure the linker can find the Hello library once it is built.
LINK_DIRECTORIES(${HBACKUP_BINARY_DIR}/lib)

# Create test executables
ADD_EXECUTABLE(list_test list_test.cpp)
SET_TARGET_PROPERTIES(list_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(list_test ssl)
TARGET_LINK_LIBRARIES(list_test z)
TARGET_LINK_LIBRARIES(list_test hbackup-lib)
ADD_TEST(list ${HBACKUP_TEST_TOOLS_DIR}/test_run list)

ADD_EXECUTABLE(files_test files_test.cpp)
SET_TARGET_PROPERTIES(files_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(files_test ssl)
TARGET_LINK_LIBRARIES(files_test z)
TARGET_LINK_LIBRARIES(files_test hbackup-lib)
ADD_TEST(files ${HBACKUP_TEST_TOOLS_DIR}/test_run files)

ADD_EXECUTABLE(filters_test filters_test.cpp)
SET_TARGET_PROPERTIES(filters_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(filters_test ssl)
TARGET_LINK_LIBRARIES(filters_test z)
TARGET_LINK_LIBRARIES(filters_test hbackup-lib)
ADD_TEST(filters ${HBACKUP_TEST_TOOLS_DIR}/test_run filters)

ADD_EXECUTABLE(parsers_test parsers_test.cpp)
SET_TARGET_PROPERTIES(parsers_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(parsers_test ssl)
TARGET_LINK_LIBRARIES(parsers_test z)
TARGET_LINK_LIBRARIES(parsers_test hbackup-lib)
ADD_TEST(parsers ${HBACKUP_TEST_TOOLS_DIR}/test_run parsers)

ADD_EXECUTABLE(cvs_parser_test cvs_parser_test.cpp)
SET_TARGET_PROPERTIES(cvs_parser_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(cvs_parser_test ssl)
TARGET_LINK_LIBRARIES(cvs_parser_test z)
TARGET_LINK_LIBRARIES(cvs_parser_test hbackup-lib)
ADD_TEST(cvs_parser ${HBACKUP_TEST_TOOLS_DIR}/test_run cvs_parser)

ADD_EXECUTABLE(paths_test paths_test.cpp)
SET_TARGET_PROPERTIES(paths_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(paths_test ssl)
TARGET_LINK_LIBRARIES(paths_test z)
TARGET_LINK_LIBRARIES(paths_test hbackup-lib)
ADD_TEST(paths ${HBACKUP_TEST_TOOLS_DIR}/test_run paths)

ADD_EXECUTABLE(db_test db_test.cpp)
SET_TARGET_PROPERTIES(db_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(db_test ssl)
TARGET_LINK_LIBRARIES(db_test z)
TARGET_LINK_LIBRARIES(db_test hbackup-lib)
ADD_TEST(db ${HBACKUP_TEST_TOOLS_DIR}/test_run db)

ADD_EXECUTABLE(clients_test clients_test.cpp)
SET_TARGET_PROPERTIES(clients_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
TARGET_LINK_LIBRARIES(clients_test ssl)
TARGET_LINK_LIBRARIES(clients_test z)
TARGET_LINK_LIBRARIES(clients_test hbackup-lib)
ADD_TEST(clients ${HBACKUP_TEST_TOOLS_DIR}/test_run clients)

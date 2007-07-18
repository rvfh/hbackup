# Make sure the compiler can find include files from our library.
include_directories(${HBACKUP_SOURCE_DIR}/lib)

# Make sure the linker can find the Hello library once it is built.
link_directories(${HBACKUP_BINARY_DIR}/lib)

# Create test executables
add_executable(list_test list_test.cpp)
set_target_properties(list_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(list_test ssl)
target_link_libraries(list_test z)
target_link_libraries(list_test hbackup-lib)
add_test(list ../../test/test_run list)

add_executable(files_test files_test.cpp)
set_target_properties(files_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(files_test ssl)
target_link_libraries(files_test z)
target_link_libraries(files_test hbackup-lib)
add_test(files ../../test/test_run files)

add_executable(filters_test filters_test.cpp)
set_target_properties(filters_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(filters_test ssl)
target_link_libraries(filters_test z)
target_link_libraries(filters_test hbackup-lib)
add_test(filters ../../test/test_run filters)

add_executable(parsers_test parsers_test.cpp)
set_target_properties(parsers_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(parsers_test ssl)
target_link_libraries(parsers_test z)
target_link_libraries(parsers_test hbackup-lib)
add_test(parsers ../../test/test_run parsers)

add_executable(cvs_parser_test cvs_parser_test.cpp)
set_target_properties(cvs_parser_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(cvs_parser_test ssl)
target_link_libraries(cvs_parser_test z)
target_link_libraries(cvs_parser_test hbackup-lib)
add_test(cvs_parser ../../test/test_run cvs_parser)

add_executable(paths_test paths_test.cpp)
set_target_properties(paths_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(paths_test ssl)
target_link_libraries(paths_test z)
target_link_libraries(paths_test hbackup-lib)
add_test(paths ../../test/test_run paths)

add_executable(db_test db_test.cpp)
set_target_properties(db_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(db_test ssl)
target_link_libraries(db_test z)
target_link_libraries(db_test hbackup-lib)
add_test(db ../../test/test_run db)

add_executable(clients_test clients_test.cpp)
set_target_properties(clients_test
	PROPERTIES
		COMPILE_FLAGS "-Wall -O2 -ansi")
target_link_libraries(clients_test ssl)
target_link_libraries(clients_test z)
target_link_libraries(clients_test hbackup-lib)
add_test(clients ../../test/test_run clients)

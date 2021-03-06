
function (test_func name)
    set (testDir "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
    set (tests "${testDir}/${name}Test.th")
    set (testName "libcommsdsl.${name}Test")
    set (runner "libcommsdsl.${name}TestRunner.cpp")

    CXXTEST_ADD_TEST (${testName} ${runner} ${tests})
    target_compile_definitions (${testName} PRIVATE "-DSCHEMAS_DIR=\"${testDir}\"")
    target_link_libraries(${testName} ${COMMON_TEST_LIB_NAME} ${PROJECT_NAME})

    if (COMMSDSL_VALGRIND_TESTS AND VALGRIND_EXECUTABLE)
        add_test(NAME "${testName}_valgrind"
             COMMAND ${CMAKE_COMMAND}
                -DTEST_EXECUTABLE=$<TARGET_FILE:${testName}>
                -DVALGRIND_EXECUTABLE=${VALGRIND_EXECUTABLE}
                -P ${PROJECT_SOURCE_DIR}/cmake/ScriptRunUnitTestWithValgrind.cmake)
    endif ()

endfunction ()

#################################################################

if (NOT CXXTEST_FOUND)
    return()
endif ()

include_directories (
    "${CXXTEST_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set (COMMON_TEST_LIB_NAME "CommonTest")
set (common_test_src
    "CommonTestSuite.cpp"
)

if (CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "7.9"))
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-qualifiers")
endif ()

add_library(${COMMON_TEST_LIB_NAME} STATIC ${common_test_src})
target_link_libraries(${COMMON_TEST_LIB_NAME} PRIVATE ${PROJECT_NAME})

test_func (int)
test_func (float)
test_func (enum)
test_func (set)
test_func (bitfield)
test_func (bundle)
test_func (string)
test_func (data)
test_func (list)
test_func (ref)
test_func (optional)
test_func (variant)
test_func (message)
test_func (schema)
test_func (namespace)
test_func (interface)
test_func (frame)
test_func (alias)

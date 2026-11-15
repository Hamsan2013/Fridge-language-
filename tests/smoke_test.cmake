file(REMOVE_RECURSE "\${TEST_WORK_DIR}")
file(MAKE_DIRECTORY "\${TEST_WORK_DIR}")

set(script [=[
rem message as "Hello"
create file "nested/created file.txt" with message
add " world" to file "nested/created file.txt"
read file "nested/created file.txt" into loaded
show loaded

rem counter as 0
repeat 3 times {
    set counter to counter + 1
}
if counter is 3 {
    show "math-and-loop-ok"
} otherwise {
    show "math-and-loop-failed"
}

fnc greet(who) {
    show "Hi " joined with who
}
greet("FRIDGE")
]=])

file(WRITE "\${TEST_WORK_DIR}/smoke.frg" "\${script}")
execute_process(
    COMMAND "\${FRIDGE_TEST_EXECUTABLE}" run "\${TEST_WORK_DIR}/smoke.frg"
    WORKING_DIRECTORY "\${TEST_WORK_DIR}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "FRIDGE smoke run failed (\${run_result}): \${run_error}\n\${run_output}")
endif()

foreach(expected "Hello world" "math-and-loop-ok" "Hi FRIDGE")
    string(FIND "\${run_output}" "\${expected}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "Expected output '\${expected}' not found. Output:\n\${run_output}")
    endif()
endforeach()

file(READ "\${TEST_WORK_DIR}/nested/created file.txt" created_content)
if(NOT created_content STREQUAL "Hello world")
    message(FATAL_ERROR "create/add/read file regression: expected 'Hello world', got '\${created_content}'")
endif()

execute_process(
    COMMAND "\${FRIDGE_TEST_EXECUTABLE}" check "\${TEST_WORK_DIR}/smoke.frg"
    WORKING_DIRECTORY "\${TEST_WORK_DIR}"
    RESULT_VARIABLE check_result
    OUTPUT_VARIABLE check_output
    ERROR_VARIABLE check_error
)
if(NOT check_result EQUAL 0)
    message(FATAL_ERROR "FRIDGE check failed: \${check_error}\n\${check_output}")
endif()

file(WRITE "\${TEST_WORK_DIR}/bad.frg" "this is not a FRIDGE statement\n")
execute_process(
    COMMAND "\${FRIDGE_TEST_EXECUTABLE}" run "\${TEST_WORK_DIR}/bad.frg"
    WORKING_DIRECTORY "\${TEST_WORK_DIR}"
    RESULT_VARIABLE bad_result
    OUTPUT_VARIABLE bad_output
    ERROR_VARIABLE bad_error
)
if(bad_result EQUAL 0)
    message(FATAL_ERROR "Invalid statement unexpectedly returned success.")
endif()

message(STATUS "FRIDGE smoke tests passed.")

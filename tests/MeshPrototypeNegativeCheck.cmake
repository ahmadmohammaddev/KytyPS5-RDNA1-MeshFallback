# Regression for S-013 finding 7: valid SPIR-V with an incorrect binding must
# make the direct validator exit nonzero, independently of CTest output regexes.
file(READ "${SHADER_SOURCE}" fixture)
string(FIND "${fixture}" "binding = 0" binding_offset)
if(binding_offset EQUAL -1)
    message(FATAL_ERROR "Expected payload binding not found in the authored fixture")
endif()
string(REPLACE "binding = 0" "binding = 2" fixture "${fixture}")
file(MAKE_DIRECTORY "${WORK_DIR}")
file(WRITE "${WORK_DIR}/wrong_binding.comp" "${fixture}")
execute_process(
    COMMAND "${GLSLANG}" -V --target-env vulkan1.3 -o "${WORK_DIR}/wrong_binding.spv" "${WORK_DIR}/wrong_binding.comp"
    RESULT_VARIABLE compile_result OUTPUT_VARIABLE compile_output ERROR_VARIABLE compile_error
)
if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR "Negative fixture compilation failed: ${compile_output} ${compile_error}")
endif()
execute_process(
    COMMAND "${TEST_EXE}" --validate-module-vk13 "${WORK_DIR}/wrong_binding.spv"
    RESULT_VARIABLE run_result OUTPUT_VARIABLE run_output ERROR_VARIABLE run_error
)
file(WRITE "${WORK_DIR}/direct-validation.log" "${run_output}${run_error}")
if(NOT run_result EQUAL 1 OR
   NOT run_output MATCHES "spirv-validation: explicit Vulkan 1.3 fixture.*PASS" OR
   NOT run_output MATCHES "declares binding 0" OR
   NOT run_output MATCHES "RESULT: FAIL")
    message(FATAL_ERROR "Expected direct shape rejection (exit 1): ${run_result}; ${run_output}; ${run_error}")
endif()
message(STATUS "Valid SPIR-V with wrong binding rejected by direct validator (expected exit 1)")

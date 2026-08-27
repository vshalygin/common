if(NOT DEFINED COVERAGE_BINARY_DIR)
    message(FATAL_ERROR "COVERAGE_BINARY_DIR is required.")
endif()

file(GLOB_RECURSE coverage_data_files
    LIST_DIRECTORIES FALSE
    "${COVERAGE_BINARY_DIR}/*.gcda"
)

if(coverage_data_files)
    file(REMOVE ${coverage_data_files})
endif()

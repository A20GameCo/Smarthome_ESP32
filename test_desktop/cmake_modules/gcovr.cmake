if(USE_GCOV)
    include(ExternalProject)
    ExternalProject_Add(gcovr
            URL https://github.com/gcovr/gcovr/archive/3.2.zip
            URL_HASH SHA1=7411d3989116c5fa65519ee1a54237df16944ad2
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ""
            )

    ExternalProject_Get_Property(gcovr source_dir)
    SET(GCOVR ${source_dir}/scripts/gcovr)

    SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")
    SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

    add_custom_command(OUTPUT _run_gcovr_parser
            POST_BUILD
            COMMAND ${GCOVR} -r ${CMAKE_SOURCE_DIR}/src --object-dir=
            ${CMAKE_BINARY_DIR} -e test_*
            COMMAND ${GCOVR} -r ${CMAKE_SOURCE_DIR}/src --object-dir=
            ${CMAKE_BINARY_DIR} --branches --exclude-unreachable-branches -e test_*
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    add_custom_target (coverage DEPENDS _run_gcovr_parser)
endif(USE_GCOV)
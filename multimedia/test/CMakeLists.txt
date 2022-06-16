cmake_minimum_required (VERSION 3.8)

project ("multimedia_test" VERSION ${PROJECT_VERSION})

include_directories(
	${CMAKE_SOURCE_DIR}/inc
	${CMAKE_SOURCE_DIR}/external_dependencies/googletest/include
	${CMAKE_CURRENT_SOURCE_DIR}/../include
)

link_directories(
	${CMAKE_SOURCE_DIR}/lib
	${CMAKE_SOURCE_DIR}/external_dependencies/glog/lib
	${CMAKE_SOURCE_DIR}/external_dependencies/ffmpeg/lib
	${GOOGLE_TEST_LIB_PATH}
)

# get source list of the directory
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}  SRC_LIST)

# message(${SRC_LIST})

if(MSVC)
    set(VERSIONINFO_RC "${CMAKE_CURRENT_SOURCE_DIR}/version_info.rc")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version_info.rc.in" "${VERSIONINFO_RC}")
endif()

# add source to this project's library
add_executable(${PROJECT_NAME} ${SRC_LIST} ${VERSIONINFO_RC})

target_link_libraries(${PROJECT_NAME} PUBLIC
	multimedia
	gtest
	gtest_main)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
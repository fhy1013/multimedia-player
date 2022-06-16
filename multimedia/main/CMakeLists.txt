cmake_minimum_required (VERSION 3.8)


# add include directory
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_SOURCE_DIR}/inc
	${CMAKE_SOURCE_DIR}/external_dependencies/glog/include
	${CMAKE_SOURCE_DIR}/external_dependencies/ffmpeg/include
	${CMAKE_SOURCE_DIR}/external_dependencies/sdl/include
)

# add link directory
link_directories(
	${CMAKE_SOURCE_DIR}/external_dependencies/glog/lib
	${CMAKE_SOURCE_DIR}/external_dependencies/ffmpeg/lib
)


# get source list of the directory
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
# message(${SRC_LIST})

if(MSVC)
    set(VERSIONINFO_RC "${CMAKE_CURRENT_SOURCE_DIR}/version_info.rc")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version_info.rc.in" "${VERSIONINFO_RC}")
endif()

# add source to this project's executable
add_executable(${PROJECT_NAME} ${SRC_LIST} ${VERSIONINFO_RC})

# target_link_libraries(${PROJECT_NAME} avcodec)

# target_link_libraries(${PROJECT_NAME} avutil)

# target_link_libraries(${PROJECT_NAME} avformat)

# target_link_libraries(${PROJECT_NAME} swresample)

target_link_libraries(${PROJECT_NAME} glog)

target_link_libraries(${PROJECT_NAME} multimedia)
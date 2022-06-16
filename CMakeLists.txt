# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
cmake_minimum_required (VERSION 3.8)

# 设置MSVC编译编码
# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")
add_definitions(-DUNICODE -D_UNICODE)

# 声明C使用c99标准，C++使用c++17标准
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif()
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHsc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")


# 设置多线程运行时选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

# 设置googletest 运行时为 MD, 默认为MT ; 直接编译源码时使用
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_compile_options(-Wall)


# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")


# 预编译宏
# add_definitions(-DTEST)


if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    # set the output directory for the debug version of the executable
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
    # set the output directory for the debug version of the library file
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})

    # set the suffix of the library file of the debug version
    # set(CMAKE_DEBUG_POSTFIX "_d")

elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
    # set the output directory for the release version of the executable
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
    # set the output directory for the release version of the library file
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE})

    # set the suffix of the library file of the release version
    # set(CMAKE_RELEASE_POSTFIX "_r")
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message("CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
    message("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG " ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})
elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message("CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
    message("CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE " ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE})
endif()

# set google test libraries path
set(GOOGLE_TEST_LIB_PATH ${CMAKE_SOURCE_DIR}/external_dependencies/googletest/lib)
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(GOOGLE_TEST_LIB_PATH ${GOOGLE_TEST_LIB_PATH}/debug)
elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
	set(GOOGLE_TEST_LIB_PATH ${GOOGLE_TEST_LIB_PATH}/release)
endif()

enable_testing()

set(PROJECT_VERSION 1.0.0.0)
project("multimedia_player" VERSION ${PROJECT_VERSION})

# include subdirectory
add_subdirectory("multimedia")

#=============Copy Source files to Build Runtime Dir=============== 
#该内容一般放在项目顶层CMakelists.txt的最后，
#目的是将项目生成后的执行文件所需的第三方库复制到执行程序目录，
#并区分Debug和Release版本。
#该方法中的COMMAND_EXPAND_LISTS参数值得关注，可以复制列表内所有文件。
set(PROJECT_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
FILE(GLOB Glog_Runtime_Debug_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/glog/lib/*.dll
) 
FILE(GLOB Glog_Runtime_Release_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/glog/lib/*.dll
) 
FILE(GLOB Ffmpeg_Runtime_Debug_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/ffmpeg/bin/*.dll
) 
FILE(GLOB Ffmpeg_Runtime_Release_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/ffmpeg/bin/*.dll
) 
# FILE(GLOB Glfw_Runtime_Debug_DLL CONFIGURE_DEPENDS 
#     ${CMAKE_SOURCE_DIR}/external_dependencies/glfw/lib/*.dll
# ) 
# FILE(GLOB Glfw_Runtime_Release_DLL CONFIGURE_DEPENDS 
#     ${CMAKE_SOURCE_DIR}/external_dependencies/glfw/lib/*.dll
# )

# FILE(GLOB Resource_Shader_Source_Code CONFIGURE_DEPENDS 
#     ${CMAKE_SOURCE_DIR}/glfw_proxy/shader_code/*.glsl
# )

FILE(GLOB SDL_Runtime_Debug_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/sdl/bin/*.dll
) 
FILE(GLOB SDL_Runtime_Release_DLL CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/external_dependencies/sdl/bin/*.dll
)

   
add_custom_target(target ALL 
  VERBATIM 
  COMMAND_EXPAND_LISTS 
    COMMAND ${CMAKE_COMMAND} -E 
        make_directory "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/" 
    COMMAND ${CMAKE_COMMAND} -E 
        copy_if_different  
            "$<$<CONFIG:Release>:${Glog_Runtime_Release_DLL}>"  
            "$<$<CONFIG:Debug>:${Glog_Runtime_Debug_DLL}>" 
            "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/"
            COMMENT "copy glog dll to runtime dir"
    COMMAND ${CMAKE_COMMAND} -E 
        copy_if_different  
            "$<$<CONFIG:Release>:${Ffmpeg_Runtime_Release_DLL}>"  
            "$<$<CONFIG:Debug>:${Ffmpeg_Runtime_Debug_DLL}>" 
            "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/"
            COMMENT "copy ffmpeg dll to runtime dir"
    # COMMAND ${CMAKE_COMMAND} -E 
    #     copy_if_different  
    #         "$<$<CONFIG:Release>:${Glfw_Runtime_Release_DLL}>"  
    #         "$<$<CONFIG:Debug>:${Glfw_Runtime_Debug_DLL}>" 
    #         "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/"
    #         COMMENT "copy glfw dll to runtime dir"
    COMMAND ${CMAKE_COMMAND} -E 
        copy_if_different  
            "$<$<CONFIG:Release>:${SDL_Runtime_Release_DLL}>"  
            "$<$<CONFIG:Debug>:${SDL_Runtime_Debug_DLL}>" 
            "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/"
            COMMENT "copy sdl dll to runtime dir"

    # COMMAND ${CMAKE_COMMAND} -E 
    #     make_directory "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/shader_code" 
    # COMMAND ${CMAKE_COMMAND} -E 
    #     copy_if_different  
    #         "$<$<CONFIG:Release>:${Resource_Shader_Source_Code}>"  
    #         "$<$<CONFIG:Debug>:${Resource_Shader_Source_Code}>" 
    #         "${PROJECT_BINARY_DIR}/$<$<CONFIG:Release>:Release>$<$<CONFIG:Debug>:Debug>/shader_code"
    #         COMMENT "copy shader source code to runtime dir"
)

# msvc 运行时设置为MT(默认为MD)
# if(MSVC)     
#   # Use the static C library for all build types
#   MESSAGE(STATUS "link to static  C and C++ runtime lirbary(/MT /MTd)")
#   foreach(var 
# 		CMAKE_C_FLAGS_DEBUG_INIT 
# 		CMAKE_C_FLAGS_RELEASE_INIT
# 		CMAKE_C_FLAGS_MINSIZEREL_INIT 
# 		CMAKE_C_FLAGS_RELWITHDEBINFO_INIT
# 		CMAKE_CXX_FLAGS_DEBUG_INIT 
# 		CMAKE_CXX_FLAGS_RELEASE_INIT
# 		CMAKE_CXX_FLAGS_MINSIZEREL_INIT 
# 		CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT
#     )
#     if(${var} MATCHES "/MD")
#       string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
#       MESSAGE(STATUS  "${var}:${${var}}")
#     endif()    
#   endforeach()	  
# endif(MSVC)

# 设置工程多处理器编译
# IF (WIN32)
# 		if(MSVC)
# 				OPTION(USE_MP "use multiple" ON)
# 				OPTION(ProjectConfig_Global_COMPILE_FLAGS_WITH_MP 
# 					"Set The Global Option COMPILE_FLAGS /MP to target." ON)
# 				if(ProjectConfig_Global_COMPILE_FLAGS_WITH_MP OR USE_MP)
# 					set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
# 					set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
# 				endif()
# 				set(VS_STARTUP_PROJECT ${PROJECT_NAME})
# 		ENDIF(MSVC)
# ENDIF()
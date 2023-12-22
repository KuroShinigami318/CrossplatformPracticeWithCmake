# CMakeList.txt : CMake project for CrossPlatformPractice, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("CrossPlatformPractice")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(libs/common)

# Add source to this project's executable.
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable (${PROJECT_NAME} ${SOURCES} ${LIB_SOURCES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/Debug
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/Release
                      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_CURRENT_SOURCE_DIR}/MinSizeRel
                      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/RelWithDebInfo
)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets)
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(${PROJECT_NAME} copy_assets)
# file(COPY assets DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets ${CMAKE_CURRENT_SOURCE_DIR}/$<CONFIG>/assets
)
endif()
target_link_libraries(${PROJECT_NAME} CommonLib)
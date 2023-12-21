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
file(GLOB SOURCES ${CrossPlatformPractice_SOURCE_DIR}/src/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_executable (CrossPlatformPractice ${SOURCES} ${LIB_SOURCES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CrossPlatformPractice)
set_target_properties(CrossPlatformPractice PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CrossPlatformPractice_SOURCE_DIR}/Debug
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CrossPlatformPractice_SOURCE_DIR}/Release
)

target_link_libraries(CrossPlatformPractice CommonLib)

# TODO: Add tests and install targets if needed.

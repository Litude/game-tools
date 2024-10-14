# CMakeList.txt : CMake project for 4MDConverter, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (4MDConverter "src/4MDConverter.c")
if(WIN32)
  target_link_libraries(4MDConverter wsock32 ws2_32)
endif()

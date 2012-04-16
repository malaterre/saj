cmake_minimum_required(VERSION 2.8)
project(SAJ)

# ref tools (dicom3tools package)
add_executable(jpegdump jpegdump.cxx)

# SAJ applications:
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
add_executable(d3tdump d3t_dump.c simpleparser.c)
add_executable(pirldump pirl_dump.c simpleparser.c)

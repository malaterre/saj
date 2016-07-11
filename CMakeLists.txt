cmake_minimum_required(VERSION 2.8)
project(FAP)

find_package(OpenJPEG)
if(OPJ_FOUND)
include_directories( ${OPENJPEG_INCLUDE_DIRS} BEFORE)
add_executable(plane plane.cxx)
target_link_libraries(plane openjpeg)
add_executable(plane2 plane2.cxx)
target_link_libraries(plane2 openjpeg)

add_executable(testempty1 testempty1.c)
add_executable(testempty2 testempty2.c)
target_link_libraries(testempty1 openjpeg)
target_link_libraries(testempty2 openjpeg)
endif(OPJ_FOUND)

add_executable(jpegdump jpegdump.cxx)
add_executable(jp2dump jp2dump.cxx)
add_executable(dump dump.cxx)
add_executable(dump2 dump2.cxx)

# SAJ applications:
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)
add_executable(d3tdump d3t_dump.c simpleparser.c)
add_executable(pirldump pirl_dump.c simpleparser.c)
add_executable(avdump av_dump.c simpleparser.c)
if(UNIX)
target_link_libraries(avdump m)
endif()
add_executable(kdudump kdu_dump.c simpleparser.c)
add_executable(copytile copy_tile.c simpleparser.c)

# http://sf.net/projects/jpeg/files/jpeg2000_images/jpeg2000_images/j2kp4files_v1_5.zip
FIND_PATH(JPEG2000_CONFORMANCE_DATA_ROOT J2KP4files/testfiles_jp2/file1.jp2
  $ENV{JPEG2000_CONFORMANCE_DATA_ROOT}
)

file(GLOB testfiles_jp2 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/testfiles_jp2/*.jp2")
file(GLOB codestreams_profile1 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/codestreams_profile1/*.j2k")
file(GLOB codestreams_profile0 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/codestreams_profile0/*.j2k")

# OpenJPEG nonregression
FIND_PATH(OPJ_DATA_ROOT README-OPJ-Data
  PATHS
  $ENV{OPJ_DATA_ROOT}
  )

file(GLOB nonregression_j2k
  "${OPJ_DATA_ROOT}/input/nonregression/*.j2k"
  "${OPJ_DATA_ROOT}/input/nonregression/*.j2c"
  "${OPJ_DATA_ROOT}/input/nonregression/*.jpc"
  )
file(GLOB nonregression_jp2 "${OPJ_DATA_ROOT}/input/nonregression/*.j2k")

enable_testing()

find_file(pirljar
  PIRL.jar
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
if(pirljar)
  find_package(Java REQUIRED)
endif(pirljar)

# UNIX only sorry
FIND_PROGRAM(CMP_EXE cmp)
FIND_PROGRAM(DIFF_EXE diff)
FIND_PROGRAM(KDU_EXPAND_EXE kdu_expand)
# JP2 files:
foreach(jp2file ${testfiles_jp2})
  get_filename_component(jp2name "${jp2file}" NAME_WE)
  # av
  set(descpath "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/descriptions_jp2/${jp2name}.desc")
  add_test( avdump_${jp2name} avdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_cmp ${CMP_EXE} ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_diff ${DIFF_EXE} -u ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)

  # PIRL
  add_test( pirldump_${jp2name} pirldump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl )
  if(pirljar)
    if(${pirljar} IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl)
      execute_process(
        COMMAND ${Java_JAVA_EXECUTABLE} -classpath "${pirljar}" PIRL.Image_Tools.JP2_Info ${jp2file}
        RESULT_VARIABLE bla
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
        )
    endif()
  endif()
  add_test( pirldump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)
  add_test( pirldump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)

  # dicom3tools / jpegdump
  add_test( d3tdump_${jp2name} d3tdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( jpegdump_${jp2name} jpegdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t)
  add_test( d3tdump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( d3tdump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)

  # kakadu
  add_test( kdudump_${jp2name} kdudump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  if(KDU_EXPAND_EXE)
    execute_process(
      COMMAND ${KDU_EXPAND_EXE} -quiet -i ${jp2file} -o /tmp/kdu_expand.rawl -record ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
      RESULT_VARIABLE ble
    )
  endif()
  add_test( kdudump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  add_test( kdudump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
endforeach(jp2file)

# J2K (P0) stream:
foreach(jp2file ${codestreams_profile0})
  get_filename_component(jp2name "${jp2file}" NAME_WE)
  # av
  set(descpath "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/descriptions_profile0/${jp2name}syntax.txt")
  add_test( avdump_${jp2name} avdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_cmp ${CMP_EXE} ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_diff ${DIFF_EXE} -u ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)

  # PIRL
  add_test( pirldump_${jp2name} pirldump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl )
  if(pirljar)
    if(${pirljar} IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl)
      execute_process(
        COMMAND ${Java_JAVA_EXECUTABLE} -classpath "${pirljar}" PIRL.Image_Tools.JPEG2000_Codestream_Info ${jp2file}
        RESULT_VARIABLE bla
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
        )
    endif()
  endif()
  add_test( pirldump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)
  add_test( pirldump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)

  # dicom3tools / jpegdump
  add_test( d3tdump_${jp2name} d3tdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( jpegdump_${jp2name} jpegdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t)
  add_test( d3tdump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( d3tdump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)

  # kakadu
  add_test( kdudump_${jp2name} kdudump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  if(KDU_EXPAND_EXE)
    execute_process(
      COMMAND ${KDU_EXPAND_EXE} -quiet -i ${jp2file} -o /tmp/kdu_expand.rawl -record ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
      RESULT_VARIABLE ble
    )
  endif()
  add_test( kdudump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  add_test( kdudump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
endforeach(jp2file)

# J2K (P1) stream:
foreach(jp2file ${codestreams_profile1})
  get_filename_component(jp2name "${jp2file}" NAME_WE)
  # av
  set(descpath "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/descriptions_profile1/${jp2name}syntax.txt")
  add_test( avdump_${jp2name} avdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_cmp ${CMP_EXE} ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)
  add_test( avdump_${jp2name}_diff ${DIFF_EXE} -u ${descpath} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)

  # PIRL
  add_test( pirldump_${jp2name} pirldump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl )
  if(pirljar)
    if(${pirljar} IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl)
      execute_process(
        COMMAND ${Java_JAVA_EXECUTABLE} -classpath "${pirljar}" PIRL.Image_Tools.JPEG2000_Codestream_Info ${jp2file}
        RESULT_VARIABLE bla
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
        )
    endif()
  endif()
  add_test( pirldump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)
  add_test( pirldump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)

  # dicom3tools / jpegdump
  add_test( d3tdump_${jp2name} d3tdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( jpegdump_${jp2name} jpegdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t)
  add_test( d3tdump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( d3tdump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)

  # kakadu
  add_test( kdudump_${jp2name} kdudump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  if(KDU_EXPAND_EXE)
    execute_process(
      COMMAND ${KDU_EXPAND_EXE} -quiet -i ${jp2file} -o /tmp/kdu_expand.rawl -record ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
      RESULT_VARIABLE ble
    )
  endif()
  add_test( kdudump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  add_test( kdudump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
endforeach(jp2file)

# JP2 non-regression:
foreach(jp2file ${nonregression_jp2})
  get_filename_component(jp2name "${jp2file}" NAME_WE)
  # av
  add_test( avdump_${jp2name} avdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.av)

  # PIRL
  add_test( pirldump_${jp2name} pirldump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl )
  if(pirljar)
    if(${pirljar} IS_NEWER_THAN ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl)
      execute_process(
        COMMAND ${Java_JAVA_EXECUTABLE} -classpath "${pirljar}" PIRL.Image_Tools.JPEG2000_Codestream_Info ${jp2file}
        RESULT_VARIABLE bla
        OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
        )
    endif()
  endif()
  add_test( pirldump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)
  add_test( pirldump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refpirl
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.pirl)

  # dicom3tools / jpegdump
  add_test( d3tdump_${jp2name} d3tdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( jpegdump_${jp2name} jpegdump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t)
  add_test( d3tdump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)
  add_test( d3tdump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refd3t
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.d3t)

  # kakadu
  add_test( kdudump_${jp2name} kdudump ${jp2file} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  if(KDU_EXPAND_EXE)
    execute_process(
      COMMAND ${KDU_EXPAND_EXE} -quiet -i ${jp2file} -o /tmp/kdu_expand.rawl -record ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
      RESULT_VARIABLE ble
    )
  endif()
  add_test( kdudump_${jp2name}_cmp ${CMP_EXE} ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
  add_test( kdudump_${jp2name}_diff ${DIFF_EXE} -u ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.refkdu
    ${CMAKE_CURRENT_BINARY_DIR}/${jp2name}.kdu)
endforeach(jp2file)
#
#add_library(libCore STATIC internal.c)
#add_library(libA SHARED a.c)
#target_link_libraries(libA LINK_PRIVATE libCore)
#add_library(libB SHARED b.c)
#target_link_libraries(libB LINK_PUBLIC libA LINK_PRIVATE libCore)
#install(TARGETS libA libB
#  EXPORT my
#  RUNTIME DESTINATION bin
#  LIBRARY DESTINATION lib
#)
# install(EXPORT my DESTINATION blu)
#
#set_target_properties(libA PROPERTIES
#  FRAMEWORK TRUE
#  FRAMEWORK_VERSION 2.0
#  PRIVATE_HEADER "internal.h"
#  PUBLIC_HEADER "a.h;b.h"
#  #RESOURCE "test.lua"
#  #INSTALL_NAME_DIR "@executable_path/../../../Library/Frameworks"
#  DEBUG_POSTFIX -d
#)

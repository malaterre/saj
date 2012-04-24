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
add_executable(avdump av_dump.c simpleparser.c)

# http://sf.net/projects/jpeg/files/jpeg2000_images/jpeg2000_images/j2kp4files_v1_5.zip
FIND_PATH(JPEG2000_CONFORMANCE_DATA_ROOT J2KP4files/testfiles_jp2/file1.jp2
  $ENV{JPEG2000_CONFORMANCE_DATA_ROOT}
)

file(GLOB testfiles_jp2 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/testfiles_jp2/*.jp2")
file(GLOB codestreams_profile1 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/codestreams_profile1/*.j2k")
file(GLOB codestreams_profile0 "${JPEG2000_CONFORMANCE_DATA_ROOT}/J2KP4files/codestreams_profile0/*.j2k")

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
endforeach(jp2file)

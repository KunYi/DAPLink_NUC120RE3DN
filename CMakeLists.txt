
cmake_minimum_required(VERSION 3.20)

# Path to toolchain file. This one has to be before 'project()' below
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/arm-none-eabi.cmake)

project(DAP_NCU120)

set(EXECUTABLE ${PROJECT_NAME}.elf)
set(LINKER_FILE ${CMAKE_SOURCE_DIR}/gcc_arm.ld)

enable_language(C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

include(cmake/nuc120bsp.cmake)
include(cmake/daplink.cmake)

set(SRC_FILES
    main.c
    startup_NUC100Series.S
)

add_executable(${EXECUTABLE} ${SRC_FILES})

target_include_directories(${EXECUTABLE} PRIVATE
  ${CMAKE_SOURCE_DIR}
  nuc120bsp
  daplink
)


target_link_libraries(${EXECUTABLE}
	nuc120bsp
	daplink
)

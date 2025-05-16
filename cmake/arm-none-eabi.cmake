set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0)


if (CMAKE_HOST_WIN32)
  set(ARM_TOOLCHAIN_DIR "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin")
  set(TOOLCHAIN_EXT ".exe")
else (CMAKE_HOST_WIN32)
  # NOT CMAKE_HOST_WIN32
  set(ARM_TOOLCHAIN_DIR "/usr/bin")
  set(TOOLCHAIN_EXT "")
endif (CMAKE_HOST_WIN32)

set(BINUTILS_PATH ${ARM_TOOLCHAIN_DIR})
set(TOOLCHAIN_PREFIX ${ARM_TOOLCHAIN_DIR}/arm-none-eabi-)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc${TOOLCHAIN_EXT} CACHE INTERNAL "GNU C compiler")
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++${TOOLCHAIN_EXT} CACHE INTERNAL "GNU C++ compiler")
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy${TOOLCHAIN_EXT} CACHE INTERNAL "objcopy tool")
set(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump${TOOLCHAIN_EXT} CACHE INTERNAL "objdump tool")
set(CMAKE_SIZE_UTIL ${TOOLCHAIN_PREFIX}size${TOOLCHAIN_EXT} CACHE INTERNAL "size tool")
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER} CACHE INTERNAL "GNU Assembler")

set(CMAKE_FIND_ROOT_PATH ${BINUTILS_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(MCPU_FLAGS "-mthumb -mcpu=cortex-m0")
set(VFP_FLAGS "")
# set(LD_FLAGS "-nostartfiles")
set(LD_FLAGS "-specs=nano.specs -specs=nosys.specs -lc -lm")

set(CMAKE_C_FLAGS   "${MCPU_FLAGS} ${VFP_FLAGS} -Wall -fno-builtin -std=gnu11 -fdata-sections -ffunction-sections" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} -Wall -fno-builtin -fdata-sections -ffunction-sections" CACHE INTERNAL "cxx compiler flags")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "asm compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${MCPU_FLAGS} ${LD_FLAGS} -Wl,--gc-sections" CACHE INTERNAL "exe link flags")

SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "c debug compiler flags")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "cxx debug compiler flags")
SET(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "asm debug compiler flags")

SET(CMAKE_C_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "c release compiler flags")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "cxx release compiler flags")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm release compiler flags")

add_library(nuc120bsp INTERFACE)

set(NUC120BSP_DIR ${CMAKE_SOURCE_DIR}/bsp)
set(NUC120_CMSIS_DIR ${NUC120BSP_DIR}/CMSIS)
set(NUC120_DEVICE_DIR ${NUC120BSP_DIR}/Device/NUC100)
set(NUC120_STDDRV_DIR ${NUC120BSP_DIR}/StdDrv)

target_sources(nuc120bsp INTERFACE
	${NUC120_DEVICE_DIR}/Source/system_NUC100Series.c
	${NUC120_STDDRV_DIR}/src/clk.c
	${NUC120_STDDRV_DIR}/src/fmc.c
	${NUC120_STDDRV_DIR}/src/gpio.c
	${NUC120_STDDRV_DIR}/src/rtc.c
	${NUC120_STDDRV_DIR}/src/sys.c
	${NUC120_STDDRV_DIR}/src/uart.c
	${NUC120_STDDRV_DIR}/src/usbd.c
)

target_include_directories(nuc120bsp INTERFACE
	${NUC120_CMSIS_DIR}/Include
	${NUC120_DEVICE_DIR}/Include
	${NUC120_STDDRV_DIR}/inc
)

add_library(daplink INTERFACE)

set(DAPLINK_DIR ${CMAKE_SOURCE_DIR}/DAP)

target_sources(daplink INTERFACE
	${DAPLINK_DIR}/Source/DAP_vendor.c
	${DAPLINK_DIR}/Source/DAP.c
	${DAPLINK_DIR}/Source/JTAG_DP.c
	${DAPLINK_DIR}/Source/SW_DP.c
	${DAPLINK_DIR}/Source/SWO.c
	${DAPLINK_DIR}/Source/UART.c
)

target_include_directories(daplink INTERFACE
	${DAPLINK_DIR}/Include
)

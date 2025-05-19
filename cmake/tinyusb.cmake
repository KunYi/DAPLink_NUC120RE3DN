add_library(tinyusb INTERFACE)

set(TINYUSB_DIR ${CMAKE_SOURCE_DIR}/tinyusb)

target_sources(tinyusb INTERFACE
    # common
    ${TINYUSB_DIR}/tusb.c
    ${TINYUSB_DIR}/common/tusb_fifo.c
    # device
    ${TINYUSB_DIR}/device/usbd.c
    ${TINYUSB_DIR}/device/usbd_control.c
    ${TINYUSB_DIR}/class/audio/audio_device.c
    ${TINYUSB_DIR}/class/cdc/cdc_device.c
    ${TINYUSB_DIR}/class/dfu/dfu_device.c
    ${TINYUSB_DIR}/class/dfu/dfu_rt_device.c
    ${TINYUSB_DIR}/class/hid/hid_device.c
    ${TINYUSB_DIR}/class/midi/midi_device.c
    ${TINYUSB_DIR}/class/msc/msc_device.c
    ${TINYUSB_DIR}/class/net/ecm_rndis_device.c
    ${TINYUSB_DIR}/class/net/ncm_device.c
    ${TINYUSB_DIR}/class/usbtmc/usbtmc_device.c
    ${TINYUSB_DIR}/class/vendor/vendor_device.c
    ${TINYUSB_DIR}/class/video/video_device.c
    # usb device controller
    ${TINYUSB_DIR}/portable/nuvoton/nuc120/dcd_nuc120.c
)

target_include_directories(tinyusb INTERFACE
	${TINYUSB_DIR}
)


#include "NUC100Series.h"

#define NUC100_UID_SIZE	(96)
#define UNIQUE_BOARD_ID_SIZE_BYTES 	((NUC100_UID_SIZE/8))

/* C string for iSerialNumber in USB Device Descriptor, two chars per byte + terminating NUL */
char usb_serial[UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static uint32_t UID[3];

/**
 * @brief Convert MCU Unique ID (UID) to USB serial number string.
 *
 * This function reads the 96-bit Unique ID of the MCU (3 x 32-bit words)
 * using `FMC_ReadUID()` with indices 2, 1, and 0 (in that order for legacy compatibility),
 * stores them in the `UID[]` array, and converts them into a hexadecimal string.
 *
 * The UID is organized as:
 * - UID[2] = bits [95:64]
 * - UID[1] = bits [63:32]
 * - UID[0] = bits [31:0]
 *
 * The resulting string is stored in `usb_serial[]` as a 24-character
 * hexadecimal string (2 characters per byte), null-terminated.
 *
 * This string can be used as a unique USB serial number in USB descriptors.
 *
 * @note Assumes `UID` is an array of three `uint32_t` elements and
 *       `usb_serial` is a char array of size `UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1`.
 */
void usb_serial_init(void)
{
	uint8_t *pId = (uint8_t *)UID;
	UID[0] = FMC_ReadUID(0);
	UID[1] = FMC_ReadUID(1);
	UID[2] = FMC_ReadUID(2);

	for (int i = 0; i < UNIQUE_BOARD_ID_SIZE_BYTES * 2; i++)
	{
		/* Byte index inside the uid array */
        	int bi = i / 2;
		/* Use high nibble first to keep memory order (just cosmetics) */
		uint8_t nibble = (pId[bi] >> 4) & 0x0F;
		pId[bi] <<= 4;
		/* Binary to hex digit */
		usb_serial[i] = nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
	}
	usb_serial[UNIQUE_BOARD_ID_SIZE_BYTES*2] = '\0';
}

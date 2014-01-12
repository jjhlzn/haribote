#define REG_STATUS	0x1F7		/*	Status				I		*/
					/* 	Any pending interrupt is cleared whenever this register is read.
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
						   |     |     |     |     |     |     `--------- 1. Obsolete.
						   |     |     |     |     |     `--------------- 2. Obsolete.
						   |     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
						   |     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
						   |     |     `--------------------------------- 5. Device Fault / Stream Error.
						   |     `--------------------------------------- 6. Drive Ready.
						   `--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
					*/
#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |                             |     |
						   |                             |     `--------- Interrupt Enable.
						   |                             |                  - IEN=0, and the drive is selected,
						   |                             |                    drive interrupts to the host will be enabled.
						   |                             |                  - IEN=1, or the drive is not selected,
						   |                             |                    drive interrupts to the host will be disabled.
						   |                             `--------------- Software Reset.
						   |                                                - The drive is held reset when RST=1.
						   |                                                  Setting RST=0 re-enables the drive.
						   |                                                - The host must set RST=1 and wait for at least
						   |                                                  5 microsecondsbefore setting RST=0, to ensure
						   |                                                  that the drive recognizes the reset.
						   `--------------------------------------------- HOB (High Order Byte)

                    */

#define REG_DEVICE	0x1F6		/*	Drive | Head | LBA bits 24-27	I/O		*/
/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
+-----+-----+-----+-----+-----+-----+-----+-----+
|           |   \_____________________/
|           |              |
|           |              `------------ If L=0, Head Select.
|           |                                    These four bits select the head number.
|           |                                    HS0 is the least significant.
|           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
|           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
|                                               When DRV=1, drive 1 (slave) is selected.
`--------------------------------------- LBA mode. This bit selects the mode of operation.
*/

#define REG_FEATURES	0x1F1		/*	Features			O		*/
#define REG_NSECTOR	0x1F2		/*	Sector Count			I/O		*/
#define REG_LBA_LOW	0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID	0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH	0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/

#define REG_CMD		REG_STATUS	/*	Command				O		*/
/*
 +--------+---------------------------------+-----------------+
 | Command| Command Description             | Parameters Used |
 | Code   |                                 | PC SC SN CY DH  |
 +--------+---------------------------------+-----------------+
 | ECh  @ | Identify Drive                  |             D   |
 | 91h    | Initialize Drive Parameters     |    V        V   |
 | 20h    | Read Sectors With Retry         |    V  V  V  V   |
 | E8h  @ | Write Buffer                    |             D   |
 +--------+---------------------------------+-----------------+
 
 KEY FOR SYMBOLS IN THE TABLE:
 ===========================================-----=========================================================================
 PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
 SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
 SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
 CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
 DH    Register 1F6: Drive / Head
*/
#define HZ             100  /* clock freq (software settable on IBM-PC) */
/* for DEVICE register. */
#define	MAKE_DEVICE_REG(lba,drv,lba_highest) (((lba) << 6) |		\
					      ((drv) << 4) |		\
					      (lba_highest & 0xF) | 0xA0)


/***************/
/* DEFINITIONS */
/***************/
#define	HD_TIMEOUT		10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE
#define ATA_IDENTIFY		0xEC
#define ATA_READ		0x20
#define ATA_WRITE		0x30


#define REG_DATA	0x1F0		/*	Data				I/O		*/
#define SECTOR_SIZE		512

void init_hd(struct FIFO32 * fifo);
u8* hd_identify(int drive);

struct hd_cmd {
	u8	features;
	u8	count;
	u8	lba_low;
	u8	lba_mid;
	u8	lba_high;
	u8	device;
	u8	command;
};

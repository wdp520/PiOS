#include "emmc.h"
#include "mailbox.h"
#include "timer.h"
#include "stringutil.h"
#include "utilities.h"

volatile Emmc* gEmmc;
sd gDevice;
unsigned int gEmmcUseDMA = 0;

unsigned int gLastCommand = 0;
unsigned int gLastCommandSuccess = 0;
unsigned int gLastError = 0;

// Predefine the commands for ease of use
static int ACMD[] = { 
	SD_CMD_RESERVED(0),
	SD_CMD_RESERVED(1),
	SD_CMD_RESERVED(2),
	SD_CMD_RESERVED(3),
	SD_CMD_RESERVED(4),
	SD_CMD_RESERVED(5),
	SD_COMMAND_INDEX(6) | SD_RESP_R1 | IS_APP_CMD,
	SD_CMD_RESERVED(7),
	SD_CMD_RESERVED(8),
	SD_CMD_RESERVED(9),
	SD_CMD_RESERVED(10),
	SD_CMD_RESERVED(11),
	SD_CMD_RESERVED(12),
	SD_COMMAND_INDEX(13) | SD_RESP_R1 | IS_APP_CMD,
	SD_CMD_RESERVED(14),
	SD_CMD_RESERVED(15),
	SD_CMD_RESERVED(16),
	SD_CMD_RESERVED(17),
	SD_CMD_RESERVED(18),
	SD_CMD_RESERVED(19),
	SD_CMD_RESERVED(20),
	SD_CMD_RESERVED(21),
	SD_COMMAND_INDEX(22) | SD_RESP_R1 | SD_DATA_READ | IS_APP_CMD,
	SD_COMMAND_INDEX(23) | SD_RESP_R1 | IS_APP_CMD,
	SD_CMD_RESERVED(24),
	SD_CMD_RESERVED(25),
	SD_CMD_RESERVED(26),
	SD_CMD_RESERVED(27),
	SD_CMD_RESERVED(28),
	SD_CMD_RESERVED(29),
	SD_CMD_RESERVED(30),
	SD_CMD_RESERVED(31),
	SD_CMD_RESERVED(32),
	SD_CMD_RESERVED(33),
	SD_CMD_RESERVED(34),
	SD_CMD_RESERVED(35),
	SD_CMD_RESERVED(36),
	SD_CMD_RESERVED(37),
	SD_CMD_RESERVED(38),
	SD_CMD_RESERVED(39),
	SD_CMD_RESERVED(40),
	SD_COMMAND_INDEX(41) | SD_RESP_R3 | IS_APP_CMD,
	SD_COMMAND_INDEX(42) | SD_RESP_R1 | IS_APP_CMD,
	SD_CMD_RESERVED(43),
	SD_CMD_RESERVED(44),
	SD_CMD_RESERVED(45),
	SD_CMD_RESERVED(46),
	SD_CMD_RESERVED(47),
	SD_CMD_RESERVED(48),
	SD_CMD_RESERVED(49),
	SD_CMD_RESERVED(50),
	SD_COMMAND_INDEX(51) | SD_RESP_R1 | SD_DATA_READ | IS_APP_CMD
};

static int CMD[] = {
	SD_COMMAND_INDEX(0)  | SD_RESP_NONE,
	SD_CMD_RESERVED(1),			 
	SD_COMMAND_INDEX(2)  | SD_RESP_R2,
	SD_COMMAND_INDEX(3)  | SD_RESP_R6,
	SD_COMMAND_INDEX(4)  | SD_RESP_NONE,
	SD_COMMAND_INDEX(5)  | SD_RESP_R4,
	SD_COMMAND_INDEX(6)  | SD_RESP_R1,
	SD_COMMAND_INDEX(7)  | SD_RESP_R1b,
	SD_COMMAND_INDEX(8)  | SD_RESP_R7,
	SD_COMMAND_INDEX(9)  | SD_RESP_R2,
	SD_COMMAND_INDEX(10) | SD_RESP_R2,
	SD_COMMAND_INDEX(11) | SD_RESP_R1,
	SD_COMMAND_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT, 
	SD_COMMAND_INDEX(13) | SD_RESP_R1,
	SD_CMD_RESERVED(14),			 
	SD_COMMAND_INDEX(15) | SD_RESP_NONE,
	SD_COMMAND_INDEX(16) | SD_RESP_R1,
	SD_COMMAND_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
	SD_COMMAND_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK,
	SD_COMMAND_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
	SD_COMMAND_INDEX(20) | SD_RESP_R1b,
	SD_CMD_RESERVED(21),			 
	SD_CMD_RESERVED(22),			 
	SD_COMMAND_INDEX(23) | SD_RESP_R1,
	SD_COMMAND_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
	SD_COMMAND_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK,
	SD_CMD_RESERVED(26),			 
	SD_COMMAND_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
	SD_COMMAND_INDEX(28) | SD_RESP_R1b,
	SD_COMMAND_INDEX(29) | SD_RESP_R1b,
	SD_COMMAND_INDEX(30) | SD_RESP_R1b | SD_DATA_READ,
	SD_CMD_RESERVED(31),
	SD_COMMAND_INDEX(32) | SD_RESP_R1,
	SD_COMMAND_INDEX(33) | SD_RESP_R1,
	SD_CMD_RESERVED(34),
	SD_CMD_RESERVED(35),
	SD_CMD_RESERVED(36),
	SD_CMD_RESERVED(37),
	SD_COMMAND_INDEX(38) | SD_RESP_R1b,
	SD_CMD_RESERVED(39),
	SD_CMD_RESERVED(40),
	SD_CMD_RESERVED(41),
	SD_COMMAND_INDEX(42) | SD_RESP_R1,
	SD_CMD_RESERVED(43),
	SD_CMD_RESERVED(44),
	SD_CMD_RESERVED(45),
	SD_CMD_RESERVED(46),
	SD_CMD_RESERVED(47),
	SD_CMD_RESERVED(48),
	SD_CMD_RESERVED(49),
	SD_CMD_RESERVED(50),
	SD_CMD_RESERVED(51),
	SD_CMD_RESERVED(52),
	SD_CMD_RESERVED(53),
	SD_CMD_RESERVED(54),
	SD_COMMAND_INDEX(55) | SD_RESP_R1,
	SD_COMMAND_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA
};

char* gSdVersionStrings[] = { "Unknown", "Version 1", "Version 1.1", "Version 2", "Version 3", "Version 4"};

char* gInterruptErrors[] = { "Command not finished", "Data not done", "Block gap", "", "Write ready", "Read ready", "", "", "Card interrupt request",
	"", "", "", "Retune", "Boot acknowledge", "Boot operation terminated", "An error meh", "Command line timeout", "Command CRC Error", "Command line end bit not 1", "Incorrect command index", "Timeout on data line", "Data CRC Error", 
	"End bit on data line not 1", "", "Auto cmd error", "", "", "", "", "", "", ""};

void PrintErrorsInInterruptRegister(unsigned int reg)
{
	printf("[ERROR] ssed - send - Command failed to complete (interrupt:%d) ", reg);

	// Start at 1 to skip "Command not finished" message
	unsigned int i;
	for(i = 1; i < 32; i++)
	{
		if(reg & (1 << i) && strlen(gInterruptErrors[i]) > 0 && i != 15) // 15 = "error has occured", only show the detailed on(there is always one?)
		{
			printf("%d. %s.\n", i, gInterruptErrors[i]);
			return;
		}
	}
	printf("Unknown error. :-(\n");
}
	
unsigned int Emmc_GetClockDivider(unsigned int base_clock, unsigned int target_rate)
{
	// MATH - HOW DOES IT WORK!?
	unsigned int targetted_divisor = 0;
	if(target_rate > base_clock)
		targetted_divisor = 1;
	else
	{
		targetted_divisor = base_clock / target_rate;
		if(base_clock % target_rate)
			targetted_divisor--;
	}

	// TODO: Decide the clock mode to use, currently only 10-bit divided clock mode is supported
	
	// Find the first set bit 
	unsigned int divisor = -1;
	unsigned int first_bit;
	for(first_bit = 31; first_bit >= 0; first_bit--)
	{
		unsigned int bit_test = (1 << first_bit);
		if(targetted_divisor & bit_test)
		{
			divisor = first_bit;
			targetted_divisor &= ~bit_test;
			if(targetted_divisor)
				divisor++; // The divisor is not power of two, increase to fix

			break; // Found it!
		}
	}

	if(divisor == -1)
		divisor = 31;
	if(divisor >= 32)
		divisor = 31;

	if(divisor != 0)
		divisor = (1 << (divisor -1));

	if(divisor >= 0x400)
		divisor = 0x3FF;

	unsigned int freq_select = divisor & 0xFF;
	unsigned int upper_bits = (divisor >> 8) & 0x3;
	unsigned int ret = (freq_select << 8) | (upper_bits << 6);

	// For debugging
	int denominator = -1;
	if(divisor != 0)
		denominator = divisor * 2;
	int actual_clock = base_clock / denominator;
	printf("ssed - Base clock: %d, target rate: %d, divisor: %d, actual clock: %d, ret: %d\n",
		base_clock, target_rate, divisor, actual_clock, ret);

	return ret;
}

unsigned int EmmcSwitchClockRate(unsigned int base_clock, unsigned int target_rate)
{
	unsigned int divider = Emmc_GetClockDivider(base_clock, target_rate);
	if(divider == -1)
	{
		printf("ssed -  Failed to retrieve divider for target_rate: %d.\n", target_rate);
		return -1;
	}

	// Wait for command line, and DAT line to become available
	while(gEmmc->Status.bits.CmdInhibit == 1 || gEmmc->Status.bits.DatInhibit == 1)
		wait(100);

	// Turn the clock off
	gEmmc->Control1.raw &= ~(1 << 2);

	// Write the new divider
	unsigned int control1 = gEmmc->Control1.raw;
	control1 &= ~0xFFE0; // Clear old clock generator select
	control1 |= divider;

	gEmmc->Control1.raw = control1;

	// Re enable the SD clock with the new speed
	control1 |= (1 << 2);
	gEmmc->Control1.raw = control1;

	wait(200);

	printf("ssed - emmc clock rate successfully changed to: %d Hz.\n", target_rate);

	return 1;
}

unsigned int EmmcInitialise(void)
{
	gEmmc = (Emmc*)EMMC_BASE;
	gDevice.rca = 0;
	gDevice.blocks_to_transfer = 0;

	printf("ssed - Initialising...\n");

	// Power cycle to ensure initial state
	// TODO: Add error checking
	EmmcPowerCycle();

	printf("ssed - Version: %d Vendor: %d SdVersion: %d Slot status: %d\n",
		gEmmc->SlotisrVer.raw, 
		gEmmc->SlotisrVer.bits.vendor, 
		gEmmc->SlotisrVer.bits.sdversion, 
		gEmmc->SlotisrVer.bits.slot_status);

	if(gEmmc->SlotisrVer.bits.sdversion < 2)
	{
		printf("ssed - Only SDHCI versions >= 3.0 are supported.\n");
		return -1;
	}
	
	// Disable clock
	unsigned int control1 = gEmmc->Control1.raw;
	control1 |= (1 << 24);
	control1 &= ~(1 << 2);
	control1 &= ~(1 << 0);
	gEmmc->Control1.raw = control1;
	
	// Wait for the circuit to reset
	while((gEmmc->Control1.raw & (0x7 << 24)) != 0);

	// Wait for a card to be detected (should always be the case on the pi)
	while((gEmmc->Status.raw & (1 << 16)) != (1 << 16));

	if((gEmmc->Status.raw  & (1 << 16)) == 0)
	{
		printf("ssed - No card inserted, how the hell did this happen!? :)\n");
		return -1;
	}

	// TODO: Read capabilities (not documented in BCM)?

	gEmmc->Control2.raw = 0;

	unsigned int base_clock = Mailbox_SD_GetBaseFrequency();
	if(base_clock == -1)
		base_clock = 100000000;

	printf("ssed - Base clock speed: %d.\n", base_clock);

	control1 = gEmmc->Control1.raw;
	control1 |= 1; // Enable clock
	control1 |= Emmc_GetClockDivider(base_clock, 400000);
	control1 |= (7 << 16); // Data timeout TMCLK * 2^10

	gEmmc->Control1.raw = control1;

	// Wait for the clock to stabalize
	while((gEmmc->Control1.raw & 0x2) != 0);

	wait(2);

	gEmmc->Control1.raw |= 4; // Enable the clock

	wait(2);

	gEmmc->IrptEn.raw = 0; // Disable ARM interrupts
	gEmmc->Interrupt.raw = 0xFFFFFFFF;
	gEmmc->IrptMask.raw = 0x17F7137;

	wait(2);

	if(!EmmcSendCommand(CMD[GoIdleState], 0)) // CMD0, should add timeout?
	{
		printf("ssed - No CMD0 response.\n");
		return -1;
	}

	// CMD8 Check if voltage is supported. 
	// Voltage: 0001b (2.7-3.6V), Check pattern: 0xAA
	EmmcSendCommand(CMD[SendIfCond], 0x1AA); 
	if((gDevice.last_resp0 & 0xFFF) != 0x1AA)
	{
		printf("ssed - Card version is not >= 2.0.\n");
		return -1;
	}
	printf("ssed - Voltage switched to 2.7-3.6V.\n");
	
	// This only returns if it's an SDIO card, this is expected to fail for all non-SDIO cards
	//if(EmmcSendCommand(CMD[IOSetOpCond], 0))
	//{
	//	printf("ssed - Unsupported SDIO card detected.\n");
	//}
	
	// Send inquiry ACMD41 to get OCR
	if(!EmmcSendCommand(ACMD[41], 0))
	{
		printf("ssed - Inquiry ACMD41 failed.\n");
		return -1;
	}

	unsigned int card_supports_voltage_switch = (gDevice.last_resp0 & (1 << 24));
	while(1)
	{
		// Switch to 1.8V Request, Set Maximum performance, Enable SDHC and SDXC support
		// TODO: Add check to see if it's a V2 card and only enable SDHC/SDXC if it is
		unsigned int acmd41arg = 0x00ff8000 | (1 << 28) | (1 << 30);

		if(card_supports_voltage_switch)
		{
			printf("ssed - Sending ACMD41 with argument voltage switch supported.\n");
			acmd41arg |= (1 << 24);
		}

		EmmcSendCommand(ACMD[41], acmd41arg);

		wait(20);

		if((gDevice.last_resp0 >> 31) & 0x1)
		{
			gDevice.ocr = (gDevice.last_resp0 >> 8) & 0xFFFF;
			gDevice.supports_sdhc = (gDevice.last_resp0 >> 30) & 0x1;
			
			if((gDevice.last_resp0 >> 30) & 0x1)
				printf("ssed - card supports SDHC.\n");
			
			card_supports_voltage_switch = (gDevice.last_resp0 >> 24) & 0x1;

			break;
		}

		wait(200);
	}

	if(((gDevice.last_resp0 >> 30) & 0x1) == 0)
		printf("ssed - card is a SDSC.\n");
	else
		printf("ssed - card is SDHC/SDXC.\n");
	
	// We have an SD card which supports SDR12 mode at 25MHz - Set frequency
	EmmcSwitchClockRate(base_clock, SdClockNormal);

	wait(5); // Wait for clock rate to change

	if(card_supports_voltage_switch)
	{
		printf("ssed - Switching to 1.8V mode.\n");

		if(!EmmcSendCommand(CMD[VoltageSwitch], 0))
		{
			printf("ssed - CMD[VoltageSwitch] failed.\n");

			// Power off
			Mailbox_SetDevicePowerState(0, 0);

			// TODO: Call initialize again, but don't try 1.8V mode

			return -1;
		}

		// Disable SD clock
		gEmmc->Control1.raw &= ~(1 << 2);

		// Check DAT[3:0]
		unsigned int status_reg = gEmmc->Status.raw;
		unsigned int dat30 = (status_reg >> 20) & 0xF;
		if(dat30 != 0)
		{
			printf("ssed - DAT[3:0] did not settle to 0. Was: %d\n", dat30);

			// TODO: Reinitialize but don't try 1.8V mode
			return -1;
		}

		// Set 1.8V signal enable to 1
		gEmmc->Control0.raw |= (1 << 8);

		wait(5);

		// Check to make sure signal enable is still set
		if(((gEmmc->Control0.raw >> 8) & 0x1) == 0)
		{
			printf("ssed - Controller did not keep the 1.8V signal high.\n");

			// TODO: Reinitialize without 1.8V support

			return -1;
		}
	
		// Re enable sd clock
		gEmmc->Control1.raw |= (1 << 2);

		wait(10);

		status_reg = gEmmc->Status.raw;

		dat30 = (status_reg >> 20) & 0xF;

		if(dat30 != 0xF)
		{
            printf("ssed - DAT[3:0] did not settle to 1111b. Was: %d.\n", dat30);

			// TODO: Reinitialize without 1.8V support

			return -1;
		}

		printf("ssed - Voltage switch complete.\n");
	}	

	if(!EmmcSendCommand(CMD[AllSendCid], 0x0))
	{
		printf("ssed - Error sending ALL_SEND_CID.\n");
		return -1;
	}
	
	printf("ssed - Got CID: %d %d %d %d\n", gDevice.last_resp3, gDevice.last_resp2, gDevice.last_resp1, gDevice.last_resp0);
	gDevice.cid[0] = gDevice.last_resp0;
	gDevice.cid[1] = gDevice.last_resp1;
	gDevice.cid[2] = gDevice.last_resp2;
	gDevice.cid[3] = gDevice.last_resp3;

	// Enter data state
	if(!EmmcSendCommand(CMD[SendRelativeAddr], 0))
	{
		printf("ssed - Error sending SEND_RELATIVE_ADDR.\n");
		return -1;
	}

	unsigned int cmd3_resp = gDevice.last_resp0;

	printf("ssed - Relative address: %d.\n", cmd3_resp);

	gDevice.rca = (cmd3_resp >> 16) & 0xFFFF;
	unsigned int crc_error = (cmd3_resp >> 15) & 0x1;
	unsigned int illegal_cmd = (cmd3_resp >> 14) & 0x1;
	unsigned int error = (cmd3_resp >> 13) & 0x1;
	unsigned int status = (cmd3_resp >> 9) & 0xF;
	unsigned int ready = (cmd3_resp >> 8) & 0x1;

	if(crc_error)
	{
		printf("ssed - CMD3 CRC error.\n");
		return -1;
	}

	if(illegal_cmd)
	{
		printf("ssed - CMD3 illegal cmd.\n");
		return -1;
	}

	if(error)
	{
		printf("ssed - CMD3 generic error.\n");
		return -1;
	}

	if(!ready)
	{
		printf("ssed - CMD3 not ready for data.\n");
		return -1;
	}

	printf("ssed - RCA: %d\n", gDevice.rca);

	// Not select the card to toggle it to transfer state
	if(!EmmcSendCommand(CMD[SelectCard], gDevice.rca << 16))
	{
		printf("ssed - Error sending SELECT_CARD.\n");
		return -1;
	}

	unsigned int cmd7_resp = gDevice.last_resp0;

	status = (cmd7_resp >> 9) & 0xF;
	if((status != 3) && (status != 4))
	{
		printf("ssed - Invalid status: %d.\n", status);
		return -1;
	}

	// Get card status (For debugging) to ensure we're in transfer mode
	if(!EmmcSendCommand(CMD[SendStatus], gDevice.rca << 16))
	{
		printf("ssed - Could not retrieve status from card.\n");
		return -1;
	}

	// Set block length to 512 if it's not a SDHC card
	if(!gDevice.supports_sdhc)
	{
		if(!EmmcSendCommand(CMD[SetBlockLen], 512))
		{
			printf("ssed - Failed to set initial block length to 512.\n");
			return -1;
		}
	}


	// Black magic
	gDevice.block_size = 512;
	unsigned int controller_block_size = gEmmc->BlockCountSize.raw;
	controller_block_size &= (~0xFFF);
	controller_block_size |= 0x200;
	gEmmc->BlockCountSize.raw = controller_block_size;
	
	printf("ssed - Getting SCR register.\n");
	
	// This is a data command, so setup the buffer
	gDevice.receive_buffer = (unsigned int*)&(gDevice.scr.scr[0]); // 1174
	gDevice.block_size = 8;
	gDevice.blocks_to_transfer = 1;
	
	// Try sending it 5 times, because CRC hobbits sleep early on
	unsigned int retries = 0;
	unsigned int acmd51SendResult;

	// Not sure if we should retry here?
	while(!(acmd51SendResult = EmmcSendCommand(ACMD[51], 0)) && retries++ < 5);

	if(!acmd51SendResult)
	{
		printf("ssed - Failed to send ACMD51.\n");
		return -1;
	}

	printf("ssed - ACMD51 response: %d, %d, %d, %d\n", gDevice.last_resp0, gDevice.last_resp1, gDevice.last_resp2, gDevice.last_resp3);
		
	// Set back to default
	gDevice.block_size = 512;
	
	// ACMD51 is "kind" enough to give us the SCR in big endian... Swap it
	unsigned int scr0 = byte_swap(gDevice.scr.scr[0]);
	unsigned int sd_spec = (scr0 >> (56 - 32)) & 0xF;
	unsigned int sd_spec3 = (scr0 >> (47 - 32)) & 0x1;
	unsigned int sd_spec4 = (scr0 >> (42 - 32)) & 0x1;
	gDevice.scr.sd_bus_widths = (scr0 >> (48 - 32)) & 0xF;

	printf("ssed - bus widths = %d\n", gDevice.scr.sd_bus_widths);

	gDevice.scr.sd_version = SdVersionUnknown;
	if(sd_spec == 0)
		gDevice.scr.sd_version = SdVersion1;
	else if(sd_spec == 1)
		gDevice.scr.sd_version = SdVersion1_1;
	else if(sd_spec == 2)
	{
		if(sd_spec3 == 0)
			gDevice.scr.sd_version = SdVersion2;
		else if(sd_spec3 == 1)
		{
			if(sd_spec4 == 0)
				gDevice.scr.sd_version = SdVersion3;
			else if(sd_spec4 == 1)
				gDevice.scr.sd_version = SdVersion4;
		}
	}
	
    printf("ssed - SCR[0]: %d, SCR[1]: %d\n", gDevice.scr.scr[0], gDevice.scr.scr[1]);;
    printf("ssed - SCR: %d, %d\n", byte_swap(gDevice.scr.scr[0]), byte_swap(gDevice.scr.scr[1]));
    printf("ssed - SCR: version %s, bus_widths %d\n", gSdVersionStrings[gDevice.scr.sd_version], gDevice.scr.sd_bus_widths);
	
	if(gDevice.scr.sd_bus_widths & 0x4)
	{
		// Set 4-bit transfer mode (ACMD6)
		printf("ssed - Setting 4-bit data mode.\n");

		unsigned int old_interrupt_mask = gEmmc->IrptMask.raw;

		unsigned int new_interrupt_mask = old_interrupt_mask & ~(1 << 8);
		gEmmc->IrptMask.raw = new_interrupt_mask;

		if(EmmcSendCommand(ACMD[6], 0x2))
		{
			printf("ssed - failed to switch to 4-bit data mode.\n");
		}
		else
		{
			gEmmc->Control0.raw |= 0x2;

			// Reenable card interrupt in host

			gEmmc->IrptMask.raw = old_interrupt_mask;

			printf("ssed - switch to 4-bit transfer mode complete.\n");
		}
	}

	printf("ssed - Found a valid %s SD card.\n", gSdVersionStrings[gDevice.scr.sd_version]);

	gEmmc->Interrupt.raw = 0xFFFFFFFF;

	printf("ssed - Initialization complete!\n");

	return 0;
}

unsigned int EmmcSendCommand(unsigned int cmd, unsigned int argument)
{
	gLastCommand = 0;
	gLastCommandSuccess = 0;

	// Wait for command line to become available
	while(gEmmc->Status.bits.CmdInhibit == 1)
		wait(1);

	// If the response type is "With busy" and the command is not an abort command
	// We have to wait for the data line to become available before sending the command
	if((cmd & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B && (cmd & SD_CMD_TYPE_ABORT) != SD_CMD_TYPE_ABORT)
	{	
		while(gEmmc->Status.bits.DatInhibit != 0)
			wait(1);
	}
	
	if(gDevice.blocks_to_transfer > 0xFFFF)
	{
		printf("ssed - blocks_too_transfer is too large %d\n", gDevice.blocks_to_transfer);
		gLastCommandSuccess = 0;
		return -1;
	}

	// Write block size count to register
	gEmmc->BlockCountSize.bits.BlkCnt = gDevice.blocks_to_transfer;
	gEmmc->BlockCountSize.bits.BlkSize = gDevice.block_size;

	unsigned int fixed_cmd;
	if(cmd & IS_APP_CMD)
	{ 		
		fixed_cmd = cmd - IS_APP_CMD;
		fixed_cmd |= 0x0 << 22; // Normal command type

		gLastCommand = fixed_cmd;

		unsigned int rca = 0;
		if(gDevice.rca)
			rca = gDevice.rca << 16;
		
		if(!EmmcSendCommand(CMD[55], rca))
			return -1;

		wait(50);
	}
	else
	{
		gLastCommand = cmd;
		fixed_cmd = cmd;
	}
	
	// TODO: Handle DMA

	
	// Set arg1
	gEmmc->Arg1 = argument;

	// Finally - Write the command
	gEmmc->Cmdtm.raw = fixed_cmd;

	// Just relax for a bit, get a drink or something
	wait(2); 
	
	// Wait for the command done flag (or error :()
	while(gEmmc->Interrupt.bits.cmd_done == 0 && gEmmc->Interrupt.bits.err == 0)
		wait(20);

	unsigned int interrupt_reg = gEmmc->Interrupt.raw;

	// Clear the command complete status interrupt
	gEmmc->Interrupt.raw = 0xffff0001; // Clear "Command finished" and all status bits

	// Now check if we encountered an error sending our command
	if((interrupt_reg & 0xffff0001) != 0x1)
	{
		gLastError = interrupt_reg & 0xFFFF0000;

		PrintErrorsInInterruptRegister(interrupt_reg);
		return -1;
	}

	wait(2);

	switch(cmd & SD_CMD_RSPNS_TYPE_MASK)
	{
		case SD_CMD_RSPNS_TYPE_NONE:
			gEmmc->Resp0 = 0;
			gEmmc->Resp1 = 0;
			gEmmc->Resp2 = 0;
			gEmmc->Resp3 = 0;
			break;
		case SD_CMD_RSPNS_TYPE_48:
		case SD_CMD_RSPNS_TYPE_48B:
			gDevice.last_resp0 = gEmmc->Resp0;
			gEmmc->Resp0 = 0;
			break;
		case SD_CMD_RSPNS_TYPE_136:
			gDevice.last_resp0 = gEmmc->Resp0;
			gDevice.last_resp1 = gEmmc->Resp1;
			gDevice.last_resp2 = gEmmc->Resp2;
			gDevice.last_resp3 = gEmmc->Resp3;
			break;
	}

	if(cmd & SD_CMD_ISDATA)
	{
		unsigned int irpt;
		unsigned int is_write = 0;
		if(cmd & SD_CMD_DAT_DIR_CH)
			irpt = (1 << 5); // Read
		else
		{
			is_write = 1;
			irpt = (1 << 4);
		}

		unsigned int current_block = 0;
		unsigned int* bufferAddress = gDevice.receive_buffer;

		while(current_block < gDevice.blocks_to_transfer)
		{
			if(gDevice.blocks_to_transfer > 1)
				printf("ssed - send - Waiting for block %d to get ready.\n", current_block);

			// Wait for block
			while(!(gEmmc->Interrupt.raw & (irpt | 0x8000)));
			interrupt_reg = gEmmc->Interrupt.raw;

			gEmmc->Interrupt.raw = 0xFFFF0000 | irpt;

			if((interrupt_reg & (0xFFFF0000 | irpt)) != irpt)
			{
				gLastError = interrupt_reg & 0xFFFF0000;
				printf("ssed - send - Error occurred whilst waiting for data ready interrupt.\n");
				return -1; // sadness, failed to read
			}

			// Transfer the block
			unsigned int current_byte_number = 0;
			while(current_byte_number < gDevice.block_size)
			{
				if(is_write)
				{
					gEmmc->Data = read_word((unsigned short*)bufferAddress, 0);
				}
				else
				{
					write_word(gEmmc->Data, (unsigned short*)bufferAddress, 0);
				}
				current_byte_number += 4;
				bufferAddress++;
			}

			printf("ssed - send - Block transfer complete.\n");

			current_block ++;
		}
	}

	// Wait for transfer complete (set if read/write transfer with busy)
	if((((cmd & SD_CMD_RSPNS_TYPE_MASK) == SD_CMD_RSPNS_TYPE_48B) ||(cmd & SD_CMD_ISDATA)))
	{
		// First make sure DAT Is not already 0
		if(gEmmc->Status.bits.DatInhibit == 0)
			gEmmc->Interrupt.raw = 0xFFFF0002;
		else
		{
			// Wait for the interrupt
			printf("ssed - send - Waiting for transfer complete interrupt.\n");
			while(!(gEmmc->Interrupt.raw & 0x8002));
			printf("ssed - send - Got transfer complete interrupt!\n");

			interrupt_reg = gEmmc->Interrupt.raw;

			gEmmc->Interrupt.raw = 0xFFFF0002;

			if(((interrupt_reg & 0xFFFF0002) != 0x2) && ((interrupt_reg & 0xFFFF0002) != 0x100002))
			{
				printf("ssed - send - Error occured whilst waiting for transfer complete interrupt.\n");
				
				gLastError = interrupt_reg & 0xFFFF0000;

				return -1;
			}

			gEmmc->Interrupt.raw = 0xFFFF0002;
		}
	}

	// If the command was with data, wait for the appropriate interrupt
	if((cmd & SD_RESP_R1b) == SD_RESP_R1b || (cmd & SD_CMD_ISDATA))
	{
		// Check if DAT is not already 0
		if(gEmmc->Status.bits.DatInhibit == 0)
		{
			// Write 0xffff0002 to interrupt? :S Clear it again I guess?
		}
		else
		{
			printf("ssed - Reading busy data command and stuff...\n");

			while(gEmmc->Interrupt.bits.cmd_done != 0 || gEmmc->Interrupt.bits.err == 0)
				wait(20);
			
			interrupt_reg = gEmmc->Interrupt.raw;
			gEmmc->Interrupt.raw = 0xFFFF0002;

			if((((interrupt_reg & 0xFFFF0002) != 0x2) && (interrupt_reg & 0xFFFF0002) != 0x100002))
			{
				printf("ssed - Error occurred whilst waiting for transfer complete interrupt! :-(\n");
				return -1;
			}

			gEmmc->Interrupt.raw = 0xFFFF0002;
		}
	}
	else if(0 == 1) // is_dma
	{
		// For SDMA transfers, we have to wait for either transfer complete,
        //  DMA int or an error
	}
	
	// Success!
	return 1;	
}

unsigned int EmmcPowerOn(void)
{
	return Mailbox_SetDevicePowerState(0x0, 1);
}

unsigned int EmmcPowerOff(void)
{
	return Mailbox_SetDevicePowerState(0x0, 0);
}

unsigned int EmmcPowerCycle(void)
{
	printf("ssed - Power cycling: ");
	
	unsigned int res = 0;
	if((res = EmmcPowerOff()) < 0)
	{
		printf("Failed!\n");
		return -1;
	}	
	
	wait(50);
	
	if((res = EmmcPowerOn()) < 0)
		printf("Failed!\n");
	else
		printf("Success!\n");
	
	return EmmcPowerOn();
}

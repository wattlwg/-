//-----------------------------------------------------------------------------
// EFM8BB2_UART0_Interrupt.c
//-----------------------------------------------------------------------------
// Copyright 2014 Silicon Laboratories, Inc.
// http://developer.silabs.com/legal/version/v11/Silicon_Labs_Software_License_Agreement.txt
//
// Program Description:
//
// This program demonstrates how to configure the EFM8BB2 to write to and
// read from the UART0 interface. The program reads a word using the UART0
// interrupt and outputs that word to the screen, with all characters in
// uppercase.
//
// Resources:
//   SYSCLK - 24.5 MHz HFOSC0 / 1
//   UART0  - 57600 baud, 8-N-1
//   Timer1 - UART0 clock source
//   P0.4   - UART0 TX
//   P0.5   - UART0 RX
//   P1.6   - RESET CODEC
//   P1.0   - CODEC I2C SDA  
//   P1.1   - CODEC I2C SCL
//   P2.3   - Display enable
//
//-----------------------------------------------------------------------------
// How To Test: EFM8BB2 STK
//-----------------------------------------------------------------------------
// 1) Place the switch in "AEM" mode.
// 2) Connect the EFM8BB2 STK board to a PC using a mini USB cable.
// 3) Compile and download code to the EFM8BB2 STK board.
//    In Simplicity Studio IDE, select Run -> Debug from the menu bar,
//    click the Debug button in the quick menu, or press F11.
// 4) On the PC, open HyperTerminal (or any other terminal program) and connect
//    to the JLink CDC UART Port at 115200 baud rate and 8-N-1.
// 5) Run the code.
//    In Simplicity Studio IDE, select Run -> Resume from the menu bar,
//    click the Resume button in the quick menu, or press F8.
// 6) Using a terminal program on the PC, input any number of lower-case
//    characters, up to UART_BUFFERSIZE (default 64), followed by either
//    a carriage return ('\r'), a newline character ('\n'), or a tilda ('~').
//    The program will change the input characters to upper-case and output
//    them over UART.
//
// Target:         EFM8BB2
// Tool chain:     Generic
//
// Release 0.1 (ST)
//    - Initial Revision
//    - 10 OCT 2014
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB2_Register_Enums.h>
#include "InitDevice.h"
#include "adc_0.h"
#include "serial.h"
#include "version.h"
#include "packet.h"

/*#define LAM_PKT_MAX_SZ 132

#define LAM_PKT_TAG_OFFS	0
#define LAM_PKT_LEN_OFFS	1
#define LAM_PKT_TYPE_OFFS	2
#define LAM_PKT_DATA_OFFS	3

// LAM protocol packet types
#define ACC_READY	1
#define LAM_READY	2
#define LAM_INFO	0x10
#define APP_DATA	0x12
#define ACC_DATA	0x20
#define ACC_REQUEST_APP_LAUNCH 0x22

// accessory protocol packet types
#define ACC_PROTOCOL_STATUS			1
#define ACC_PROTOCOL_LOAD_FW		2
#define ACC_PROTOCOL_FLASH_BLK	3
#define ACC_PROTOCOL_BOOT_FW		4
#define ACC_PROTOCOL_CANCEL			5

#define STATUS_SUCCESS					0
#define STATUS_FW_IS_RUNNING		1
#define STATUS_WAITING_FW_DATA	2
#define STATUS_FAILED						3

// failed status details
#define FAILED_FLASH		1
#define FAILED_CHECKSUM	2
#define FAILED_LENGTH		3
#define FAILED_REQUEST	4
#define FAILED_TIMEOUT	5

#define WAIT_NEXT_PACKET	0
#define LAST_PACKET				1

uint8_t lam_ready;
uint8_t app_ready;
uint8_t flashing_in_progress;
uint16_t wait_unit;
uint8_t wait_cnt;
uint8_t SI_SEG_XDATA  lam_packet[LAM_PKT_MAX_SZ] _at_ 0x500;*/

extern void delay_us(unsigned int delay);
extern void delay_ms(unsigned int mdelay);
extern void codec_init(void);
extern void hs_manual(void);
extern void hs_type_det(void);
extern void ADC_pwr_up(void);
extern void ADC_pwr_dn(void);
extern void codec_pwr_up(void);
extern void enable_mic(void);
extern void disable_mic(void);
extern void codec_pwr_dn(void);
#define uchar unsigned char
#define VREF_MV         (3300UL)
#define ADC_MAX_RESULT  ((1 << 10)-1) // 10 bit ADC

extern bit  I2C_WriteRegister(uchar sla,uchar reg,uchar c);
extern uint8_t next_plug_state(uint8_t current_plug_state);
volatile uint8_t rx_write_index = 0;					// global UART receive ring buffer write index
uint8_t rx_read_index = 0;
//uint8_t SI_SEG_XDATA packet_payload[MAX_PACKET_LENGTH - 4];
volatile uint8_t SI_SEG_XDATA rx_buffer[MAX_UART_LENGTH] _at_ 0x00;	// global UART receive ring buffer //XRAM 2048bytes

uint8_t VolumeDownFlag = 0;
uint8_t VolumeUpFlag = 0;
uint8_t LAM_busy;									// global LAM busy flag
uint8_t hptype_on=0;
uint8_t EA_open;									// global EA session open flag
uint8_t pending_launch;						// global flag indicating need to request launch
uint8_t DAC_EN;								  // global flag indicating DAC enabled
uint8_t ADC_EN;									// global flag indicating ADC enabled
uint8_t LED_ON;									// global flag indicating LED is off for low power mode
uint8_t WAITING;								// global flag indicating 1 sec timer for SDA

uint8_t pending_en_ch;						// global flag indicating need to change enable state
uint8_t Tip_plugged;	  					// global flag indicating status of 3.5mm jack
uint8_t HP_Only;								  // global flag indicating headphone only (no mic)
uint8_t Optical_Only;						  // global flag indicating optical only (no capability)

#define FLASH_PAGESIZE         512
#define START_ADDRESS          0x1e00L
#define FLASH_TEMP             0x2000L 
SI_LOCATED_VARIABLE_NO_INIT(flash_write_array[512], uint8_t, const SI_SEG_CODE, START_ADDRESS);
SI_LOCATED_VARIABLE_NO_INIT(flash_write_temp[512], uint8_t, const SI_SEG_CODE, FLASH_TEMP);
const uint8_t code FW_Version[3] = {FW_MAJOR_VER, FW_MINOR_VER, FW_REVISION}; //literal address 0x9FC0//@eeprom
//uint8_t code LAM_Configured = 0;//@eeprom
uint8_t LAM_Config = 0x00;
uint8_t Active_Config;
volatile uint8_t timer2_count;						// global TIM2 count divider
volatile uint16_t TIM4_tout;						// global TIM4 tick counter (from AN3281)
typedef uint16_t FLADDR;

void FLASH_PageErase (FLADDR addr)
{
   bool EA_SAVE = IE_EA;                // Preserve IE_EA
   SI_VARIABLE_SEGMENT_POINTER(pwrite, uint8_t, SI_SEG_XDATA); // Flash write pointer

   IE_EA = 0;                          // Disable interrupts

  // VDM0CN = 0x80;                      // Enable VDD monitor

  // RSTSRC = 0x02;                      // Enable VDD monitor as a reset source

   pwrite = (SI_VARIABLE_SEGMENT_POINTER(, uint8_t, SI_SEG_XDATA)) addr;

   FLKEY  = 0xA5;                      // Key Sequence 1
   FLKEY  = 0xF1;                      // Key Sequence 2
   PSCTL |= 0x03;                      // PSWE = 1; PSEE = 1

  // VDM0CN = 0x80;                      // Enable VDD monitor

  // RSTSRC = 0x02;                      // Enable VDD monitor as a reset source
   *pwrite = 0;                        // Initiate page erase

   PSCTL &= ~0x03;                     // PSWE = 0; PSEE = 0

   IE_EA = EA_SAVE;                    // Restore interrupts
}


void FLASH_ByteWrite(FLADDR addr, uint8_t byte)
{
   bool EA_SAVE = IE_EA;                // Preserve IE_EA
   SI_VARIABLE_SEGMENT_POINTER(pwrite, uint8_t, SI_SEG_XDATA); // Flash write pointer

   IE_EA = 0;                          // Disable interrupts

 //  VDM0CN = 0x80;                      // Enable VDD monitor

 //  RSTSRC = 0x02;                      // Enable VDD monitor as a reset source

   pwrite = (SI_VARIABLE_SEGMENT_POINTER(, uint8_t, SI_SEG_XDATA)) addr;

   FLKEY  = 0xA5;                      // Key Sequence 1
   FLKEY  = 0xF1;                      // Key Sequence 2
   PSCTL |= 0x01;                      // PSWE = 1 which enables writes

  // VDM0CN = 0x80;                      // Enable VDD monitor

   //RSTSRC = 0x02;                      // Enable VDD monitor as a reset source
   *pwrite = byte;                     // Write the byte

   PSCTL &= ~0x01;                     // PSWE = 0 which disable writes

   IE_EA = EA_SAVE;                    // Restore interrupts
}

uint8_t FLASH_ByteRead(FLADDR addr)
{
   bool EA_SAVE = IE_EA;                // Preserve IE_EA
   SI_VARIABLE_SEGMENT_POINTER(pread, uint8_t, const SI_SEG_CODE); // Flash read pointer
   uint8_t byte;

   IE_EA = 0;                          // Disable interrupts

   pread = (SI_VARIABLE_SEGMENT_POINTER(, uint8_t, const SI_SEG_CODE)) addr;

   byte = *pread;                      // Read the byte

   IE_EA = EA_SAVE;                    // Restore interrupts

   return byte;
}
//-----------------------------------------------------------------------------
// Pin Definitions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
void FLASH_Copy (FLADDR dest, FLADDR src, uint16_t numbytes)
{
   FLADDR i;

   for (i = 0; i < numbytes; i++) {

      FLASH_ByteWrite ((FLADDR) dest+i,
                       FLASH_ByteRead((FLADDR) src+i));
   }
}

//-----------------------------------------------------------------------------
// FLASH_Clear
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   :
//   1) FLADDR addr - address of the byte to write to
//                    valid range is from 0x0000 to 0x1FFF for 8 kB devices
//                    valid range is from 0x0000 to 0x0FFF for 4 kB devices
//                    valid range is from 0x0000 to 0x07FF for 2 kB devices
//   2) uint16_t numbytes - the number of bytes to clear to 0xFF
//                    valid range is 0 to FLASH_PAGESIZE
//
// This routine erases <numbytes> starting from the flash addressed by
// <dest> by performing a read-modify-write operation using <FLASH_TEMP> as
// a temporary holding area.
// <addr> + <numbytes> must be less than the maximum flash address.
//
//-----------------------------------------------------------------------------
void FLASH_Clear (FLADDR dest, uint16_t numbytes)
{
   FLADDR dest_1_page_start;           // First address in 1st page
                                       // containing <dest>
   FLADDR dest_1_page_end;             // Last address in 1st page
                                       // containing <dest>
   FLADDR dest_2_page_start;           // First address in 2nd page
                                       // containing <dest>
   FLADDR dest_2_page_end;             // Last address in 2nd page
                                       // containing <dest>
   unsigned numbytes_remainder;        // When crossing page boundary,
                                       // number of <src> bytes on 2nd page
   unsigned FLASH_pagesize;            // Size of flash page to update

   FLADDR  wptr;                       // Write address
   FLADDR  rptr;                       // Read address

   unsigned length;

   FLASH_pagesize = FLASH_PAGESIZE;

   dest_1_page_start = dest & ~(FLASH_pagesize - 1);
   dest_1_page_end = dest_1_page_start + FLASH_pagesize - 1;
   dest_2_page_start = (dest + numbytes)  & ~(FLASH_pagesize - 1);
   dest_2_page_end = dest_2_page_start + FLASH_pagesize - 1;

   if (dest_1_page_end == dest_2_page_end) {

      // 1. Erase Scratch page
      FLASH_PageErase (FLASH_TEMP);

      // 2. Copy bytes from first byte of dest page to dest-1 to Scratch page

      wptr = FLASH_TEMP;
      rptr = dest_1_page_start;
      length = dest - dest_1_page_start;
      FLASH_Copy (wptr, rptr, length);

      // 3. Copy from (dest+numbytes) to dest_page_end to Scratch page

      wptr = FLASH_TEMP + dest - dest_1_page_start + numbytes;
      rptr = dest + numbytes;
      length = dest_1_page_end - dest - numbytes + 1;
      FLASH_Copy (wptr, rptr, length);

      // 4. Erase destination page
      FLASH_PageErase (dest_1_page_start);

      // 5. Copy Scratch page to destination page
      wptr = dest_1_page_start;
      rptr = FLASH_TEMP;
      length = FLASH_pagesize;
      FLASH_Copy (wptr, rptr, length);

   } else {                            // value crosses page boundary
      // 1. Erase Scratch page
      FLASH_PageErase (FLASH_TEMP);

      // 2. Copy bytes from first byte of dest page to dest-1 to Scratch page

      wptr = FLASH_TEMP;
      rptr = dest_1_page_start;
      length = dest - dest_1_page_start;
      FLASH_Copy (wptr, rptr, length);

      // 3. Erase destination page 1
      FLASH_PageErase (dest_1_page_start);

      // 4. Copy Scratch page to destination page 1
      wptr = dest_1_page_start;
      rptr = FLASH_TEMP;
      length = FLASH_pagesize;
      FLASH_Copy (wptr, rptr, length);

      // now handle 2nd page

      // 5. Erase Scratch page
      FLASH_PageErase (FLASH_TEMP);

      // 6. Copy bytes from numbytes remaining to dest-2_page_end to Scratch page

      numbytes_remainder = numbytes - (dest_1_page_end - dest + 1);
      wptr = FLASH_TEMP + numbytes_remainder;
      rptr = dest_2_page_start + numbytes_remainder;
      length = FLASH_pagesize - numbytes_remainder;
      FLASH_Copy (wptr, rptr, length);

      // 7. Erase destination page 2
      FLASH_PageErase (dest_2_page_start);

      // 8. Copy Scratch page to destination page 2
      wptr = dest_2_page_start;
      rptr = FLASH_TEMP;
      length = FLASH_pagesize;
      FLASH_Copy (wptr, rptr, length);
   }
}

//-----------------------------------------------------------------------------
// ADC0_convertSampleToMillivolts
//-----------------------------------------------------------------------------
//
// Converts a raw ADC sample to millivolts.
// sample - the raw sample from the ADC
//
// returns - a two byte number representing the adc sample in millivolts
//  e.g. 1.65V would equal 1650.
//
uint16_t ADC0_convertSampleToMillivolts(uint16_t sample)
{
  // The measured voltage applied to P1.7 is given by:
  //
  //                           Vref (mV)
  //   measurement (mV) =   --------------- * result (bits)
  //                       (2^10)-1 (bits)
  return ((uint32_t)sample * VREF_MV) / ADC_MAX_RESULT;
}
//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main (void)
{
	    uint8_t temp_read;								// local buffer
	    uint16_t mV;
	    uint8_t ADC_count=0;
	    uint16_t ADC0_temp=0;
	    uint32_t ADC0_sum=0;
	//    uint16_t temp_readl,temp_readh;								// local buffer
  //    uint8_t temp_count;								// local buffer
      
		uint8_t packet_index;							// receive packet position marker
		uint8_t packet_type;							// receive packet type
		uint8_t packet_length;							// receive packet length

		// _at_ 0x100;	//@near

		uint8_t current_packet;							// current packet undergoing transmission to LAM

		uint8_t plug_state; 							// headset jack flag, 1=plugged
		uint8_t button_state;							// side button flag, 1=released
		uint8_t SP_button_st;						  // spare button flag, 1=released
    /*uint16_t volumetemph1=0;
    uint16_t volumetempl1=0;
    uint16_t volumetemph2=0;
    uint16_t volumetempl2=0;
    uint16_t volumetemph3=0;
    uint16_t volumetempl3=0;
    uint16_t volumetemph4=0;
    uint16_t volumetempl4=0;
    uint16_t volumetemph5=0;
    uint16_t volumetempl5=0;
    uint16_t volumetemph6=0;
    uint16_t volumetempl6=0;
    uint16_t volumetemph7=0;
    uint16_t volumetempl7=0;
    uint16_t volumetemph8=0;
    uint16_t volumetempl8=0;
    uint16_t volumetemph9=0;
    uint16_t volumetempl9=0;
    uint16_t volumetemph10=0;
    uint16_t volumetempl10=0;*/
		// system initialization --------------------------------------------------

			packet_index = 0;							// reset packet position marker
			LAM_busy = 0;									// reset LAM busy flag
			EA_open = 0;									// reset EA session open flag
			pending_launch = 0;						// reset pending_launch flag
			ADC_EN = 0;									 // ADC has not been enabled
			DAC_EN = 0;									 // DAC has not been enabled
			LED_ON = 1;									// set flag
			WAITING = 0;									// set flag
			current_packet = accReady;						// initialize packet state machine
			button_state = 1;								// assume side button is released
			SP_button_st = 1;								// assume spare button is released
			Tip_plugged = 0;
			Optical_Only = 1;							// no capability
			HP_Only = 0;


      enter_DefaultMode_from_RESET();
    //  FLASH_ByteWrite(START_ADDRESS, 0xaa);
      LAM_Config = FLASH_ByteRead(START_ADDRESS);
      if(LAM_Config!=0x01)
      	{
      		if(LAM_Config!=0x00)
      			{
      		FLASH_PageErase(START_ADDRESS);
      		//FLASH_Clear(START_ADDRESS, 1);
      		FLASH_ByteWrite(START_ADDRESS, 0x00);
      	   }
      	}
      	else
      		{
      		}
      	LAM_Config =FLASH_ByteRead(START_ADDRESS);
      	
     
      //reset codec CS42L42
      SDA=0;
      SCL=0;
      MUTE=0;
      POWERON=1;
      HP_SWITCH=1;
      CODEC_RESET=1;
     
      CODEC_RESET=0; 
      delay_ms(100); 
      CODEC_RESET=1;
      MICSWITCH=0;   //低电平使用外部MIC，高电平用内部MIC，默认用耳机上的MIC
     // delay_ms(1000);
       

      //lam test程序
  //    lam_ready = 0;
	//    app_ready = 0;
	//    flashing_in_progress = 0;
     // lam_test();//lam 测试在开中断前完成
      
                                       
     
      
       IE_EA = 1;   //enableInterrupts();	
       IE_EX0=0;
    IE_EX1=0;  	 
      codec_init();
      
      
     if(HP_DET==1)
      {
      Tip_plugged = 1;
      
    while(1)
    {
      hs_type_det();									//initial detection if plugged at power up
      
      enable_mic();
      codec_pwr_up();
      DAC_EN = 0;	
      
      if (Optical_Only)
    	  hs_manual();

    	  delay_ms(100);
    	
    	// 
        ADC0_startConversion();
    while (!ADC0_isConversionComplete());	
    ADC0_temp=ADC0_getResult();
     // Convert sample to mV
    mV = ADC0_convertSampleToMillivolts(ADC0_temp);
    if((mV>100)&&(hptype_on==1))
    	   break;
    if(HP_DET==0) 	   
    	{
    		IE_EX0=0;
        IE_EX1=0;  	 	                      //here because it's not plugged
		    Tip_plugged = 0;							// reset flag 
		    HP_Only = 1;									// no mic
		    disable_mic();
		    codec_pwr_dn();
    		break;
    	}
    if(mV>100)
      	 {
      	 	hptype_on=2;
       	
       	 HP_SWITCH=1;
      	 	break;
     
         
         // IT01CF =   IT01CF_IN0PL__ACTIVE_HIGH | IT01CF_IN0SL__P0_0 | IT01CF_IN1PL__ACTIVE_LOW | IT01CF_IN1SL__P0_2;//
      	}
       else
       	{
       	 
      	 	hptype_on=1;
      	 	HP_SWITCH=0;
      	 
       	// IT01CF =   IT01CF_IN0PL__ACTIVE_HIGH | IT01CF_IN0SL__P0_0 | IT01CF_IN1PL__ACTIVE_LOW | IT01CF_IN1SL__P0_2;//
       	}  
    	}
    	  switch_Interrupt_from_ADC0();
    	}
      else
      	 {  
     IE_EX0=0;
     IE_EX1=0;  	 	                      //here because it's not plugged
		Tip_plugged = 0;							// reset flag 
		HP_Only = 1;									// no mic
		disable_mic();
		codec_pwr_dn();
		
	   }	
	   

   while(1)
   {
	   plug_state = next_plug_state(plug_state);
	   current_packet = next_packet (current_packet);
	 
	   	// evaluate bytes received from LAM and address potential packets
		if (rx_write_index != rx_read_index)
		{
			temp_read = rx_buffer[rx_read_index++];

			// wrap read pointer
			rx_read_index %= MAX_UART_LENGTH;
			packet_index = packet_receive(packet_index, temp_read, &packet_length, &packet_type);//, packet_payload);

		}
   }
}

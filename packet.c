#include <SI_EFM8BB2_Register_Enums.h>
#include "version.h"
#include "packet.h"

#define uchar unsigned char
#define START_ADDRESS          0x1e00L
typedef uint16_t FLADDR;

extern void FLASH_PageErase(FLADDR addr);
extern void FLASH_ByteWrite(FLADDR addr, uint8_t byte);
extern void FLASH_Clear(FLADDR dest, uint16_t numbytes);
extern uint8_t FLASH_ByteRead(FLADDR addr);
extern uint8_t LAM_Config;
extern volatile uint8_t timer2_count;
extern void TIM2_Cmd(char NewState);
extern char TIM2_GetStatus(void);
extern void codec_pwr_up(void);
extern void codec_pwr_dn(void);
extern void ADC_pwr_up(void);
extern void ADC_pwr_dn(void);
extern uint8_t LED_ON;	
extern uchar  I2C_ReadRegistermult(uchar sla,uchar reg,uchar NumByteToRead,uchar *c);
extern bit  I2C_WriteRegistermult(uchar sla,uchar reg,uchar NumByteToWrite,uchar *c);
extern bit  I2C_WriteRegister(uchar sla,uchar reg,uchar c);
uchar SI_SEG_XDATA tx_payload[MAX_PACKET_LENGTH - 4] _at_ 0x100;
uchar SI_SEG_XDATA EA_return_packet[MAX_PACKET_LENGTH - 4] _at_ 0x180;
uint8_t SI_SEG_XDATA packet_payload[MAX_PACKET_LENGTH - 4] _at_ 0x200;
void serial_print_char(uint8_t value)
{
       SBUF0=value;
		while(SCON0_TI == 0);
		  SCON0_TI=0; 
	// wait for transmit empty flag
	//while (USART_GetFlagStatus(USART_FLAG_TXE) == RESET);

	// send character
	//USART_SendData8(value);

} // --------------------------------------------------------------------------



// packet state machine traversal function ------------------------------------
uint8_t next_packet(uint8_t current_packet)
{

	static uchar pending_packet;
	uchar return_packet;
//	char snum[10];

	//@near uchar tx_payload[MAX_PACKET_LENGTH - 4];

	switch (current_packet)
	{

	case accReady:
    {
    	uchar i;
    for (i = 0; i < (MAX_PACKET_LENGTH-4); i++) packet_payload[i]=0;	
		packet_send(1, accReady);//,0);

		timer2_count = 0;				// initialize 1-sec timer
		TIM2_Cmd(1);				// start 1-sec timer

		return_packet = accSpecialInitWait;
	 }
		break;

	case accSpecialInitWait:

		// query if LAM has responded
		if (!LAM_busy)
		{

			// since LAM has responded, move forward with operation or configuration
		//	return_packet = LAM_Configured? accAudioTerminalStateInformation : accConfigurationInformation;
        //revised by watt 2017,02,17
			return_packet = LAM_Config ? accAudioTerminalStateInformation : accConfigurationInformation;

		} else {

			// since LAM has not responded, issue another accReady packet after 1 sec
			return_packet = (TIM2_GetStatus () == 1) ? accSpecialInitWait : accReady;

		} // if (!LAM_busy)

		break;

	case accConfigurationInformation:
    {
    	uchar i;
		tx_payload[0] = 0x01;			// LAM protocol version (fixed)
		tx_payload[1] = 0x01;			// accessory power mode (constant)
		tx_payload[2] = 0x01;			// I2S role (host)
		tx_payload[3] = 0x00;			// reserved (fixed)
		tx_payload[4] = 0x00;			// UART baud rate (57600 bps)
		tx_payload[5] = HW_MAJOR_VER;	// hardware major version
		tx_payload[6] = HW_MINOR_VER;	// hardware minor version
		tx_payload[7] = HW_REVISION;	// hardware revision
		tx_payload[8] = FW_MAJOR_VER;	// firmware major version
		tx_payload[9] = FW_MINOR_VER;	// firmware minor version
		tx_payload[10] = FW_REVISION;	// firmware revision

	//	pending_packet = accAudioTerminalStateInformation;
	    pending_packet = accNameInformation;
		return_packet = accSpecialConfigWait;

		//FLASH_Unlock(FLASH_MemType_Data);
		//while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET);
		LAM_Config = 1;
		FLASH_PageErase(START_ADDRESS);
	//	FLASH_Clear(START_ADDRESS, 1);
    FLASH_ByteWrite(START_ADDRESS, 0x01);
    LAM_Config=FLASH_ByteRead(START_ADDRESS);
		//FLASH_Lock(FLASH_MemType_Data);
		
    for (i = 0; i < 11; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send(12, accConfigurationInformation);//, tx_payload);
   }
		break;
  	case accNameInformation:
  		 {
    	uchar i;
      tx_payload[0]='H';
      tx_payload[1]='i';
      tx_payload[2]='F';
      tx_payload[3]='i';
      tx_payload[4]=0x20;
      tx_payload[5]='d';
      tx_payload[6]='i';
      tx_payload[7]='g';
      tx_payload[8]='i';
      tx_payload[9]='t';
      tx_payload[10]='a';
      tx_payload[11]='l';
      tx_payload[12]=0x20;
      tx_payload[13]='a';
      tx_payload[14]='u';
      tx_payload[15]='d';
      tx_payload[16]='i';
      tx_payload[17]='o';
      tx_payload[18]=0x20;
      tx_payload[19]='c';
      tx_payload[20]='o';
      tx_payload[21]='n';
      tx_payload[22]='v';
      tx_payload[23]='e';
      tx_payload[24]='r';
      tx_payload[25]='t';
      tx_payload[26]='e';
      tx_payload[27]='r';
   
     /* tx_payload[0]='C';//CRD42L42-MFi
      tx_payload[1]='R';
      tx_payload[2]='D';
      tx_payload[3]='4';
      tx_payload[4]='2';;
      tx_payload[5]='L';
      tx_payload[6]='4';
      tx_payload[7]='2';
      tx_payload[8]='-';
      tx_payload[9]='M';
      tx_payload[10]='F';
      tx_payload[11]='i';*/

//		strcpy (tx_payload, INFO_NAME);
       for (i = 0; i < 28; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (29, accNameInformation);// tx_payload);

		pending_packet = accManufacturerInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accManufacturerInformation:
    {
    	uchar i;
      tx_payload[0]='C';
      tx_payload[1]='-';
      tx_payload[2]='S';
      tx_payload[3]='m';
      tx_payload[4]='a';
      tx_payload[5]='r';
      tx_payload[6]='t';
      tx_payload[7]='l';
      tx_payload[8]='i';
      tx_payload[9]='n';
      tx_payload[10]='k';
      /*tx_payload[11]=0x20;
      tx_payload[12]='I';
      tx_payload[13]='n';
      tx_payload[14]='f';
      tx_payload[15]='o';
      tx_payload[16]='r';
      tx_payload[17]='m';
      tx_payload[18]='a';
      tx_payload[19]='t';
      tx_payload[20]='i';
      tx_payload[21]='o';
      tx_payload[22]='n';
      tx_payload[23]=0x20;
      tx_payload[24]='T';
      tx_payload[25]='e';
      tx_payload[26]='c';
      tx_payload[27]='h';
      tx_payload[28]='n';
      tx_payload[29]='o';
      tx_payload[30]='l';
      tx_payload[31]='o';
      tx_payload[32]='g';
      tx_payload[33]='y';*/
      /*tx_payload[0]='C';//Cirrus Logic
      tx_payload[1]='i';
      tx_payload[2]='r';
      tx_payload[3]='r';
      tx_payload[4]='u';
      tx_payload[5]='s';
      tx_payload[6]=0x20;
      tx_payload[7]='L';
      tx_payload[8]='o';
      tx_payload[9]='g';
      tx_payload[10]='i';
      tx_payload[11]='c';*/
      
		//strcpy (tx_payload, INFO_MANF);
     for (i = 0; i < 11; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (12, accManufacturerInformation);//, tx_payload);

		pending_packet = accModelInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accModelInformation:
    {
    	uchar i;
      tx_payload[0]='C';
      tx_payload[1]='S';
      tx_payload[2]='4';
      tx_payload[3]='2';
      tx_payload[4]='L';
      tx_payload[5]='4';
      tx_payload[6]='2';
     // tx_payload[7]='1';
     for (i = 0; i < 7; i++) 
    packet_payload[i]=tx_payload[i];	
		//strcpy (tx_payload, INFO_MODEL);

		packet_send (8, accModelInformation);//, tx_payload);

		pending_packet = accSerialNumberInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accSerialNumberInformation:
    {
    	uchar i;
    	tx_payload[0]='a';
      tx_payload[1]='7';
      tx_payload[2]='6';
      tx_payload[3]='d';
      tx_payload[4]='e';
      tx_payload[5]='6';
      tx_payload[6]='6';
      tx_payload[7]='1';
     /* tx_payload[8]='l';
      tx_payload[9]='4';
      tx_payload[10]='2';
      tx_payload[11]='d';
      tx_payload[12]='c';
      
      tx_payload[13]='s';
      tx_payload[14]='1';
      tx_payload[15]='6';
      tx_payload[16]='1';
      tx_payload[17]='2';
      tx_payload[18]='0';
      tx_payload[19]='6';
      tx_payload[20]='x';
      tx_payload[21]='l';*/
     
		//sprintf(snum, "%08lX", serial_num);
		//memcpy (tx_payload, snum, 8);
     for (i = 0; i < 8; i++) 
    packet_payload[i]=tx_payload[i];	
    
		packet_send (9, accSerialNumberInformation);//, tx_payload);

		pending_packet = accPreferredAppBundleIdentifierInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accPreferredAppBundleIdentifierInformation:
   {
   	uchar i;
		//strcpy (tx_payload, INFO_PREF_APP);
      tx_payload[0]='c';
      tx_payload[1]='o';
      tx_payload[2]='m';
      tx_payload[3]='.';
      tx_payload[4]='s';
      tx_payload[5]='m';
      tx_payload[6]='a';
      tx_payload[7]='r';
      tx_payload[8]='t';
      tx_payload[9]='l';
      tx_payload[10]='i';
      tx_payload[11]='n';
      tx_payload[12]='k';
      tx_payload[13]='.';
      tx_payload[14]='m';
      tx_payload[15]='f';
      tx_payload[16]='i';
      
       for (i = 0; i < 17; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (18, accPreferredAppBundleIdentifierInformation);//, tx_payload);

		pending_packet = accExternalAccessoryProtocolNameInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accExternalAccessoryProtocolNameInformation:
   {
   	  uchar i;
		//strcpy (tx_payload, INFO_EA_NAME);
      tx_payload[0]='c';
      tx_payload[1]='o';
      tx_payload[2]='m';
      tx_payload[3]='.';
      tx_payload[4]='s';
      tx_payload[5]='m';
      tx_payload[6]='a';
      tx_payload[7]='r';
      tx_payload[8]='t';
      tx_payload[9]='l';
      tx_payload[10]='i';
      tx_payload[11]='n';
      tx_payload[12]='k';
      tx_payload[13]='.';
      
      tx_payload[14]='m';
      tx_payload[15]='f';
      tx_payload[16]='i';
      /*tx_payload[14]='p';
      tx_payload[15]='r';
      tx_payload[16]='o';
      tx_payload[17]='t';
      tx_payload[18]='o';
      tx_payload[19]='c';
      tx_payload[20]='o';
      tx_payload[21]='l';*/
      
      for (i = 0; i < 17; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (18, accExternalAccessoryProtocolNameInformation);//, tx_payload);

		pending_packet = accHIDComponentInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accHIDComponentInformation:
    {
    	uchar i;
		tx_payload[0] = 0x08;			// fixed
		tx_payload[1] = 0x00;			// null (not supported)
		tx_payload[2] = 0x00;			// null (not supported)

       for (i = 0; i < 3; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (4, accHIDComponentInformation);//, tx_payload);

		pending_packet = accAudioTerminalInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accAudioTerminalInformation:
   {
   	uchar i;
   	
		tx_payload[0] = 0x02;			// audio output terminal type (jack)
		//tx_payload[0] = 0x01;			// audio output terminal type (headset)
		tx_payload[1] = 0x00;			// audio output terminal latency [15:8]
		tx_payload[2] = 0x04;			// audio output terminal latency [7:0] (4.2 samples)
		tx_payload[3] = 0x01;			// audio output terminal gain/attn. step size (1.0 dB)
		tx_payload[4] = 0x00;			// audio output terminal gain steps 0 indicates 1, cannot be set to zero
		tx_payload[5] = 0x3D;			// audio output terminal attn. steps (62-1) 
		//tx_payload[6] = 0x00;			// audio input terminal type (no microphone)
		//tx_payload[6] = 0x01;			// audio input terminal type (headset microphone)
		tx_payload[6] = 0x02;			// audio input terminal type (headset jack)
		tx_payload[7] = 0x00;			// audio input terminal latency [15:8]
		tx_payload[8] = 0x05;			// audio input terminal latency [7:0] (4.6 samples)
		tx_payload[9] = 0x00;			// audio input terminal gain/attn. step size (not available)
		tx_payload[10] = 0x00;			// audio input terminal gain steps - 1
		tx_payload[11] = 0x00;			// audio input terminal attn. steps - 1

		tx_payload[12] = 0x00;			// preferred audio processing (none)

       for (i = 0; i < 13; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (14, accAudioTerminalInformation);//, tx_payload);

		pending_packet = accAudioTerminalStateInformation;
		return_packet = accSpecialConfigWait;
	}
		break;
	case accAudioTerminalStateInformation:
    {
    	uchar i;
		//tx_payload[0] = 0x00;	// audio output terminal state (disabled)
	//	tx_payload[1] = 0x00;	// audio input terminal state (disabled)
		if (!Tip_plugged ) //| Optical_Only  //revised by watt 2017,02,21
			{
				tx_payload[0] = 0x00;
				}	// audio output terminal state (disabled)
   	else 
   		{tx_payload[0] = 0x01;
   			}							// audio output terminal state (enabled)
		
		if (HP_Only | Optical_Only) {tx_payload[1] = 0x00;}	// audio input terminal state (disabled)
		else {tx_payload[1] = 0x01;}							// audio input terminal state (enabled)
    for (i = 0; i < 2; i++) 
    packet_payload[i]=tx_payload[i];	
		packet_send (3, accAudioTerminalStateInformation);//, tx_payload);
		pending_en_ch = 0;						// reset pending_launch flag

		return_packet = accSpecialOperational;
	 }
		break;
  	case accVersionInformation:			// not used in current LAM

		//for (i = 0; i < 33; i++) wait_15ms ();			//**DEBUG Only

		packet_send (1, accVersionInformation);//, 0);

		return_packet = accSpecialOperational;
		break;
	case accRequestAppLaunch:			// request device to launch app

		packet_send (1, accRequestAppLaunch);//, 0);
		pending_launch = 0;						// reset pending_launch flag
																	//we are not testing that device acknowledged
		return_packet = accSpecialOperational;
		break;

	case accSpecialOperational:
		// LAM sets SDA low when it's time to go to sleep. 
		// Turning off the LED conserves power.
	//	GPIO_ResetBits (GPIOB, GPIO_Pin_2);		// MCU_IO2 set low **DEBUG Only

		//if (GPIO_ReadInputDataBit (GPIOC, GPIO_Pin_4) == RESET)
		if(LAMMUTE==0)
		{ //SDA=0
			if (LED_ON){																					 //LED still ON
				if (!WAITING){																			 //timer not started
					WAITING = 1;						//start timer and start waiting
				//	GPIO_ResetBits (GPIOB, GPIO_Pin_3);				//##DEBUG Only

					timer2_count = 50;				// initialize 1-sec timer for half sec
					TIM2_Cmd (1);				// start 1-sec timer
				}
		else {
						if (TIM2_GetStatus () != 1) {
							LED_ON = 0;					//timer is done so turn LED off
							WAITING = 0;						//no longer waiting
							//Tip_plugged = 0;				// required to start up after sleep 
							// pendant board
							//GPIO_ResetBits (GPIOB, GPIO_Pin_6);		// green OFF
							//GPIO_ResetBits (GPIOB, GPIO_Pin_5);		// red OFF
							//halt();                               // reduce power when SDA pad low
							while(1);
						}
					}
			}
		}
		else {
			if (!LED_ON){
				//GPIO_SetBits (GPIOB, GPIO_Pin_5);		// red ON
				LED_ON = 1;
			}			
		}		
	//	GPIO_SetBits (GPIOB, GPIO_Pin_2);	*/	// MCU_IO2 set low **DEBUG Only

		return_packet = accSpecialOperational;
		if (pending_launch)return_packet = accRequestAppLaunch;
		if (pending_en_ch) return_packet = accAudioTerminalStateInformation;
		break;

	case accSpecialConfigWait:

		return_packet = LAM_busy ? accSpecialConfigWait : pending_packet;
		break;

	case accSpecialUnknown:

		return_packet = accSpecialUnknown;
		break;

	default:

		return_packet = accSpecialUnknown;
        break;
	} // switch (current_packet);

	return (return_packet);
//	return 0;

} // --------------------------------------------------------------------------


// packet checksum calculator -------------------------------------------------
char packet_checksum (uchar packet_length, uchar packet_type)//, uchar *packet_payload)
{

	char temp_sum;
	uchar i;

	temp_sum = (char)packet_length + (char)packet_type;

	for (i = 0; i < (packet_length - 1); i++) temp_sum += (char)packet_payload[i];

	return (-temp_sum);

} // --------------------------------------------------------------------------


// receive packet processing function -----------------------------------------
void packet_process (uchar packet_length, uchar packet_type)// uchar *packet_payload)
{



	// take action based on received packet type
	switch (packet_type)
	{

	case lamReady:

		LAM_busy = 0;
		break;

	case lamInformation:

		// update EA session status
		EA_open = packet_payload[0];

		// update playback indicator LEDs
		if (packet_payload[3] == 0x01)				// playing
		{

			// issue codec initialization if necessary
			if (!DAC_EN)
			{
			//	GPIO_SetBits (GPIOB, GPIO_Pin_6);		// green ON**DEBUG Only

				//codec_pwr_up();
			
				MUTE=1;
				// pendant board
			//	GPIO_ResetBits (GPIOB, GPIO_Pin_5);		// red OFF
				//GPIO_SetBits (GPIOB, GPIO_Pin_6);		// green ON

				
				//if (GPIO_ReadInputDataBit (GPIOD, GPIO_Pin_0) == RESET)
				/*	{
						// enable mic test mode for loopback test
						// ADC_test_config
						I2C_WriteRegister(0x90,0x00,0x1D);					// set page 0x1D
						I2C_WriteRegister(0x90,0x03,0x06);					//ADC_VOL = 6dB,
						I2C_WriteRegister(0x90,0x01,0x00);					//no ADC_DIG_BOOST
						I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
						I2C_WriteRegister(0x90,0x02,0xFF);					//no sidetone
						while (1){							//**DEBUG
						}	
					}*/
			} 

		} else {									// not playing
			if (DAC_EN)
			{

				//codec_pwr_dn();
				MUTE=0;
				// pendant board
				//GPIO_ResetBits (GPIOB, GPIO_Pin_6);		// green OFF
				//GPIO_SetBits (GPIOB, GPIO_Pin_5);		// red ON
	
		}
		} // if (playing)

		if (packet_payload[4] == 0x01)	// iOS requests record
		{
			if (!ADC_EN)
			{
				//hs_type_det (); 
				ADC_pwr_up ();
			}

		} else {									// not recording
			if (ADC_EN)
			{
				ADC_pwr_dn ();						//ADC has now been turned off
			}

		} // if (recording)


		break;

	case lamAudioTerminalInformation:

		// apply mute or adjust playback volume 
		if (packet_payload[0])
		{
				//DAC_mute
				I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
				I2C_WriteRegister(0x90,0x01,0xFF);					//CHA_Vol = MUTE
				I2C_WriteRegister(0x90,0x03,0xFF);					//CHB_Vol = MUTE
		}	else	{
				//DAC_volume_A_B
				I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
				I2C_WriteRegister(0x90,0x01,(0 - packet_payload[1]));
				I2C_WriteRegister(0x90,0x03,(0 - packet_payload[1]));
		}			
		break;

	case lamExternalAccessoryProtocolSessionData:
    {	uchar i;
		// verify that EA session is open and length is valid
		if (EA_open) {
			if ((packet_payload[0] == EA_STATUS) && (packet_length == 2)) {
				EA_return_packet[0] = EA_STATUS;				// start of packet
				EA_return_packet[1] = EA_STATUS_FW_RUNNING;
				EA_return_packet[2] = 0x01;//FW_Version[0];
				EA_return_packet[3] = 0x00;//FW_Version[1];
				EA_return_packet[4] = 0x00;//FW_Version[2];
				
				for (i = 0; i < 5; i++) 
    packet_payload[i]=EA_return_packet[i];	
				packet_send(6,accExternalAccessoryProtocolSessionData);//,EA_return_packet);
			} else if ((packet_payload[0] == EA_LOAD_FW) && (packet_length == 2)) {
			//	_asm("JPF $8000"); 
			*((uint8_t SI_SEG_DATA *)0x00) = 0xA5;
      RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
			} else if ((((packet_payload[0] == EA_WRITE) && (packet_length > 5) && (packet_length == (packet_payload[3] + 5))) || ((packet_payload[0] == EA_READ) && (packet_length == 5))) && (packet_payload[3] > 0) && (packet_payload[3] < 125)) {

				// issue write if necessary
				if (packet_payload[0] == EA_WRITE)
				{
	
					// set page
					I2C_WriteRegister(0x90,0x00,packet_payload[1]);
	
					// write data
				I2C_WriteRegistermult(0x90,packet_payload[2], packet_payload[3], &packet_payload[4]);
	
				} // if (write)
	
				// populate return packet header
				EA_return_packet[0] = EA_READ;				// start of packet
				EA_return_packet[1] = packet_payload[1];	// page
				EA_return_packet[2] = packet_payload[2];	// address
	
				// return blank packet for page 0xBD, or read packet for all others
				if ((packet_payload[0] == EA_WRITE) && (packet_payload[1] == 0xBD))
				{
	
					// return zero length
					EA_return_packet[3] = 0x00;
	
				} else {
	
					// return matching length
					EA_return_packet[3] = packet_payload[3];
	
					// set page
					I2C_WriteRegister(0x90,0x00,packet_payload[1]);
	
					// read data
					I2C_ReadRegistermult(0x90,packet_payload[2],packet_payload[3],&EA_return_packet[4]);
	
				} // if (page 0xBD special case)
	
				// send return packet
				for (i = 0; i < EA_return_packet[3]+4; i++) 
          packet_payload[i]=EA_return_packet[i];	
				packet_send((EA_return_packet[3] + 5),accExternalAccessoryProtocolSessionData);//, EA_return_packet);
	
				// return to user space
				I2C_WriteRegister(0x90,0x00, 0x00);
	
			} else { //app waits for some responce
				EA_return_packet[0] = EA_STATUS;
				EA_return_packet[1] = EA_STATUS_FAILED;
				EA_return_packet[2] = EA_FAILED_REQUEST;
				EA_return_packet[3] = packet_payload[0];	// invalid data tag
				for (i = 0; i < 4; i++) 
          packet_payload[i]=EA_return_packet[i];	
				packet_send (5,accExternalAccessoryProtocolSessionData);//, EA_return_packet);
			}
		}
	}
		break;

	case lamHIDReport:

		/* not supported, do nothing */
		break;

	default:

		/* unrecognized packet, do nothing */
		;

	} // switch (packet_type)

} // --------------------------------------------------------------------------


// packet transmit function ---------------------------------------------------
void packet_send(uchar packet_length,uchar packet_type)//,uchar *packet_payload)
{

	uchar i;

	// set LAM busy flag for initialization and configuration packets
	if (packet_type < lamInformation) LAM_busy = 1;
	

	// send packet header
	serial_print_char(START_OF_PACKET);
	serial_print_char(packet_length);
	serial_print_char(packet_type);

	// send packet payload
	for (i = 0; i < (packet_length - 1); i++) serial_print_char(packet_payload[i]);

	// send checksum
	serial_print_char((uchar)packet_checksum (packet_length,packet_type));//,packet_payload));

} // --------------------------------------------------------------------------


// packet receive function ----------------------------------------------------
uchar packet_receive(uchar packet_index,uchar packet_byte,uchar *packet_length,uchar *packet_type)//,uchar *packet_payload)
{

	uchar temp_index;
	temp_index = packet_index;

	// parse packets received from LAM
	switch (temp_index)
	{

	case 0:			// looking for start of packet

		if (packet_byte == START_OF_PACKET) temp_index++;
		break;

	case 1:			// expect next byte to be length field

		*packet_length = packet_byte;
		temp_index++;
		break;

	case 2:			// expect next byte to be type field

		*packet_type = packet_byte;
		temp_index++;
		break;

	default:		// all other bytes could be payload or checksum

		// look for checksum or invalid length, or simply record byte
		if (temp_index == (*packet_length + 2))
		{

			// process packet if checksum was valid
			if (packet_checksum (*packet_length, *packet_type) == (char)packet_byte) packet_process (*packet_length, *packet_type);//, packet_payload

			// reset location marker whether packet was valid or not
			temp_index = 0;

		} else if (temp_index >= MAX_PACKET_LENGTH) {

			// reset location marker (presumably off in the weeds)
			temp_index = 0;

		} else {

			// record byte and increment location marker
			packet_payload[(temp_index++) - 3] = packet_byte;

		} // if (temp_index)

	} // switch (temp_index)

	// report updated location marker
	return (temp_index);

} // --------------------------------------------------------------------------

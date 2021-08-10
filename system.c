#include <SI_EFM8BB2_Register_Enums.h>
#include "InitDevice.h"
#include "serial.h"
#include "adc_0.h"

#define uchar unsigned char
extern bit  I2C_WriteRegister(uchar sla,uchar reg,uchar c);
extern uchar  I2C_ReadRegister(uchar sla,uchar reg);
extern void delay_ms(unsigned int mdelay);
extern uint16_t ADC0_convertSampleToMillivolts(uint16_t sample);
extern uchar Optical_Only;
extern uint8_t pending_en_ch;
extern uchar HP_Only;
extern uint8_t hptype_on;
extern volatile uint8_t SI_SEG_XDATA rx_buffer[MAX_UART_LENGTH];
extern uint8_t Tip_plugged;
extern volatile uint8_t timer2_count;
extern volatile uint8_t rx_write_index;
//extern uchar hs_type_rd;
extern uchar DAC_EN;								  // global flag indicating DAC enabled
extern uchar ADC_EN;									// global flag indicating ADC enabled
// 15-ms delay function -------------------------------------------------------
void wait_15ms(void)
{
delay_ms(15);

	//TIM3_Cmd(ENABLE);								// enable TIM3

	// wait for TIM3 to count
	//while (TIM3_GetFlagStatus (TIM3_FLAG_Update) == RESET);

	//TIM3_Cmd(DISABLE);								// disable TIM3

	//TIM3_ClearFlag (TIM3_FLAG_Update);				// clear update flag

} // --------------------------------------------------------------------------
char TIM2_GetStatus(void)
{
  //return ((FunctionalState)(TIM2->CR1 & TIM_CR1_CEN));
  if((TMR2CN0 & TMR2CN0_TR2__RUN)==0)
  	   return 0;
  else
  	   return 1;	   
}
void TIM2_Cmd(char NewState)
{
  
  /* set or Reset the CEN Bit */
  if (NewState != 0)
  {
   // TIM2->CR1 |= TIM_CR1_CEN;
   TMR2CN0 |= TMR2CN0_TR2__RUN;
  }
  else
  {
    TMR2CN0 &= (uint8_t)(~TMR2CN0_TR2__RUN);
  }
}

void serial_service_rx (void)
{

	// capture received byte and increment write pointer
	rx_buffer[rx_write_index++] = SBUF0;   //USART_ReceiveData8 ();

	// wrap write pointer
	rx_write_index %= MAX_UART_LENGTH;

} // --------------------------------------------------------------------------
// timer 2 overflow interrupt handler -----------------------------------------
void timer2_service(void)
{

	// divide 10-ms timer into one-shot 1-sec timer
	if (timer2_count < 100) timer2_count++; else TIM2_Cmd(0);

	//TIM2_ClearITPendingBit (TIM2_IT_Update);		// clear interrupt flag

} // --------------------------------------------------------------------------


// codec power up function ----------------------------------------------
void codec_pwr_up(void)
{
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x02,0xA4);					//Release Discharge cap, FILT+
	I2C_WriteRegister(0x90,0x02,0xA7);					//Power Down Control 2
	I2C_WriteRegister(0x90,0x01,0x00);					//power up ASP, Mixer, HP, and DAC
	I2C_WriteRegister(0x90,0x00,0x12);					// set page 0x12
	I2C_WriteRegister(0x90,0x07,0x2C);					//Enable ASP SCLK, LRCK input, SCPOL_IN (ADC and DAC) inverted
	I2C_WriteRegister(0x90,0x00,0x2A);					// set page 0x2A
	I2C_WriteRegister(0x90,0x01,0x0C);					//Enable Ch1/2 DAI
	I2C_WriteRegister(0x90,0x00,0x20);					// set page 0x20
	I2C_WriteRegister(0x90,0x01,0x01);					//unmute analog A and B, not -6dB mode
	DAC_EN = 1;	    											// reset flag
	
} // --------------------------------------------------------------------------

// codec power down function ----------------------------------------------
void codec_pwr_dn(void)
{
// power-down sequence for CS42L42

	I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
	I2C_WriteRegister(0x90,0x01,0x3F);					//CHA_Vol = MUTE
	I2C_WriteRegister(0x90,0x03,0x3F);					//CHB_Vol = MUTE
	I2C_WriteRegister(0x90,0x02,0xFF);					//Mixer ADC Input = MUTE
	I2C_WriteRegister(0x90,0x00,0x20);					// set page 0x20
	I2C_WriteRegister(0x90,0x01,0x0F);					//CHA, CHB = MUTE, FS VOL
	I2C_WriteRegister(0x90,0x00,0x2A);					// set page 0x2A
	I2C_WriteRegister(0x90,0x01,0x00);					//Disable ASP_TX
	I2C_WriteRegister(0x90,0x00,0x12);					// set page 0x12
	I2C_WriteRegister(0x90,0x07,0x00);					//Disable SCLK
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x01,0xFE);					//Power down HP amp
	I2C_WriteRegister(0x90,0x02,0x8C);					//Power down ASP and SRC
	I2C_WriteRegister(0x90,0x00,0x13);					// set page 0x13
	wait_15ms ();													// delay codec power down

	// read data
	I2C_ReadRegister(0x90,0x08);

	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x02,0x9C);					//Discharge cap, FILT+
	DAC_EN = 0;														// set flag
	ADC_EN = 1;	    											// reset flag
} // --------------------------------------------------------------------------
void enable_mic(void)
{
	// enable mic
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x74,0x07);	//0x07				//HSBIAS set to 2.7V mode
}

void disable_mic(void)
{
	// enable mic
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x74,0x01);					//HSBIAS set to 2.7V mode
}

// ADC power up function ----------------------------------------------
void ADC_pwr_up(void)
{
	
	//revised by watt 2017.02.23
	// enable mic
//	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
//	I2C_WriteRegister(0x90,0x74,0x07);					//HSBIAS set to 2.7V mode
	
//	codec_pwr_up();
	// enable ASP
	I2C_WriteRegister(0x90,0x00,0x29);					// set page 0x29
	I2C_WriteRegister(0x90,0x04,0x00);					//Ch1 Bit Start UB
	I2C_WriteRegister(0x90,0x05,0x00);					//Ch1 Bit Start LB = 00
	I2C_WriteRegister(0x90,0x0A,0x00);					//Ch2 Bit Start UB
	I2C_WriteRegister(0x90,0x0B,0x00);					//Ch2 Bit Start LB = 00
	I2C_WriteRegister(0x90,0x02,0x01);					//enable ASP Transmit CH1
	I2C_WriteRegister(0x90,0x03,0x4A);					//RES=24 bits, CH2 and CH1
	I2C_WriteRegister(0x90,0x01,0x01);					//ASP_TX_EN
	I2C_WriteRegister(0x90,0x02,0x00);					//disable ASP Transmit CH2 and CH1
	I2C_WriteRegister(0x90,0x02,0x03);					//enable ASP Transmit CH2 and CH1
	// set volume
	I2C_WriteRegister(0x90,0x00,0x1C);					// set page 0x1C
	I2C_WriteRegister(0x90,0x03,0xC3);					//set HSBIAS_RAMP to slowest
	I2C_WriteRegister(0x90,0x00,0x1D);					// set page 0x1D
	I2C_WriteRegister(0x90,0x02,0x06);					//ADC soft-ramp enable
	I2C_WriteRegister(0x90,0x03,0x06);					//ADC_VOL = 6dB,
	I2C_WriteRegister(0x90,0x01,0x01);					//ADC_DIG_BOOST; +20dB, 
	I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
	I2C_WriteRegister(0x90,0x02,0x19);					//Mixer ADC_Vol = -25dB, 
																				//Is this a good sidetone amount?
	ADC_EN = 1;						//reset flag
} // --------------------------------------------------------------------------

// ADC power up function ----------------------------------------------
void ADC_pwr_dn(void)
{
	// disable ADC (if currently enabled)
	I2C_WriteRegister(0x90,0x00,0x1D);					// set page 0x1D
	I2C_WriteRegister(0x90,0x03,0x9F);					//ADC_VOL = Mute, 0X9F is Mute
	I2C_WriteRegister(0x90,0x01,0x00);					//ADC_DIG_BOOST; 0x00 is no boost
	
	I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
	I2C_WriteRegister(0x90,0x02,0x3F);					//Mixer ADC_Vol = Mute
	
	//revised by watt 2017.02.23
	//I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	//I2C_WriteRegister(0x90,0x74,0x01);					//Turn off HSBIAS until requested by ADC
	ADC_EN = 0;						//reset flag
	
} // --------------------------------------------------------------------------

// codec initialization function ----------------------------------------------
void codec_init(void)
{

	uchar i;

//	GPIO_SetBits (GPIOB, GPIO_Pin_7);				// take codec out of reset
   CODEC_RESET=1;

	for (i = 0; i < 3; i++) wait_15ms ();			// wait for control port (40ms)

	I2C_WriteRegister(0x90,0x00,0x10);					// set page 0x10
	I2C_WriteRegister(0x90,0x09,0x02);					// MCLK Control: /256 mode
	I2C_WriteRegister(0x90,0x0A,0xA4);					// DSR_RATE to ?
	I2C_WriteRegister(0x90,0x00,0x12);					// set page 0x12
	I2C_WriteRegister(0x90,0x0C,0x00);					//SCLK_PREDIV = 00
	I2C_WriteRegister(0x90,0x00,0x15);					// set page 0x15
	I2C_WriteRegister(0x90,0x05,0x40);					//PLL_DIV_INT = 0x40
	I2C_WriteRegister(0x90,0x02,0x00);					//PLL_DIV_FRAC = 0x000000
	I2C_WriteRegister(0x90,0x03,0x00);					//PLL_DIV_FRAC = 0x000000
	I2C_WriteRegister(0x90,0x04,0x00);					//PLL_DIV_FRAC = 0x000000
	I2C_WriteRegister(0x90,0x1B,0x03);					//PLL_MODE = 11 (bypassed)
	I2C_WriteRegister(0x90,0x08,0x10);					//PLL_DIVOUT = 0x10
	I2C_WriteRegister(0x90,0x0A,0x80);					//PLL_CAL_RATIO = 128
	I2C_WriteRegister(0x90,0x01,0x01);					//power up PLL
	I2C_WriteRegister(0x90,0x00,0x12);					// set page 0x12
	I2C_WriteRegister(0x90,0x01,0x01);					//MCLKint = internal PLL
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x01,0x00);					//power up ASP, Mixer, HP, and DAC
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x75,0x9F);					//Latch_To_VP = 1
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x07,0x01);					//SCLK present
	I2C_WriteRegister(0x90,0x00,0x12);					// set page 0x12
	I2C_WriteRegister(0x90,0x03,0x1F);					//FSYNC High time LB. LRCK +Duty = 32 SCLKs
	I2C_WriteRegister(0x90,0x04,0x00);					//FSYNC High time UB
	I2C_WriteRegister(0x90,0x05,0x3F);					//FSYNC Period LB. LRCK = 64 SCLKs
	I2C_WriteRegister(0x90,0x06,0x00);					//FSYNC Period UB
	I2C_WriteRegister(0x90,0x07,0x2C);					//Enable ASP SCLK, LRCK input, SCPOL_IN (ADC and DAC) inverted
	I2C_WriteRegister(0x90,0x08,0x0A);					//frame starts high to low, I2S mode, 1 SCLK FSD
	I2C_WriteRegister(0x90,0x00,0x1F);					// set page 0x1F
	I2C_WriteRegister(0x90,0x06,0x02);					//Dac Control 2: Default
	I2C_WriteRegister(0x90,0x00,0x20);					// set page 0x20
	I2C_WriteRegister(0x90,0x01,0x01);					//unmute analog A and B, not -6dB mode
	I2C_WriteRegister(0x90,0x00,0x23);					// set page 0x23
	I2C_WriteRegister(0x90,0x01,0xFF);					//CHA_Vol = MUTE
	I2C_WriteRegister(0x90,0x03,0xFF);					//CHB_Vol = MUTE
	I2C_WriteRegister(0x90,0x00,0x2A);					// set page 0x2A
	I2C_WriteRegister(0x90,0x01,0x0C);					//Enable Ch1/2 DAI
	I2C_WriteRegister(0x90,0x02,0x02);					//Ch1 low 24 bit
	I2C_WriteRegister(0x90,0x03,0x00);					//Ch1 Bit Start UB
	I2C_WriteRegister(0x90,0x04,0x00);					//Ch1 Bit Start LB = 00
	I2C_WriteRegister(0x90,0x05,0x42);					//Ch2 high 24 bit
	I2C_WriteRegister(0x90,0x06,0x00);					//Ch2 Bit Start UB
	I2C_WriteRegister(0x90,0x07,0x00);					//Ch2 Bit Start LB = 00
	I2C_WriteRegister(0x90,0x00,0x26);					// set page 0x26
	I2C_WriteRegister(0x90,0x01,0x00);					//SRC in at don't know
	I2C_WriteRegister(0x90,0x09,0x00);					//SRC out at don't know
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x71,0xC1);					//Toggle WAKEB_CLEAR
	I2C_WriteRegister(0x90,0x71,0xC0);					//Set WAKE back to normal
	I2C_WriteRegister(0x90,0x75,0x84);					//Set LATCH_TO_VP
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x29,0x01);					//headset clamp disable
	I2C_WriteRegister(0x90,0x02,0xA7);					//Power Down Control 2

	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x74,0xA7);					//Misc


// codec initialize for plug detection function -------------------------------
// The WAKEB pin will be low when unplugged, high when plugged

	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x75,0x9F);					//Latch_To_VP = 1
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x73,0xE2);					//Write TIP_SENSE_CTRL for plug type
	I2C_WriteRegister(0x90,0x71,0xA0);					//Unmask WAKEB

	
	DAC_EN = 1; 							//next lamInformation will set LEDs
	for (i = 0; i < 33; i++) wait_15ms();			// wait for plug detect (500ms)

} // --------------------------------------------------------------------------

// headset type detection function --------------------------------------------
void hs_type_det(void)
{
	uchar i;
	uchar temp_rd;
	uchar auto_done;
//	uchar D_CTRL_2;


	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x75,0x9F);					//Set Latch_To_VP = 1
	// Configure the automatic headset-type detection.
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x29,0x01);					//HS Clamp disabled
	I2C_WriteRegister(0x90,0x01,0xFE);					//PDN_ALL = 0
	I2C_WriteRegister(0x90,0x21,0x00);					//Set all switches to open
	I2C_WriteRegister(0x90,0x00,0x1F);					// set page 0x1F
	I2C_WriteRegister(0x90,0x06,0x86);					//Disable HPOUT_CLAMP and HPOUT_PULLDOWN when channels are powered down
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x74,0x07);					//HSBIAS set to 2.7V mode
	//Wait for HSBIAS to ramp up, set to slowest at 0x1C03
	for (i = 0; i < 13; i++) wait_15ms ();			// wait for control port (195 ms)
	I2C_WriteRegister(0x90,0x00,0x13);					// set page 0x13
	I2C_WriteRegister(0x90,0x1B,0x01);					//HSDET interrupt unmasked
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x20,0x80);					//HSDET_CTRL = Automatic, Disabled
													//Wait 100us
	wait_15ms ();
	I2C_WriteRegister(0x90,0x1F,0x77);					//set COMPs 2V, 1V
	I2C_WriteRegister(0x90,0x20,0xC0);					//HSDET_CTRL = Automatic, Enabled (trigger a detect sequence)
													//Wait for interrupt
	// Service the HSDET_AUTO_DONE interrupt.
	I2C_WriteRegister(0x90,0x00,0x13);					// set page 0x13
	do
	{
		temp_rd = I2C_ReadRegister(0x90,0x08);			// read 0x1308
		if ((temp_rd & 0x02) == 0x02){		//test bit 1
			auto_done = 1;									// set flag showing it completed
		} else {
			wait_15ms ();							// delay
		}
	} while (!auto_done);					//wait until HSDET logic has finished its detection cycle
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	temp_rd = I2C_ReadRegister(0x90,0x24);
	//hs_type_rd = temp_rd;                 //****DEBUG Only
	// Decode HSDET_TYPE

	// unplug
	I2C_WriteRegister(0x90,0x00,0x1b);					//set page
	I2C_WriteRegister(0x90,0x70,0x03);					// Reset Auto HSBIAS Clamp = Current Sense


	I2C_WriteRegister(0x90,0x20,0x80);					//HSDET mode set to auto, disabled
	// If headset type 1-3 is detected, the switches are set automatically.


	//I2C_WriteRegister(0x90,0x00, 0x11);					// set page 0x11
	//temp_rd = codec_reg_read (0x24);			//was 0x24
	// Decode HSDET_TYPE
	Optical_Only = 0;							          //start with assumption
	HP_Only = 0;							          //start with assumption
	if (temp_rd & 0x01){
	} else {
		HP_Only = 1;							          //possibly HP_Only
	}
	if (temp_rd & 0x02){
		Optical_Only = (temp_rd & 0x1);			//Only set if both bits are 1
	} else {
		HP_Only = 0;							          // a mic is present
	}
	for (i = 0; i < 8; i++) wait_15ms ();			// wait for control port (120 ms)


	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x74,0x01);					//Turn off HSBIAS until requested by ADC





} // --------------------------------------------------------------------------

// Headset Manual Default function --------------------------------------------
void hs_manual(void)
{
	// manually apply AHJ (Type 1) switch states
	I2C_WriteRegister(0x90,0x00,0x1B);					// set page 0x1B
	I2C_WriteRegister(0x90,0x75,0x84);					// Set LATCH_TO_VP
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x02,0xA7);					// Power Down Control 2
	I2C_WriteRegister(0x90,0x00,0x11);					// set page 0x11
	I2C_WriteRegister(0x90,0x21,0xA6);					// set switches for Apple headset
	I2C_WriteRegister(0x90,0x20,0x00);					// HSDET_CTRL = Manual, Disabled	
	Optical_Only = 0;							        // manually clear
	HP_Only = 0;							            // manually clear

	
} // --------------------------------------------------------------------------

uint8_t next_plug_state (uint8_t current_plug_state)
{

	uint8_t temp_state;
	uint8_t i;
	uint16_t mV;

	temp_state = current_plug_state;				// latch current state
		//using global Tip_plugged instead of current_plug_state

	if (!Tip_plugged) {												//flag shows not plugged
			if(HP_DET==1){		//true when plugged
				Tip_plugged = 1;								// set flag showing plugged 
				temp_state = 1;
				switch_ADC0_from_Interrupt();
				// ADC0_startConversion();

    // Wait for conversion to complete
        // while (!ADC0_isConversionComplete());	
    //ADC0_temp=;
     // Convert sample to mV
        // mV = ADC0_convertSampleToMillivolts(ADC0_getResult());
					 
					
			while(1)
			  {
				hs_type_det ();
				enable_mic();
        codec_pwr_up();
			//	ADC_pwr_up();
				DAC_EN = 0; 							//next lamInformation will set LEDs
						
				pending_en_ch = 1;
				for (i = 0; i < 13; i++) //for (i = 0; i < 13; i++)
				wait_15ms ();			// wait for control port (195 ms)
				
				 ADC0_startConversion();
         while (!ADC0_isConversionComplete());	
        // ADC0_temp=;
     // Convert sample to mV
         mV = ADC0_convertSampleToMillivolts(ADC0_getResult());
         if((mV>100)&&(hptype_on==1))
    	            break;
    	   if(HP_DET==0) 	   
    	     {
    		     goto unplugged;
    	     }    
    	   if(mV>100)
      	   {
      	 	 hptype_on=2;
       	   HP_SWITCH=1;
      	 	  break;
      	   }
           else
       	   {
      	 	hptype_on=1;
      	 	HP_SWITCH=0;
       	   }          
		   } 	
		   switch_Interrupt_from_ADC0();
			} 
	}	else {														//Tip_plugged
			if(!HP_DET){	//true when not plugged
				//it was previously plugged but now it is not
unplugged:
				Tip_plugged = 0;							// reset flag 
				temp_state = 0;
				pending_en_ch = 1;
				//GPIO_ResetBits (GPIOB, GPIO_Pin_6);		// green OFF
				//GPIO_ResetBits (GPIOB, GPIO_Pin_5);		// red OFF
				DAC_EN = 0; 							//next lamInformation will set LEDs
				ADC_EN = 0; 							//next lamInformation will set LEDs
				HP_Only = 1;									// no mic
				hptype_on=0;
				HP_SWITCH=1;
				// unplug
				 IE_EX0=0;
         IE_EX1=0;  	
				disable_mic();
				codec_pwr_dn();
				I2C_WriteRegister(0x90,0x00,0x1b);					//set page
				I2C_WriteRegister(0x90,0x70, 0x03);					// Reset Auto HSBIAS Clamp = Current Sense
			//	ADC_pwr_dn();
			} 
	}

	return (temp_state);

} // --------------------------------------------------------------------------

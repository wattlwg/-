//=========================================================
// inc/InitDevice.h: generated by Hardware Configurator
//
// This file will be regenerated when saving a document.
// leave the sections inside the "$[...]" comment tags alone
// or they will be overwritten!
//=========================================================
#include <SI_EFM8BB2_Register_Enums.h>
#include <stdio.h>
#ifndef __INIT_DEVICE_H__
#define __INIT_DEVICE_H__

// USER CONSTANTS
// USER PROTOTYPES
//SI_SFR (EIE1,      0xE6); ///< Extended Interrupt Enable 1 
//#define SFR_EIE1 0xE6
//SI_SBIT (EIE1_EPCA0,SFR_EIE1, 5); ///< External Interrupt 1 Enable
SI_SBIT (VOLUMEIN, SFR_P0, 0);
SI_SBIT (VOLUMEDOWN, SFR_P0, 1);
SI_SBIT (HP_TYPE, SFR_P0, 2);
SI_SBIT (LAMMUTE, SFR_P0, 3);
SI_SBIT (POWERON, SFR_P0, 6);
SI_SBIT (MUTE, SFR_P0, 7);

SI_SBIT (SDA, SFR_P1, 0); 
SI_SBIT (SCL, SFR_P1, 1);
SI_SBIT (HP_DET, SFR_P1, 2); 
SI_SBIT (MICSWITCH, SFR_P1, 3);
SI_SBIT (SELECT, SFR_P1, 4);
SI_SBIT (VOLUMEUP, SFR_P1, 5);
SI_SBIT (CODEC_RESET, SFR_P1, 6);

SI_SBIT (HP_SWITCH, SFR_P2, 0);
// $[Mode Transition Prototypes]
extern void enter_DefaultMode_from_RESET(void);
// [Mode Transition Prototypes]$

// $[Config(Per-Module Mode)Transition Prototypes]
extern void WDT_0_enter_DefaultMode_from_RESET(void);
extern void PORTS_0_enter_DefaultMode_from_RESET(void);
extern void PORTS_1_enter_DefaultMode_from_RESET(void);
extern void PORTS_2_enter_DefaultMode_from_RESET(void);
extern void PBCFG_0_enter_DefaultMode_from_RESET(void);
extern void CLOCK_0_enter_DefaultMode_from_RESET(void);
extern void TIMER01_0_enter_DefaultMode_from_RESET(void);
extern void TIMER_SETUP_0_enter_DefaultMode_from_RESET(void);
extern void UART_0_enter_DefaultMode_from_RESET(void);
extern void INTERRUPT_0_enter_DefaultMode_from_RESET(void);
extern void EXTINT_0_enter_DefaultMode_from_RESET(void);
extern void TIMER16_2_enter_DefaultMode_from_RESET(void);
extern void TIMER16_3_enter_DefaultMode_from_RESET(void);
extern void PCA_0_enter_DefaultMode_from_RESET(void);
extern void PCACH_0_enter_DefaultMode_from_RESET(void);
extern void ADC_0_enter_DefaultMode_from_RESET(void);
extern void enter_ADC0_Disable(void);
extern void switch_Interrupt_from_ADC0(void);
extern void PBCFG_0_switch_ADC0_from_Interrupt(void);
extern void PBCFG_0_switch_Interrupt_from_ADC0(void);
extern void switch_ADC0_from_Interrupt(void);
extern void VREF_0_enter_DefaultMode_from_RESET(void);
// [Config(Per-Module Mode)Transition Prototypes]$


#endif


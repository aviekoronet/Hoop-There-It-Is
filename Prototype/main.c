/*
* Rochester Institute of Technology
* Department of Computer Engineering
* CMPE 460  Interfacing Digital Electronics
* Spring 2016
*
* Filename: main_A2D_template.c
*/
 
#include "uart.h"
#include "MK64F12.h"
#include "stdio.h"
uint8_t countdone = 0;
uint16_t count = 0;
uint16_t time = 0;
uint16_t period = 5000;//count for 5 seconds
uint16_t threshold = 1000;
char str[100];

 void initFTM(void){
	//Enable clock for FTM module (use FTM0)
	SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
	SIM_SCGC6 |= SIM_SCGC6_FTM1_MASK;
	SIM_SCGC6 |= SIM_SCGC6_FTM2_MASK;
	
	//turn off FTM Mode to  write protection;
		FTM0_MODE |= FTM_MODE_WPDIS_MASK;
		FTM1_MODE |= FTM_MODE_WPDIS_MASK;
		FTM2_MODE |= FTM_MODE_WPDIS_MASK;
	
	//divide the input clock down by 128,  
	FTM0_SC |= FTM_SC_PS_MASK;
	FTM1_SC |= FTM_SC_PS_MASK;
	FTM2_SC |= FTM_SC_PS_MASK;
	
	//reset the counter to zero
	FTM0_CNT = (uint16_t)0x0000;
	FTM1_CNT = (uint16_t)0x0000;
	FTM2_CNT = (uint16_t)0x0000;
	
	
	//Set the overflow rate
	//(Sysclock/128)- clock after prescaler
	//(Sysclock/128)/1000- slow down by a factor of 1000 to go from
	//Mhz to Khz, then 1/KHz = msec
	//Every 1msec, the FTM counter will set the overflow flag (TOF) and 
	FTM0->MOD = (20971520u/(1<<7))/1000;
	FTM1->MOD = (20971520u/(1<<7))/1000;
	FTM2->MOD = (20971520u/(1<<7))/1000;
	
	//Select the System Clock 
	FTM0_SC &= ~FTM_SC_CLKS_MASK;
	FTM0_SC |= FTM_SC_CLKS(0x1);
	
	//Enable the interrupt mask. Timer overflow Interrupt enable
	FTM0_SC |= FTM_SC_TOIE_MASK;
	
	return;
}
 
void PDB_INIT(void) {
    //Enable PDB Clock
    SIM_SCGC6 |= SIM_SCGC6_PDB_MASK;

    PDB0_SC = PDB_SC_PDBEN_MASK | PDB_SC_CONT_MASK | PDB_SC_TRGSEL(0xf)
                                    | PDB_SC_LDOK_MASK;
		PDB0_SC |= PDB_SC_PRESCALER_MASK;
		PDB0->MOD = 5000;
    PDB0_CH1C1 = PDB_C1_EN(0x01) | PDB_C1_TOS(0x01);
		PDB0_SC |= PDB_SC_SWTRIG_MASK;
}
 
void ADC1_INIT(void) {
    unsigned int calib;
 
    // Turn on ADC1
    SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK;
 
    // Configure CFG Registers 
    // Configure ADC to divide 50 MHz down to 6.25 MHz AD Clock, 16-bit single ended
    ADC1_CFG1 &= ~ADC_CFG1_ADIV_MASK & ~ADC_CFG1_MODE_MASK;
    ADC1_CFG1 |= ADC_CFG1_ADIV(0x00); //no clock divider
    ADC1_CFG1 |= ADC_CFG1_MODE(0x03); //11 means 16 bit single ended
 
    // Do ADC Calibration for Singled Ended ADC. Do not touch.
    ADC1_SC3 = ADC_SC3_CAL_MASK;
    while ( (ADC1_SC3 & ADC_SC3_CAL_MASK) != 0 );
    calib = ADC1_CLP0;
    calib += ADC1_CLP1;
    calib += ADC1_CLP2;
    calib += ADC1_CLP3;
    calib += ADC1_CLP4;
    calib += ADC1_CLPS;
    calib = calib >> 1;
    calib |= 0x8000;
    ADC1_PG = calib;
 
    // Configure SC registers.
    // Select hardware trigger.
    ADC1_SC2 |= ADC_SC2_ADTRG_MASK;
		//SIM_SOPT7 stuff
		SIM_SOPT7 &= ~(SIM_SOPT7_ADC1PRETRGSEL_MASK | SIM_SOPT7_ADC1ALTTRGEN_MASK |
											SIM_SOPT7_ADC1TRGSEL_MASK);
										
    // Configure SC1A register.
    // Select ADC Channel and enable interrupts. Use ADC1 channel DADP3  in single ended mode.
	//pg 833
    ADC1_SC1A |= ADC_SC1_AIEN_MASK; //Enable interrupt
	ADC1_SC1A &= ~(ADC_SC1_DIFF_MASK); //Set diff to single ended convervion (0)
	ADC1_SC1A &= ~ADC_SC1_ADCH_MASK; //DADP0 input DAD3 output '00011'
	//ADC1_SC1A |= ADC_SC1_ADCH(0x03); //DADP3 input DAD3 output '00011'
 
 
    // Enable NVIC interrupt
    NVIC_EnableIRQ(ADC1_IRQn);
}
 
// ADC1 Conversion Complete ISR
void ADC1_IRQHandler(void) {
    // Read the result (upper 12-bits). This also clears the Conversion complete flag.
    unsigned short i = ADC1_RA >> 4;
				if (ADC1_RA > threshold && !countdone){
					count++;
				}
		
    //Set DAC output value (12bit)
    DAC0_DAT1L = (i & 0x00FF0);
	DAC0_DAT1H |= (unsigned char)(i >> 8) & 0x0F; //preserves the top 4 bits
}

void FTM0_IRQHandler(void){ //For FTM timer
	FTM0_SC &= ~FTM_SC_TOF_MASK;
		if (time < period){
		time += 1;//increment counter
		}
		else{
			countdone = 1;
			time = 0;
		}
}

void DAC0_INIT(void) {
    //enable DAC clock
    SIM_SCGC2 |= SIM_SCGC2_DAC0_MASK;
    DAC0_C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;
    DAC0_C1 = 0;
}

 
int main(void) {
    // Initialize modules
    uart_init();
               
    DAC0_INIT();
    ADC1_INIT();
    PDB_INIT();
		initFTM();
		NVIC_EnableIRQ(FTM0_IRQn);
    
		// Start the PDB (ADC Conversions)
    PDB0_SC |= PDB_SC_SWTRIG_MASK;
    for(;;) {
		if (countdone){
				sprintf(str,"\n My Heart Rate is %d BPM\n\r",count*12);
				uart_put(str);
				countdone = 0;
				count = 0;
				}
    }
}
 

/*
 * Switch.c
 *
 *  Created on: Nov 5, 2023
 *      Author: Colby McLane & Ajay Bulusu
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    IOMUX->SECCFG.PINCM[PB12INDEX] = 0x00040081;
    IOMUX->SECCFG.PINCM[PB17INDEX] = 0x00040081;
}
// return current state of switches
uint32_t Switch_In(void){
    // write this
  return(GPIOB->DIN31_0&(0x00021000));
}

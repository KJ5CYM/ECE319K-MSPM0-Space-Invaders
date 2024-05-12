 //Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Colby C. McLane & Ajay V. Bulusu
// Last Modified: 12/31/2023

//    _____ ____  __________    _____   ___    _____    ____  __________  _____
//   / ___// __ \/ ____/ __ \  /  _/ | / / |  / /   |  / __ \/ ____/ __ \/ ___/
//   \__ \/ /_/ / __/ / / / /  / //  |/ /| | / / /| | / / / / __/ / /_/ /\__ \
//  ___/ / ____/ /___/ /_/ /  / // /|  / | |/ / ___ |/ /_/ / /___/ _, _/___/ /
// /____/_/   /_____/_____/ /___/_/ |_/  |___/_/  |_/_____/_____/_/ |_|/____/

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"


uint32_t LanguageFlag = 0;      // IF languageflag = 1: French
                               // IF languageflag = 0: English
uint32_t spritePositionX = 0; // Initial position
uint32_t spriteDirectionX = 1; // Initial direction
uint32_t spritePositionY = 0; // Initial position
uint32_t life[30];
uint32_t Score = 0;
uint32_t win = 0;
uint32_t Data;        // 12-bit ADC
uint32_t Position;    // 32-bit fixed-point 0.001 cm
float FloatPosition;  // 32-bit floating-point cm
uint32_t Flag;        // semaphore
uint32_t startTime,stopTime;
uint32_t Offset,ADCtime,Converttime,FloatConverttime,OutFixtime,FloatOutFixtime; // in bus cycles
uint32_t Time;

void pause(void){
    Switch_Init(); // initialize switches
    uint32_t pausePushed = 0;
    while(1)
    {
        pausePushed = Switch_In();
        if(pausePushed == 0x0000100)
        {
            pausePushed = Switch_In();
            if(pausePushed = 0x0000000)
            {
            break;
            }
        }
    }


}
// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}


// games  engine runs at 30Hz
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    Time++;
    Data = ADCin();
    Flag = 1;
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

int GameLoop(void){
     Switch_Init(); // initialize switches
     Sound_Init();  // initialize sound
     ST7735_InitPrintf();
     ST7735_FillScreen(ST7735_BLACK);
     uint32_t downflag = 0;
     uint32_t firepushed;
     uint32_t bulletflag;
     uint32_t bulletPositionY = 0;
     uint32_t og_pos;
     uint32_t OldPosition;
     uint32_t bulletPositionX;
     uint32_t Shiplife = 1;
     uint32_t spriteposx[30];
     uint32_t spriteposy[30];
     uint32_t m;
     uint32_t SpriteBulletFlag = 0;
     uint32_t spritepossx = 0;
     uint32_t spritepossy=0;
     uint32_t pausePushed;
     for (uint32_t i = 0; i<=30; i++){
         life[i] = 1;
     }
     ADCinit();
     TimerG12_IntArm((80000000/30),1);
     #define T33ms 26666666
     #define longt 2666666666
     Flag = 0;
     Time = 0;
     __enable_irq();
     while(1){

              pausePushed = Switch_In();
              firepushed = Switch_In();
              uint32_t bulletSpeed = 10;
              if ((firepushed == 0x00020000) && (bulletPositionY == 0)){
                  Sound_Shoot();
                  bulletPositionX = Position;
                  og_pos = bulletPositionX;
                  bulletPositionY = 151;
                  bulletflag = 1;
              }
              if (bulletflag == 1){
                  ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                  bulletPositionY -= bulletSpeed; // Move the bullet upwards
                  ST7735_DrawBitmap((bulletPositionX+8), bulletPositionY, Laser0, 2,9);
                  SpriteBulletFlag = 1;
                  if(spritepossy>170)
                  {
                  m =  Random(30);
                  spritepossx = spriteposx[m];
                  spritepossy = spriteposy[m];
                  }

              }
              if(SpriteBulletFlag == 1){
              ST7735_DrawBitmap((spritepossx), spritepossy, Laser1, 2,9);
              spritepossy += bulletSpeed; // Move the bullet upwards
              ST7735_DrawBitmap(spritepossx, spritepossy, Missile, 2,9);
              }

         while (Flag == 0){
      // Update sprite position
      spritePositionX += spriteDirectionX;
      // Check for collision with walls
      if (spritePositionX >= 10 || spritePositionX <= 0) {
      // Reverse direction
      spriteDirectionX *= -1;
      downflag++;
      }
      if (downflag == 2){
          downflag = 0;
          spritePositionY += 1;
      }
      // Draw the sprites at the new position
      // Repeat for all other sprites, adding spritePositionX to the X coordinate
      // 10pointA
        if (life[0] == 1){
            ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 19, SmallEnemy30pointA, 16,10);
            spriteposx[0] = spritePositionX + 106;
            spriteposy[0] = spritePositionY + 19;
            if(spriteposy[0]>160)
                                 {
                                     Shiplife=0;
                                 }
            if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) && (((spritePositionX + 86) < (bulletPositionX-4)) && ((spritePositionX +106)>(bulletPositionX-4))))
            {
                    ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 19, Dead, 16,10);
                    bulletPositionY=0;
                    life[0]=0;
                    Clock_Delay(T33ms);
                    Sound_Shoot();
                    ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                    ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 19, Clean, 16,10);
                    bulletflag=0;
            }
            if(spriteposy[0]>160)
            {
                Shiplife==0;
            }
        }
        if (life[1] == 1){
            ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 19, SmallEnemy30pointA, 16,10);
            spriteposx[1] = spritePositionX + 86;
            spriteposy[1] = spritePositionY + 19;
            if(spriteposy[1]>160)
                                 {
                                     Shiplife=0;
                                 }
            if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) && (((spritePositionX + 66) < (bulletPositionX-4)) && ((spritePositionX -46)>(bulletPositionX-4))))
                       {
                               ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 19, Dead, 16,10);
                               bulletPositionY=0;
                               life[1]=0;
                               Clock_Delay(T33ms);
                               Sound_Killed();
                               ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                               ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 19, Clean, 16,10);
                               bulletflag=0;
                       }

        }
        if (life[2] == 1){
            ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 19, SmallEnemy30pointA, 16,10);
            spriteposx[2] = spritePositionX + 66;
            spriteposy[2] = spritePositionY + 19;
            if(spriteposy[2]>160)
                                 {
                                     Shiplife=0;
                                 }
            if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) && (((spritePositionX + 46) < (bulletPositionX-4)) && ((spritePositionX +66)>(bulletPositionX-4))))
                       {
                               ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 19, Dead, 16,10);
                               bulletPositionY=0;
                               life[2]=0;
                               Clock_Delay(T33ms);
                               Sound_Killed();
                               ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                               ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 19, Clean, 16,10);
                               bulletflag=0;
                       }
        }
        if (life[3] == 1){
        ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 19, SmallEnemy30pointA, 16,10);
        spriteposx[3] = spritePositionX + 46;
        spriteposy[3] = spritePositionY + 19;
        if(spriteposy[3]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) && (((spritePositionX + 26) < (bulletPositionX-4)) && ((spritePositionX +46)>(bulletPositionX-4))))
                   {
                           ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 19, Dead, 16,10);
                           bulletPositionY=0;
                           life[3]=0;
                           Clock_Delay(T33ms);
                           Sound_Killed();
                           ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                           ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 19, Clean, 16,10);
                           bulletflag=0;
                   }
        }
        if (life[4] == 1){
        ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 19, SmallEnemy30pointA, 16,10);
        spriteposx[4] = spritePositionX + 26;
        spriteposy[4] = spritePositionY + 19;
        if(spriteposy[4]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) && (((spritePositionX + 6) < (bulletPositionX-4)) && ((spritePositionX +26)>(bulletPositionX-4))))                   {
                           ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 19, Dead, 16,10);
                           bulletPositionY=0;
                           life[4]=0;
                           Clock_Delay(T33ms);
                           Sound_Killed();
                           ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                           ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 19, Clean, 16,10);
                           bulletflag=0;
                   }
        }
        if (life[5] == 1){
        ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 19, SmallEnemy30pointA, 16,10);
        spriteposx[5] = spritePositionX + 6;
        spriteposy[5] = spritePositionY + 19;
        if(spriteposy[5]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+9)&&(bulletPositionY < spritePositionY+19)) &&(((spritePositionX + 0) < (bulletPositionX-4)) && ((spritePositionX +6)>(bulletPositionX-4))))
                   {
                           ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 19, Dead, 16,10);
                           bulletPositionY=0;
                           life[5]=0;
                           Clock_Delay(T33ms);
                           Sound_Killed();
                           ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                           ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 19, Clean, 16,10);
                           bulletflag=0;
                   }
        }
      // 10pointB
        if (life[6] == 1){
        ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[6] = spritePositionX + 106;
        spriteposy[6] = spritePositionY + 19;
        if(spriteposy[6]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX + 86) < (bulletPositionX-4)) && ((spritePositionX +106)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[6]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }

        if (life[7] == 1){
        ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[7] = spritePositionX + 86;
        spriteposy[7] = spritePositionY + 29;
        if(spriteposy[7]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX + 66) < (bulletPositionX-4)) && ((spritePositionX -46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[7]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[8] == 1){
        ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[8] = spritePositionX + 66;
        spriteposy[8] = spritePositionY + 29;
        if(spriteposy[8]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX + 46) < (bulletPositionX-4)) && ((spritePositionX +66)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[8]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[9] == 1){
        ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[9] = spritePositionX + 46;
        spriteposy[9] = spritePositionY + 29;
        if(spriteposy[9]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX + 26) < (bulletPositionX-4)) && ((spritePositionX +46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[9]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[10] == 1){
        ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[10] = spritePositionX + 26;
        spriteposy[10] = spritePositionY + 29;
        if(spriteposy[10]>160)
                             {
                                 Shiplife=0;
                             }
        if  (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX + 6) < (bulletPositionX-4)) && ((spritePositionX +26)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[10]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[11] == 1){
        ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 29, SmallEnemy10pointB, 16,10);
        spriteposx[11] = spritePositionX + 6;
        spriteposy[11] = spritePositionY + 29;
        if(spriteposy[11]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+19)&&(bulletPositionY < spritePositionY+29)) && (((spritePositionX) < (bulletPositionX-4)) && ((spritePositionX +6)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 29, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[11]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 29, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[12] == 1){
        ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[12] = spritePositionX + 106;
        spriteposy[12] = spritePositionY + 39;
        if(spriteposy[12]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX + 86) < (bulletPositionX-4)) && ((spritePositionX +106)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[12]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[13] == 1){
        ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[13] = spritePositionX + 86;
        spriteposy[13] = spritePositionY + 39;
        if(spriteposy[13]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX + 66) < (bulletPositionX-4)) && ((spritePositionX -46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[13]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[14] == 1){
        ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[14] = spritePositionX + 66;
        spriteposy[14] = spritePositionY + 39;
        if(spriteposy[14]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX + 46) <(bulletPositionX-4)) && ((spritePositionX +66)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[14]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[15] == 1){
        ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[15] = spritePositionX + 46;
        spriteposy[15] = spritePositionY + 39;
        if(spriteposy[15]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX + 26) < (bulletPositionX-4)) && ((spritePositionX +46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[15]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[16] == 1){
        ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[16] = spritePositionX + 26;
        spriteposy[16] = spritePositionY + 39;
        if(spriteposy[16]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX + 6) < (bulletPositionX-4)) && ((spritePositionX + 26)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[16]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[17] == 1){
        ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 39, SmallEnemy10pointB, 16,10);
        spriteposx[17] = spritePositionX + 6;
        spriteposy[17] = spritePositionY + 39;
        if(spriteposy[17]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+29)&&(bulletPositionY < spritePositionY+39)) && (((spritePositionX) < (bulletPositionX-4)) && ((spritePositionX +6)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 39, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[17]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 39, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
      // 20pointB
        if (life[18] == 1){
        ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[18] = spritePositionX + 106;
        spriteposy[18] = spritePositionY + 49;
        if(spriteposy[18]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX + 86) <(bulletPositionX-4)) && ((spritePositionX +106)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[18]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[19] == 1){
        ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[19] = spritePositionX + 86;
        spriteposy[19] = spritePositionY + 49;
        if(spriteposy[19]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX + 66) < (bulletPositionX-4)) && ((spritePositionX -46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[19]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[20] == 1){
        ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[20] = spritePositionX + 66;
        spriteposy[20] = spritePositionY + 49;
        if(spriteposy[20]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX + 46) < (bulletPositionX-4)) && ((spritePositionX +66)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[20]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[21] == 1){
        ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[21] = spritePositionX + 46;
        spriteposy[21] = spritePositionY + 49;
        if(spriteposy[21]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX + 26) < (bulletPositionX-4)) && ((spritePositionX +46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[21]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[22] == 1){
        ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[22] = spritePositionX + 26;
        spriteposy[22] = spritePositionY + 49;
        if(spriteposy[22]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX + 6) < (bulletPositionX-4)) && ((spritePositionX +26)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[22]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[23] == 1){
        ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 49, SmallEnemy20pointB, 16,10);
        spriteposx[23] = spritePositionX + 6;
        spriteposy[23] = spritePositionY + 49;
        if(spriteposy[23]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+39)&&(bulletPositionY < spritePositionY+49)) && (((spritePositionX ) < (bulletPositionX-4)) && ((spritePositionX +6)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 49, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[23]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 49, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[24] == 1){
        ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[24] = spritePositionX + 106;
        spriteposy[24] = spritePositionY + 59;
        if(spriteposy[24]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59)) && (((spritePositionX + 86) < (bulletPositionX-4)) && ((spritePositionX +106)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[24]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 106, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[25] == 1){
        ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[25] = spritePositionX + 86;
        spriteposy[25] = spritePositionY + 59;
        if(spriteposy[25]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59)) && (((spritePositionX + 66) < (bulletPositionX-4)) && ((spritePositionX -46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[25]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 86, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[26] == 1){
        ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[26] = spritePositionX + 66;
        spriteposy[26] = spritePositionY + 59;
        if(spriteposy[26]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59))&& (((spritePositionX + 46) < (bulletPositionX-4)) && ((spritePositionX +66)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[26]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 66, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[27] == 1){
        ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[27] = spritePositionX + 46;
        spriteposy[27] = spritePositionY + 59;
        if(spriteposy[27]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59))&& (((spritePositionX + 26) < (bulletPositionX-4)) && ((spritePositionX +46)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[27]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 46, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[28] == 1){
        ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[28] = spritePositionX + 26;
        spriteposy[28] = spritePositionY + 59;
        if(spriteposy[28]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59)) && (((spritePositionX + 6) < (bulletPositionX-4)) && ((spritePositionX +26)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[28]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 26, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;
                               }
        }
        if (life[29] == 1){
        ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 59, SmallEnemy20pointB, 16,10);
        spriteposx[29] = spritePositionX + 6;
        spriteposy[29] = spritePositionY + 59;
        if(spriteposy[29]>160)
                             {
                                 Shiplife=0;
                             }
        if (((bulletPositionY > spritePositionY+49)&&(bulletPositionY < spritePositionY+59)) && (((spritePositionX) < (bulletPositionX-4)) && ((spritePositionX +6)>(bulletPositionX-4))))
                               {
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 59, Dead, 16,10);
                                       bulletPositionY=0;
                                       life[29]=0;
                                       Clock_Delay(T33ms);
                                       Sound_Killed();
                                       ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
                                       ST7735_DrawBitmap(spritePositionX + 6, spritePositionY + 59, Clean, 16,10);
                                       bulletflag=0;

                               }
        }
        pausePushed = Switch_In();
                if(pausePushed == 0x0000100)
                {
                    Clock_Delay(longt);
                }

        if (bulletPositionY==1){
            ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
            bulletflag=0; // prevents subtraction
            bulletPositionY=0;
        }
        if (bulletPositionY==0){
            ST7735_DrawBitmap((og_pos+8), bulletPositionY, Laser1, 2,9);
        }
        if(spritepossy>180){
            spritepossx = spriteposx[m];
            spritepossy = spriteposy[m];
            SpriteBulletFlag=0;
            ST7735_DrawBitmap((spritepossx), spritepossy, Laser1, 2,9);
        }
        Clock_Delay(T33ms);
     }
         ST7735_DrawBitmap(OldPosition, 150, Clean, 16,10);
         Flag = 0;
         Data = ADCin();
         Data = Convert(Data)/18.86;
         Position = Data;
         OldPosition = Position;
         if(Shiplife == 1){
             ST7735_DrawBitmap(Data, 150, PlayerShip0, 18,8);
         }
         if((spritepossy > 160) && (spritepossx < Data+18) && (spritepossx > Data)){
             Shiplife = 0;
         }
         if(Shiplife == 0)
         {
           ST7735_DrawBitmap(Data, 150, PlayerShipX, 18,8);
           break;
         }
         uint32_t count = 0;
         for(uint32_t j = 0; j<=30; j++)
         {
             if(life[j]==0)
             {
                 count++;
             }
             if(count == 30)
             {
                 win = 1;
                 break;
             }
         }
     }
}


int main(void){ // main5
  __disable_irq();
  uint32_t switchSet;
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  Switch_Init(); // initialize switches
  ST7735_InitPrintf();
  ST7735_FillScreen(ST7735_BLACK);
  while(1){
     ST7735_SetCursor(1,1);
     ST7735_OutString("Choose Language:");
     ST7735_SetCursor(1,3);
     ST7735_OutString("Push right button");
     ST7735_SetCursor(1,4);
     ST7735_OutString("for English");
     ST7735_SetCursor(1,6);
     ST7735_OutString("Push left button");
     ST7735_SetCursor(1,7);
     ST7735_OutString("for French");
     switchSet = Switch_In();
     if ((switchSet == 0x00020000)){
            LanguageFlag = 0;
            break;
        }
        if ((switchSet == 0x00001000)){
            LanguageFlag = 1;
            break;
        }
  }
  if (LanguageFlag == 0){
      ST7735_FillScreen(ST7735_BLACK);
       while(1){
       ST7735_SetCursor(1, 1);
       ST7735_OutString("       SPACE");
       ST7735_SetCursor(1, 2);
       ST7735_OutString("      INVADERS");
       ST7735_SetCursor(1, 7);
       ST7735_OutString("   *SCORE TABLE*");
       ST7735_DrawBitmap(20,97, SmallEnemy30pointA, 16,10);
       ST7735_SetCursor(6, 9);
       ST7735_OutString(" = 30 points");
       ST7735_DrawBitmap(20,117, SmallEnemy10pointB, 16,10);
       ST7735_SetCursor(6, 11);
       ST7735_OutString(" = 20 points");
       ST7735_DrawBitmap(20,137, SmallEnemy20pointB, 16,10);
       ST7735_SetCursor(6, 13);
       ST7735_OutString(" = 10 points");
       switchSet = Switch_In();
       if ((switchSet == 0x00020000) || (switchSet == 0x00001000)){
           GameLoop();
           while(1){
           ST7735_SetCursor(1, 1);
           ST7735_OutString("        GAME");
           ST7735_SetCursor(1, 2);
           ST7735_OutString("        OVER");
           ST7735_SetCursor(1, 7);
           ST7735_OutString("   SCORE:");
           ST7735_SetCursor(7,10);
           for(uint32_t i; i < 6; i++){
               if(life[i] == 0){
                   Score = Score + 30;
               }
           }
            for(uint32_t i=7; i<=18; i++){
                if(life[i] == 0){
                    Score = Score + 20;
                }
            }

           for(uint32_t i=19; i<=30; i++){
               if(life[i] == 0){
                   Score = Score + 10;
               }
           }
           printf("%d",Score);
           break;
           }
       }
       break;
       }
  }
  if (LanguageFlag == 1){
      ST7735_FillScreen(ST7735_BLACK);
       while(1){
       ST7735_SetCursor(1, 1);
       ST7735_OutString("    Envahisseurs");
       ST7735_SetCursor(1, 2);
       ST7735_OutString("    de l'espace");
       ST7735_SetCursor(1, 7);
       ST7735_OutString(" *Tableau de score*");
       ST7735_DrawBitmap(20,97, SmallEnemy30pointA, 16,10);
       ST7735_SetCursor(6, 9);
       ST7735_OutString(" = 30 points");
       ST7735_DrawBitmap(20,117, SmallEnemy10pointB, 16,10);
       ST7735_SetCursor(6, 11);
       ST7735_OutString(" = 20 points");
       ST7735_DrawBitmap(20,137, SmallEnemy20pointB, 16,10);
       ST7735_SetCursor(6, 13);
       ST7735_OutString(" = 10 points");
       switchSet = Switch_In();
       if ((switchSet == 0x00020000) || (switchSet == 0x00001000)){
           GameLoop();
           while(1){
           ST7735_SetCursor(1, 1);
           ST7735_OutString("        Jeu");
           ST7735_SetCursor(1, 2);
           ST7735_OutString("       Termine");
           ST7735_SetCursor(1, 7);
           ST7735_OutString("   SCORE:");
           ST7735_SetCursor(7,10);
           for(uint32_t i; i < 6; i++){
               if(life[i] == 0){
                   Score = Score + 30;
               }
           }
            for(uint32_t i=7; i<=18; i++){
                if(life[i] == 0){
                    Score = Score + 20;
                }
            }
           for(uint32_t i=19; i<=30; i++){
               if(life[i] == 0){
                   Score = Score + 10;
               }
           }
           printf("%d",Score);
           break;
           }
       }
       break;
       }
       }
}

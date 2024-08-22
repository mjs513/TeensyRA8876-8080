#include "RA8876_Config_8080.h"
#include <MemoryHexDump.h>
#include "Teensy41_Cardlike.h"
#include "flexio_teensy_mm.c"
#include "teensy41.c"
#include <RA8876_t41_p.h>

RA8876_t41_p tft = RA8876_t41_p(RA8876_8080_DC,RA8876_8080_CS,RA8876_8080_RESET);

uint32_t start = 0;
uint32_t end =  0;

volatile bool ASYNC_frame_active = false;
void ASYNC_frame_complete_callback() {
//    Serial.println("\n*** ASYNC Frame Complete Callback ***");
    ASYNC_frame_active = false;
}

volatile bool DMA_frame_active = false;
void DMA_frame_complete_callback() {
//    Serial.println("\n*** DMA Frame Complete Callback ***");
    DMA_frame_active = false;
}

void setup() {
  //I'm guessing most copies of this display are using external PWM
  //backlight control instead of the internal RA8876 PWM.
  //Connect a Teensy pin to pin 14 on the display.
  //Can use analogWrite() but I suggest you increase the PWM frequency first so it doesn't sing.
#if defined(BACKLITE)
  pinMode(BACKLITE, OUTPUT);
  digitalWrite(BACKLITE, HIGH);
#endif

  Serial.begin(115200);
  while (!Serial && millis() < 1000) {} //wait for Serial Monitor
  if (CrashReport) {
    Serial.print(CrashReport);
    waitforInput();
  }

  // Set 8/16bit mode
  tft.setBusWidth(USE_8080_8_BIT_MODE);
  // DB5.0 WR pin, RD pin, D0 pin.
  tft.setFlexIOPins(RA8876_WR,RA8876_RD,RA8876_D0);
  tft.begin(BUS_SPEED); // 

  Serial.printf("%c RA8876 parallel 8080 mode testing (8Bit/16Bit ASYNC/DMA)\n\n",12);

  Serial.print("Bus speed: ");
  Serial.print(BUS_SPEED,DEC);
  Serial.println(" MHZ");
  Serial.print("Bus Width: ");
  Serial.print(tft.getBusWidth(),DEC);
  Serial.println("-bits");

  tft.graphicMode(true);
  tft.setRotation(0);

  tft.onCompleteCB(ASYNC_frame_complete_callback);
  tft.onDMACompleteCB(DMA_frame_complete_callback);
}

uint16_t pixel_data[4000];

void loop() {
#if 1 // ******** Set to 1 for Async testing, set to 0 for DMA testing. *********   
  tft.fillScreen(BLUE);
  ASYNC_frame_active = true;
  start = micros();
  tft.pushPixels16bitAsync(teensy41_Cardlike,10,10,575,424);
  while(ASYNC_frame_active) {}; //******** FIXED **********
  end = micros() - start;
  Serial.printf("Waiting for ASYNC transfer to complete took: %dus\n\n",end);

//  delay(250); // make sure
//  MemoryHexDump(Serial, teensy41_Cardlike, 128, true, "\nObject:\n", -1, 0);
//  tft.readRect(10, 10, 575, 2, pixel_data);
//  MemoryHexDump(Serial, pixel_data, 128, true, "\nReadBack:\n", -1, 0);

  waitforInput();
  tft.pushPixels16bitAsync(flexio_teensy_mm,0,0,480,320); // 480x320
  waitforInput();
  tft.pushPixels16bitAsync(teensy41,0,0,480,320); // 480x320
  waitforInput();
  tft.writeRect(10,10,575,424,teensy41_Cardlike);
  waitforInput();
  tft.writeRect(10,280,480,320,teensy41);
  waitforInput();
  tft.writeRect(530,0,480,320,flexio_teensy_mm);
  waitforInput();
#else
  tft.fillScreen(0x0010);
  start = micros();
  DMA_frame_active = true;
  tft.pushPixels16bitDMA(teensy41,1,1,480,320); // FLASHMEM buffer. 16-Bit bus width fails with bus speed
  while(DMA_frame_active) {}; //******** FIXED **********
                                                // above 12 MHZ. Causes distorted image. SDRAM buffer is ok.
  end = micros() - start;
  Serial.printf("Waiting for DMA transfer to complete took: %dus\n\n",end);
  waitforInput();

  tft.fillScreen(0x0010);
  start = micros();
  tft.pushPixels16bitDMA(teensy41_Cardlike,1,1,575,424); // FLASHMEM buffer. 16-Bit bus width fails with bus speed
                                                         // above 12 MHZ. Causes distorted image. SDRAM buffer is ok.
  end = micros() - start;
  Serial.printf("Rendered in %dus\n\n",end);
  waitforInput();

  tft.fillScreen(0x0010);
  start = micros();
  tft.pushPixels16bitDMA(flexio_teensy_mm,530,260,480,320); // FLASHMEM buffer. 16-Bit bus width fails with bus speed
                                                            // above 12 MHZ. Causes distorted image. SDRAM buffer is ok.
  end = micros() - start;
  Serial.printf("Rendered in %dus\n\n",end);
  waitforInput();
#endif
}

void waitforInput()
{
  Serial.println("Press anykey to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}

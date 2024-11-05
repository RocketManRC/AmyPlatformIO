/* 
  Play the PCM patches installed in Amy. For Arduino the default is 11 patches.
  This can be changed by modifying amy.c to include either pcm_small.h or pcm_large.h.

  amy.c is in the ".pio/AMY Synthesizer" folder.

  This example is provided under the MIT license in compliance with Amy. 

  MIT License

  Copyright (c) 2024 Rick MacDonald

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.  
*/

#include <AMY-Arduino.h>
#include <ESP_I2S.h>

extern const uint16_t pcm_samples; // this constant in Amy gives how many pcm patches there are

AMY amy;
I2SClass I2S;

/*
  Simple state machine called from loop() to play the PCM patches for 1 second each.

  Note that the default is for Amy to use pcm_tiny.h which is only 11 patches. 
*/
void pcmPatches() 
{
  static uint32_t start_ms = millis();
  static uint16_t patchId =  0;
  static uint8_t state = 0;

  if(patchId >= pcm_samples)
    return;

  switch(state)
  {
    case 0: // Send the patch using the wire protocol
    {
      char s[32];

      amy_reset_oscs();

      // osc = 0; wave = 7 (PCM), volume = 4, patchId = 0 to 11
      snprintf(s, sizeof(s), "v0w7l4p%dZ", patchId);
      
      amy_play_message(s);

      Serial.printf("PCM patch id: %d\n", patchId);

      state= 1;
    }
    break;

    case 1: // wait 1 second to go to next patch
    {
      if(millis() - start_ms > 1000)
        state = 2;
    }
    break;

    case 2: // wait 250 ms then next patch
    {
      if(millis() - start_ms > 1250)
      {
        // set to go to next patch
        start_ms = millis();
        patchId++;
        state = 0;
      }
    }
    break;
  }
}

void setupAmy()
{
  // Set your I2S pins. Data/SD/DIN/DOUT, SCK/BLCK, FS/WS/LRCLK. 
  //  int8_t bclk, int8_t ws, int8_t dout,
  I2S.setPins(21, 47, 38, -1, -1); // I recommend these pins for the I2S audio board
  I2S.begin(I2S_MODE_STD, AMY_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO); 

  // Start up AMY
  amy.begin(/* cores= */ 1, /* reverb= */ 0, /* chorus= */ 0, /* echo= */ 0);
}

void setup()
{
  Serial.begin(115200);  

  delay(2000); // Wait for the USB serial port to become active

  Serial.printf("Amy has %d PCM patches\n", pcm_samples);

  setupAmy();
}

void loop()
{
  pcmPatches(); // Note that pcmPatches() does not block.

  // Send Amy data to I2S device
  short * samples = amy.render_to_buffer();
  I2S.write((uint8_t*)samples, AMY_BLOCK_SIZE*AMY_NCHANS*AMY_BYTES_PER_SAMPLE);
}


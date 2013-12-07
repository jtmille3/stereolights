/*
  G35: An Arduino library for GE Color Effects G-35 holiday lights.
 Copyright © 2011 The G35 Authors. Use, modification, and distribution are
 subject to the BSD license as described in the accompanying LICENSE file.
 
 See README for complete attributions.
 */

#include <G35String.h>
#include <Math.h>

// Total # of lights on string (usually 50, 48, or 36). Maximum is 63, because
// the protocol uses 6-bit addressing and bulb #63 is reserved for broadcast
// messages.
#define LIGHT_COUNT (25)  // 2-25 works great.  1 thinks it's position 32?

// Audio middle even though it's 0-1024
#define EQUALIBRIUM (515) // this is an observered through monitoring audio "silence" 515

#define BARS (LIGHT_COUNT / 2) // Divide up the equalizer into bars

#define DEBUG (1) // print messages

// Arduino pin number. Pin 13 will blink the on-board LED.
#define G35_PIN (2)

G35String lights(G35_PIN, LIGHT_COUNT);

color_t color_array[LIGHT_COUNT];

int left_channel_bulb[BARS];
int left_channel_last_bulb;
int right_channel_bulb[BARS];
int right_channel_last_bulb;

void setup() 
{
  if(DEBUG) 
  {
    Serial.begin(9600);
  }

  lights.enumerate();
  lights.fill_color(0, 100, G35::MAX_INTENSITY, COLOR_BLACK);

  // rgb1, rgb2, start, length
  fill(0x0, 0xF, 0x0, 0x0, 0xF, 0x0, 9, 3);
  fill(0xF, 0x0, 0x0, 0x0, 0xF, 0x0, 3, 6);
  fill(0xF, 0x0, 0x0, 0xF, 0x0, 0x0, 0, 3);
  
  if(DEBUG) 
  {
    color_array[12] = COLOR_BLACK;  // middle light, easier to locate when black
  } 
  else 
  {
    color_array[12] = COLOR_GREEN;  // middle light
  }
  
  fill(0x0, 0xF, 0x0, 0x0, 0xF, 0x0, 13, 3);
  fill(0x0, 0xF, 0x0, 0xF, 0x0, 0x0, 16, 6);
  fill(0xF, 0x0, 0x0, 0xF, 0x0, 0x0, 22, 3);
  
  for(int i = 0; i < LIGHT_COUNT / 2; i++) 
  {
    // +1 offset from the middle because of odd bulbs
    left_channel_bulb[i] = LIGHT_COUNT / 2 + 1 - i;  
    left_channel_last_bulb = 0;
    right_channel_bulb[i] = LIGHT_COUNT / 2 + 1 + i;
    right_channel_last_bulb = 0;
  }
}

// control the equalizer through intensity 0x00 to 0xcc
void loop() 
{
  int sampleCount = 20;
  int highAmp0 = 0;
  int highAmp1 = 0;
  while(sampleCount--) 
  {
    int a0value = analogRead(A0);
    int a1value = analogRead(A1);
    int amp0 = getAmplitude(a0value);
    int amp1 = getAmplitude(a1value);
    
    if(amp0 > highAmp0) 
    {
      highAmp0 = amp0;
    }
    
    if(amp1 > highAmp1) 
    {
      highAmp1 = amp1;
    }
  }

  debugAmplitudes(highAmp0, highAmp1);

  int bars0 = getBars(highAmp0);
  int bars1 = getBars(highAmp1);
  updateLights(bars0, bars1);

  delayMicroseconds(100); // delay in between reads for stability
}

void updateLights(int left_bars, int right_bars) 
{
  for(int i = 0; i < LIGHT_COUNT; i++) {
    if(left_bars + i > BARS && i < BARS) { // assume counting from the middle (13) to the right (25)
      lights.fill_color(i, 1, G35::MAX_INTENSITY, color_array[i]);
    } 
    else if(right_bars + BARS > i && i > BARS) { // assume counting from the middle (11) to the left (0)
      lights.fill_color(i, 1, G35::MAX_INTENSITY, color_array[i]);
    } 
    else 
    {
      // fade out the lights that are turned on.
      // wait until the intensity is half before turning off the next one and so on.
      lights.fill_color(i, 1, 0x0, color_array[i]);
    }
  }
  
  return;
  
  if(left_bars > left_channel_last_bulb) // rise
  {
    for(int j = 0; j < BARS; j++) 
    {
      if(left_bars <= j) 
      {
        lights.fill_color(left_channel_bulb[j], 1, G35::MAX_INTENSITY, color_array[j]);
      }
    }
  }
  else // decay
  {
    lights.fill_color(left_channel_bulb[left_channel_last_bulb], 1, 0, color_array[left_channel_last_bulb]);
    if(left_channel_last_bulb - 1 > 0)
      lights.fill_color(left_channel_bulb[left_channel_last_bulb - 1], 1, 0xCC/2, color_array[left_channel_last_bulb - 1]);
    if(left_channel_last_bulb - 2 > 0)
      lights.fill_color(left_channel_bulb[left_channel_last_bulb - 2], 1, 0xCC/4, color_array[left_channel_last_bulb - 2]);
    
    delay(20); // slowly decay
  }
  
  if(right_bars > right_channel_last_bulb) // rise
  {
    for(int j = 0; j < BARS; j++) 
    {
      if(right_bars <= j) 
      {
        lights.fill_color(right_channel_bulb[j], 1, G35::MAX_INTENSITY, color_array[j]);
      }
    }
  }
  else // decay
  {
    lights.fill_color(right_channel_bulb[right_channel_last_bulb], 1, 0, color_array[right_channel_last_bulb]);
    if(right_channel_last_bulb - 1 > 0)
      lights.fill_color(right_channel_bulb[right_channel_last_bulb - 1], 1, 0xCC/2, color_array[right_channel_last_bulb - 1]);
    if(right_channel_last_bulb - 2 > 0)
      lights.fill_color(right_channel_bulb[right_channel_last_bulb - 2], 1, 0xCC/4, color_array[right_channel_last_bulb - 2]);
      
    delay(20); // slowly decay
  }
  
  left_channel_last_bulb = left_bars;
  right_channel_last_bulb = right_bars;
}

void fill(int red1, int green1, int blue1, int red2, int green2, int blue2, int start, int frames)
{
  echo("Start: ");
  echo(start);
  echoln("");

  for(int frame = 0; frame < frames; frame++) 
  {
    float ratio = (float) frame / (float) frames;
    int red = (int) ceil((red2 * ratio + red1 * (1 - ratio)));
    int green = (int) ceil((green2 * ratio + green1 * (1 - ratio)));
    int blue = (int) ceil((blue2 * ratio + blue1 * (1 - ratio)));

    echo("rgb: (");
    echo(red);
    echo(", ");
    echo(green);
    echo(", ");
    echo(blue);
    echo(")");
    echoln("");

    echo("Into color_array position ");
    echo(start + frame);
    echoln("");
    color_array[start + frame] = G35::color(red, green, blue);
  }
}

long microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

void echo(char string[]) 
{
  if(DEBUG) 
  {
    Serial.print(string);
  }
}

void echo(int number) 
{
  if(DEBUG) 
  {
    Serial.print(number);
  }
}

void echoln(char string[]) 
{
  if(DEBUG)
  {
    Serial.println(string);
  }
}

void get_rgb(color_t color, int rgb[]) 
{
  rgb[0] = get_red(color);
  rgb[1] = get_green(color);
  rgb[2] = get_blue(color);
}

uint8_t get_blue(color_t color) 
{
  return (color >> 0) & 0xFF;
}

uint8_t get_green(color_t color) 
{
  return (color >> 8) & 0xFF;
}

uint8_t get_red(color_t color) 
{
  return (color >> 16) & 0xFF;
}

int getAmplitude(int sensorValue) 
{
  return abs(sensorValue - EQUALIBRIUM);
}

void debugAmplitudes(int amp0, int amp1) 
{
  if(DEBUG)
  {
    int bars0 = getBars(amp0);  
    echoln("");
    for(int i = BARS; i != 0; i--) {
      if(bars0 > i) {
        echo("<");
      } 
      else {
        echo(" ");
      }
    }

    echo(" | ");

    int bars1 = getBars(amp1);
    for(int j = 0; j < BARS; j++) {
      if(bars1 > j) {
        echo(">");
      } 
      else {
        echo(" ");
      }
    }

    echo(" (");
    echo(amp0);
    echo(",");
    echo(amp1);
    echo(")");
  }
}

// based on the amplitude and our equalibrium return the correct number of bars
int getBars(int amplitude) 
{
  return round(amplitude * BARS / EQUALIBRIUM);
}



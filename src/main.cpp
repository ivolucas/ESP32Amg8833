#include <Wire.h>
#include "Arduino.h"
#include <SPI.h>
#include <TFT_eSPI.h>

#include <Adafruit_AMG88xx.h>
#include <Adafruit_GFX.h>    // core graphics library
#include <Adafruit_ST7735.h> // hardware-specific library
#include <EasyButtonTouch.h>

// Option 1: use any pins but a little slower

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite msg = TFT_eSprite(&tft);

#define TA_SHIFT 8 // Default shift for MLX90640 in open air
#define COLS 8
#define ROWS 8
#define IMG_COLS 64
#define IMG_ROWS 64

#define SPRITE_IMG_COLS 128
#define SPRITE_IMG_ROWS 128

#define irCameraArraySize (COLS * ROWS)

#define imageArraySize (IMG_COLS * IMG_ROWS)

float pixels_ir[COLS * ROWS];
float pixels_img[IMG_COLS * IMG_ROWS];

void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols,
                       float *dest, uint8_t dest_rows, uint8_t dest_cols);

float get_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y);

Adafruit_AMG88xx amg;

// low range of the sensor (this will be blue on the screen)
float mintemp = 24;    // For color mapping
float min_v = 24;      // Value of current min temp
float min_cam_v = -40; // Spec in datasheet

// high range of the sensor (this will be red on the screen)
float maxtemp = 35;    // For color mapping
float max_v = 35;      // Value of current max temp
float max_cam_v = 300; // Spec in datasheet

long constrainMap(float x, float in_min, float in_max, long out_min, long out_max) {
    const long dividend = out_max - out_min;
    const float divisor = in_max - in_min;
    const float delta = x - in_min;

    return constrain( (long)((delta * (float)dividend + (divisor / 2.0)) / divisor + out_min), out_min,out_max);
}


// the colors we will be using
const uint16_t camColors[] = {
    0x480F,
    0x400F,
    0x400F,
    0x400F,
    0x4010,
    0x3810,
    0x3810,
    0x3810,
    0x3810,
    0x3010,
    0x3010,
    0x3010,
    0x2810,
    0x2810,
    0x2810,
    0x2810,
    0x2010,
    0x2010,
    0x2010,
    0x1810,
    0x1810,
    0x1811,
    0x1811,
    0x1011,
    0x1011,
    0x1011,
    0x0811,
    0x0811,
    0x0811,
    0x0011,
    0x0011,
    0x0011,
    0x0011,
    0x0011,
    0x0031,
    0x0031,
    0x0051,
    0x0072,
    0x0072,
    0x0092,
    0x00B2,
    0x00B2,
    0x00D2,
    0x00F2,
    0x00F2,
    0x0112,
    0x0132,
    0x0152,
    0x0152,
    0x0172,
    0x0192,
    0x0192,
    0x01B2,
    0x01D2,
    0x01F3,
    0x01F3,
    0x0213,
    0x0233,
    0x0253,
    0x0253,
    0x0273,
    0x0293,
    0x02B3,
    0x02D3,
    0x02D3,
    0x02F3,
    0x0313,
    0x0333,
    0x0333,
    0x0353,
    0x0373,
    0x0394,
    0x03B4,
    0x03D4,
    0x03D4,
    0x03F4,
    0x0414,
    0x0434,
    0x0454,
    0x0474,
    0x0474,
    0x0494,
    0x04B4,
    0x04D4,
    0x04F4,
    0x0514,
    0x0534,
    0x0534,
    0x0554,
    0x0554,
    0x0574,
    0x0574,
    0x0573,
    0x0573,
    0x0573,
    0x0572,
    0x0572,
    0x0572,
    0x0571,
    0x0591,
    0x0591,
    0x0590,
    0x0590,
    0x058F,
    0x058F,
    0x058F,
    0x058E,
    0x05AE,
    0x05AE,
    0x05AD,
    0x05AD,
    0x05AD,
    0x05AC,
    0x05AC,
    0x05AB,
    0x05CB,
    0x05CB,
    0x05CA,
    0x05CA,
    0x05CA,
    0x05C9,
    0x05C9,
    0x05C8,
    0x05E8,
    0x05E8,
    0x05E7,
    0x05E7,
    0x05E6,
    0x05E6,
    0x05E6,
    0x05E5,
    0x05E5,
    0x0604,
    0x0604,
    0x0604,
    0x0603,
    0x0603,
    0x0602,
    0x0602,
    0x0601,
    0x0621,
    0x0621,
    0x0620,
    0x0620,
    0x0620,
    0x0620,
    0x0E20,
    0x0E20,
    0x0E40,
    0x1640,
    0x1640,
    0x1E40,
    0x1E40,
    0x2640,
    0x2640,
    0x2E40,
    0x2E60,
    0x3660,
    0x3660,
    0x3E60,
    0x3E60,
    0x3E60,
    0x4660,
    0x4660,
    0x4E60,
    0x4E80,
    0x5680,
    0x5680,
    0x5E80,
    0x5E80,
    0x6680,
    0x6680,
    0x6E80,
    0x6EA0,
    0x76A0,
    0x76A0,
    0x7EA0,
    0x7EA0,
    0x86A0,
    0x86A0,
    0x8EA0,
    0x8EC0,
    0x96C0,
    0x96C0,
    0x9EC0,
    0x9EC0,
    0xA6C0,
    0xAEC0,
    0xAEC0,
    0xB6E0,
    0xB6E0,
    0xBEE0,
    0xBEE0,
    0xC6E0,
    0xC6E0,
    0xCEE0,
    0xCEE0,
    0xD6E0,
    0xD700,
    0xDF00,
    0xDEE0,
    0xDEC0,
    0xDEA0,
    0xDE80,
    0xDE80,
    0xE660,
    0xE640,
    0xE620,
    0xE600,
    0xE5E0,
    0xE5C0,
    0xE5A0,
    0xE580,
    0xE560,
    0xE540,
    0xE520,
    0xE500,
    0xE4E0,
    0xE4C0,
    0xE4A0,
    0xE480,
    0xE460,
    0xEC40,
    0xEC20,
    0xEC00,
    0xEBE0,
    0xEBC0,
    0xEBA0,
    0xEB80,
    0xEB60,
    0xEB40,
    0xEB20,
    0xEB00,
    0xEAE0,
    0xEAC0,
    0xEAA0,
    0xEA80,
    0xEA60,
    0xEA40,
    0xF220,
    0xF200,
    0xF1E0,
    0xF1C0,
    0xF1A0,
    0xF180,
    0xF160,
    0xF140,
    0xF100,
    0xF0E0,
    0xF0C0,
    0xF0A0,
    0xF080,
    0xF060,
    0xF040,
    0xF020,
    0xF800,
};

void drawpixels(float *p, uint8_t rows, uint8_t cols)
{

  int yS = SPRITE_IMG_COLS / rows;
  int xS = SPRITE_IMG_ROWS / cols;

  for (int y = 0, yI = 0; y < rows; y++, yI += yS)
  {
    for (int x = 0, xI = 0; x < cols; x++, xI += xS)
    {

      float colorTemp = get_point(p, rows, cols, x, y);     

      uint8_t colorIndex = constrainMap(colorTemp, mintemp, maxtemp, 0, 255);
      // draw the pixels!
      img.fillRect(xI, yI, xS, yS, camColors[colorIndex]);
    }
  }

  img.drawCircle(SPRITE_IMG_COLS / 2, SPRITE_IMG_ROWS / 2, 5, TFT_WHITE);                                              // update center spot icon
  img.drawLine(SPRITE_IMG_COLS / 2 - 8, SPRITE_IMG_ROWS / 2, SPRITE_IMG_COLS / 2 + 8, SPRITE_IMG_ROWS / 2, TFT_WHITE); // vertical line
  img.drawLine(SPRITE_IMG_COLS / 2, SPRITE_IMG_ROWS / 2 - 8, SPRITE_IMG_COLS / 2, SPRITE_IMG_ROWS / 2 + 8, TFT_WHITE); // horizontal line
  img.setCursor(SPRITE_IMG_COLS / 2 + 6, SPRITE_IMG_ROWS / 2 - 12);

  img.pushSprite(0, 0);
 
}

void drawText(float *p, uint8_t rows, uint8_t cols)
{

  msg.fillScreen(TFT_BLACK);
  msg.setTextFont(1);
  msg.setTextColor(TFT_WHITE);
  msg.setCursor(13, 4);
  msg.printf("%.2fC", get_point(p, rows, cols, cols / 2, rows / 2));
  msg.setTextColor(TFT_YELLOW);
  msg.setCursor(11, 16);
  msg.print("min tmp");
  msg.setCursor(13, 28);
  msg.printf("%.2fC", min_v);
  msg.setCursor(11, 39);
  msg.print("max tmp");
  msg.setCursor(13, 51);
  msg.printf("%.2fC", max_v);
  msg.pushSprite(64, 0, TFT_BLACK);
}

void drawScale(){
  for (int icol = 0; icol <= 127; icol++)
  {
    tft.drawRect(icol, 130, 1, 30, camColors[icol * 2]);
  }
  
  tft.setTextFont(4);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0,136);
  tft.printf("%d",(int)mintemp);
  tft.setCursor(94,136);
  tft.printf("%3d",(int)maxtemp);
}

// Instance of the button.
EasyButtonTouch button(27);

// Callback function to be called when the button is pressed.
void onPressed()
{
  Serial.println("Button has been pressed");
  mintemp = floorf (min_v);
  maxtemp = ceilf (max_v);
  
  drawScale();
}

void setup()
{
  Serial.begin(9600);
  tft.init(INITR_BLACKTAB);

  // Increase I2C clock speed to 450kHz
  Wire.begin(SDA, SCL, 450000);

  // use show tmp bitmap
  img.createSprite(SPRITE_IMG_COLS, SPRITE_IMG_ROWS);
  // use show tmp data
  msg.createSprite(64, 64);

  Serial.println("M5StickC AMG8833 IR Camera");

  // Get device parameters - We only have to do this once
  bool status;
  // default settings
  status = amg.begin();
  if (!status)
  {
    Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
    while (1)
      ;
  }

  // Display bottom side colorList and info
  tft.fillScreen(TFT_BLACK);

  drawScale();

  // Initialize the button.
  button.begin();
  // Add the callback function to be called when the button is pressed.
  button.onPressed(onPressed);
}

void loop()
{
  long t_read = millis();
  //read all the pixels
  amg.readPixels(pixels_ir);
  t_read = millis() - t_read;

  max_v = mintemp;
  min_v = maxtemp;

  for (int itemp = 0; itemp < irCameraArraySize; itemp++)
  {
    if (pixels_ir[itemp] > max_v)
    {
      max_v = pixels_ir[itemp];
    }
    if (pixels_ir[itemp] < min_v)
    {
      min_v = pixels_ir[itemp];
    }
  }

  long t_inter = millis();
  // cover pixels to pixels_3
  interpolate_image(pixels_ir, COLS, ROWS, pixels_img, IMG_COLS, IMG_ROWS);
  t_inter = millis() - t_inter;

  // show tmp image
  long t_draw = millis();
  drawpixels(pixels_img, IMG_ROWS, IMG_COLS);
  drawText(pixels_img, IMG_ROWS, IMG_COLS);
  t_draw = millis() - t_draw;
  
  // Continuously read the status of the button.
  button.read();
}

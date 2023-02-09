#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <MAX30105.h>
#include <heartRate.h>

MAX30105 particleSensor;
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, 10, 8);

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
long currentMillis;
long initMillis;

float beatsPerMinute;
int beatAvg;

int BPM;
int AVG;

uint8_t button_jump = 4;
int highscore = 0;  //highscore is reserved

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DINO_WIDTH 4
#define DINO_WIDTH_PX 25
#define DINO_HEIGHT 26
#define DINO_INIT_X 10
#define DINO_INIT_Y 29

#define BASE_LINE_X0 0
#define BASE_LINE_Y0 54
#define BASE_LINE_X1 127
#define BASE_LINE_Y1 54

#define TREE1_WIDTH 2
#define TREE1_WIDTH_PX 11
#define TREE1_HEIGHT 23
#define TREE2_WIDTH 3
#define TREE2_WIDTH_PX 22
#define TREE2_HEIGHT 23
#define TREE_Y 35

#define JUMP_PIXEL 22

/*bitmaps*/
static const unsigned char dino1[] = {
  // 'dino', 25x26px
0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x06, 0xff, 0x00, 0x00, 0x0e, 0xff, 0x00, 
0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0x0f, 0xc0, 0x00, 
0x00, 0x0f, 0xfc, 0x00, 0x40, 0x0f, 0xc0, 0x00, 0x40, 0x1f, 0x80, 0x00, 0x40, 0x7f, 0x80, 0x00, 
0x60, 0xff, 0xe0, 0x00, 0x71, 0xff, 0xa0, 0x00, 0x7f, 0xff, 0x80, 0x00, 0x7f, 0xff, 0x80, 0x00, 
0x7f, 0xff, 0x80, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 
0x03, 0xfc, 0x00, 0x00, 0x01, 0xdc, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 0x01, 0x8c, 0x00, 0x00, 
0x01, 0x0c, 0x00, 0x00, 0x01, 0x8e, 0x00, 0x00
};

static const unsigned char tree1[] = {
  // 'tree1', 11x23px
0x1e, 0x00, 0x1f, 0x00, 0x1f, 0x40, 0x1f, 0xe0, 0x1f, 0xe0, 0xdf, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 
0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xe0, 0xff, 0xc0, 0xff, 0x00, 0xff, 0x00, 0x7f, 0x00, 
0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x1f, 0x00
};

static const unsigned char tree2[] = {
  // 'tree2', 22x23px
0x1e, 0x01, 0xe0, 0x1f, 0x03, 0xe0, 0x1f, 0x4f, 0xe8, 0x1f, 0xff, 0xfc, 0x1f, 0xff, 0xfc, 0xdf, 
0xff, 0xfc, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfc, 0xff, 0xff, 
0xfc, 0xff, 0xef, 0xfc, 0xff, 0x83, 0xfc, 0xff, 0x03, 0xfc, 0xff, 0x03, 0xf8, 0x7f, 0x03, 0xe0, 
0x1f, 0x03, 0xe0, 0x1f, 0x03, 0xe0, 0x1f, 0x03, 0xe0, 0x1f, 0x03, 0xe0, 0x1f, 0x03, 0xe0, 0x1f, 
0x03, 0xe0, 0x1f, 0x03, 0xe0
};

/*game*/

void setup()
{
    u8g2.begin();
    u8g2.clearDisplay();
    u8g2.setDrawColor(1);
    u8g2.setBitmapMode(1);
    u8g2.enableUTF8Print();

    //particleSensor.setup();
    //particleSensor.setPulseAmplitudeRed(0x0A);
    //particleSensor.setPulseAmplitudeGreen(0);

    Serial.begin(9600);
    choiceMenu();

    while(1)
    {

        if(digitalRead(button_jump) == HIGH)
        {   
            delay(300); //prevent instant jumping
                prepare();
                for(;;)
                {   
                    float initMillis;
                    float currentMillis = millis();
                    float initInterval = 5000;

                    if((currentMillis - initMillis) > initInterval)
                    {
                      play();
                      initMillis = currentMillis;
                      break;
                    }
                }
        }
    }
}


void choiceMenu()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_pressstart2p_8f);
        u8g2.setCursor(10,20);
        u8g2.println("Dino Game");

        u8g2.setFont(u8g2_font_trixel_square_tr);
        u8g2.setCursor(10,45);
        u8g2.println("Press X To Play!");
        u8g2.display();
    }   while(u8g2.nextPage());
}

void prepare()
{   //in here we should display countdown and calibrate the sensor
    initMillis = millis();
    u8g2.firstPage();
    do
    {   
        u8g2.setFont(u8g2_font_trixel_square_tr);
        u8g2.setCursor(10,45);
        u8g2.println("Initializing Sensor...");
        u8g2.display();
    }   while(u8g2.nextPage());
}

void play()
{
    uint16_t tree_x = 127;
    uint16_t tree1_x = 195;

    uint8_t tree_type = random(0,2);
    uint8_t tree_type1 = random(0,2);

    uint16_t dino_y = DINO_INIT_Y;

    uint8_t jump = 0;
    int score = 0;
    float f_score = 0;
    /*highscore is near headers*/

    float multiplier = 1;

    unsigned long startMillis = millis();

    float previousMillis = 0;
    float interval = 50;
    float newInterval = interval;

    for(;;)
    {   
        if(digitalRead(button_jump) == HIGH)
        {
            if(jump == 0)
            {
                jump = 1;
            }
        }

        if(jump == 1)
        {
            dino_y--;
            if(dino_y == (DINO_INIT_Y-JUMP_PIXEL))
            {
                jump = 2;
            }
        }
        else if(jump == 2)
        {
            dino_y++;
            if(dino_y == (DINO_INIT_Y))
            {
                jump = 0;
            }
        }

        /*score*/
        unsigned long gameMillis = millis();
        score = ((gameMillis-startMillis)/100);
        f_score = score;

        if(score > highscore)
        {
          highscore = score;
        }

        if(tree_x <= (DINO_INIT_X+DINO_WIDTH_PX-8) && tree_x >= (DINO_INIT_X + ((DINO_WIDTH_PX-8)/2)))
            if(dino_y >= (DINO_INIT_Y-3))
            {
                gameEnd(score, highscore);
                break;
            }
        
        if(tree1_x <= (DINO_INIT_X+DINO_WIDTH_PX-8) && tree_x >= (DINO_INIT_X + ((DINO_WIDTH_PX-8)/2)))
            if(dino_y >= (DINO_INIT_Y-3))
            {   
                gameEnd(score, highscore);
                break;
            }
        
        u8g2.firstPage();
        do
        {   
            /*score*/
            u8g2.setFont(u8g2_font_trixel_square_tr);
            u8g2.setCursor(64,10);
            u8g2.print("Score: ");
            u8g2.print(score);

            moveTree(&tree_x, tree_type);
            moveTree(&tree1_x, tree_type1);
            moveDino(&dino_y);
            u8g2.drawLine(0,54,127,54);

            /*score multiplier x stress multiplier*/
            //if(score>100)
            //{
            //  multiplier = score/100;
            //}
            //interval_mx = interval/multiplier;
            //!!! OR !!!
            //find way to reduce scoring with multiplier (not very exciting) 

            float trigger = 50;
            if(f_score >= trigger)
            {
              multiplier = 1+((f_score/trigger)/8);
              newInterval = interval/multiplier;

              //multiplier max = 2
              if(multiplier >= 2)
              {
                multiplier = 2;
              }
            }

            float currentMillis = millis();

            if((currentMillis - previousMillis) > newInterval)
            {
              tree_x--;
              tree1_x--;
              previousMillis = currentMillis;
            }

            int score = (int)f_score;

            if(tree_x == 0)
            {
                tree_x = 128;
                tree_type = random(0,2);
            }

            if(tree1_x == 0)
            {
                tree1_x = 255;
                tree_type1 = random(0,2);
            }
            u8g2.display();
        }   while(u8g2.nextPage());
    }   
}

void gameEnd(int score, int highscore)
{
    u8g2.clearDisplay();
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_pressstart2p_8f);
        u8g2.setCursor(10,20);
        u8g2.println("Game Over");

        u8g2.setFont(u8g2_font_trixel_square_tr);
        u8g2.setCursor(10,35);
        u8g2.print("Score: ");
        u8g2.print(score);

        /*highscore*/
        u8g2.setCursor(50,35);
        u8g2.print("Highscore: ");
        u8g2.print(highscore);

        u8g2.setCursor(10,45);
        u8g2.println("Press X To Play Again!");
        u8g2.display();
    }   while(u8g2.nextPage());
}

void moveTree(int16_t *x, int type)
{
    if(type == 0)
    {
        u8g2.drawBitmap(*x, TREE_Y, TREE1_WIDTH, TREE1_HEIGHT, tree1);
    }
    else if(type == 1)
    {
        u8g2.drawBitmap(*x, TREE_Y, TREE2_WIDTH, TREE2_HEIGHT, tree2);
    }
}

void moveDino(int16_t *y)
{
    u8g2.drawBitmap(DINO_INIT_X, *y, DINO_WIDTH, DINO_HEIGHT, dino1);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  unsigned long currentTime = millis();

  static unsigned long lastTimeSerialOutput = 0;
  const unsigned long serialOutputInterval = 1000;  ///* !!!THIS IS THE DELAY!!! *///
  if (currentTime - lastTimeSerialOutput >= serialOutputInterval)
  {
    lastTimeSerialOutput = currentTime;
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);

    if (irValue < 50000)
      Serial.print(" No finger?");

    Serial.println();
  }
}

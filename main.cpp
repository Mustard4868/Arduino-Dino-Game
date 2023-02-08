#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, 10, 8);

uint8_t button_jump = 4;

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

    choiceMenu();

    while(1)
    {
        if(digitalRead(button_jump) == HIGH)
        {
            prepare();
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
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_trixel_square_tn);
        u8g2.setCursor(50,30);
        /*display countdown from 3*/
        play();
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
    uint8_t score = 0;
    uint8_t highscore = 0;

    for(;;)
    {   
        delay(1);
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
                score++;
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

        if(tree_x <= (DINO_INIT_X+DINO_WIDTH_PX) && tree_x >= (DINO_INIT_X + (DINO_WIDTH_PX/2)))
            if(dino_y >= (DINO_INIT_Y-3))
            {
                gameEnd(score);
                break;
            }
        
        if(tree1_x <= (DINO_INIT_X+DINO_WIDTH_PX) && tree_x >= (DINO_INIT_X + (DINO_WIDTH_PX/2)))
            if(dino_y >= (DINO_INIT_Y-3))
            {   
                gameEnd(score);
                break;
            }
        
        u8g2.firstPage();
        do
        {   
            moveTree(&tree_x, tree_type);
            moveTree(&tree1_x, tree_type1);
            moveDino(&dino_y);
            u8g2.drawLine(0,54,127,54);

            tree_x--;
            tree1_x--;

            if(tree_x == 0)
            {
                tree_x = 127;
                tree_type = random(0,2);
            }

            if(tree1_x == 0)
            {
                tree1_x = 195;
                tree_type1 = random(0,2);
            }
            u8g2.display();
        }   while(u8g2.nextPage());
    }   
}

void gameEnd(int score)
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

        /*highscore
        u8g2.setCursor(10,40);
        u8g2.print("Highscore: ");
        u8g2.print(highscore);*/

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

void loop()
{

}

void displayScore(int score)
{
    u8g2.setFont(u8g2_font_trixel_square_tr);
    u8g2.setCursor(64,10);
    u8g2.print("Score: ");
    u8g2.print(score);
}

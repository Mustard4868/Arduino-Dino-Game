#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, 10, 8);

//*button config*//
uint8_t button_x = 2; //jump
uint8_t button_y = 4; //duck

#define DG_FP 4

//*object types*//
struct _dg_ot_struct
{
    uint8_t player_mask;
    uint8_t hit_mask;
    uint8_t points;
    uint8_t draw_fn;
    uint8_t jump_fn;
    uint8_t duck_fn;
    uint8_t is_hit_fn;
};
typedef struct _dg_ot_struct dg_ot;

//*objects visible to player*//
struct _dg_obj_struct
{
    uint8_t ot;
    int8_t tmp;
    int16_t x,y;
    int8_t x0,y0,x1,y1;
};
typedef struct _dg_obj_struct dg_obj;

#define DG_DRAW_NONE 0
#define DG_DRAW_BBOX 1
#define DG_DRAW_PLAYER 2
#define DG_DRAW_CACTUS_NORM 3
#define DG_DRAW_CACTUS_WIDE 4
#define DG_DRAW_CACTUS_HIGH 5
#define DG_DRAW_BIRD 6

#define DG_MOVE_X_SPEED 0
#define DG_JUMP_PLAYER 1
#define DG_DUCK_PLAYER 2

#define DG_IS_HIT_NONE 0
#define DG_IS_HIT_BBOX 1
#define DG_IS_HIT_WALL 2

#define DG_OT_WALL_SOLID 1
#define DG_OT_CACTUS_NORM 2
#define DG_OT_PLAYER 3
#define DG_OT_CACTUS_WIDE 4
#define DG_OT_CACTUS_HIGH 5
#define DG_OT_BIRD 6

//*graphics object*//
u8g2_t *dg_u8g2;
u8g2_uint_t u8g_height_minus_one;
#define AREA_HEIGHT(dg_u8g2->height-8)
#define AREA_WIDTH(dg_u8g2->width)

//*object types*//
const dg_ot dg_object_types[] U8X8_PROGMEM = 
{
    /*0: Empty object type*/
    {0,0,0, DG_DRAW_NONE, DG_JUMP_NONE, DG_DUCK_NONE, DG_IS_HIT_NONE},
};

//*list of all objects on screen*//
#if RAMEND < 0x300
#define DG_OBJ_CNT 25
#else
#define DG_OBJ_CNT 60
#endif

dg_obj dg_objects[DG_OBJ_CNT];

//*about player model*//
uint8_t dg_player_pos;
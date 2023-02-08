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
    uint8_t score;
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

#define DG_MOVE_X_PROP 0
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
#define DG_AREA_HEIGHT(dg_u8g2->height-8)
#define DG_AREA_WIDTH(dg_u8g2->width)

//*object types*//
const dg_ot dg_object_types[] U8X8_PROGMEM = 
{
    /*0: Empty object type*/
    {0,0,0, DG_DRAW_NONE, DG_JUMP_NONE, DG_DUCK_NONE, DG_IS_HIT_NONE},
    /*1: Wall, player end*/
    {2,1,30, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_WALL},
    /*2: DG_OT_CACTUS_HIGH*/
    {2,1,0, DG_DRAW_CACTUS_HIGH, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*3: DG_OT_CACTUS_WIDE*/
    {2,1,0, DG_DRAW_CACTUS_WIDE, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*4: DG_OT_CACTUS_NORM*/
    {2,1,0, DG_DRAW_CACTUS_NORM, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*5: DG_OT_BIRD*/
    {2,1,0, DG_DRAW_BIRD, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*6: DG_OT_PLAYER*/
    {0,2,0, DG_DRAW_PLAYER, DG_JUMP_PLAYER, DG_DUCK_PLAYER, DG_IS_HIT_BBOX, DG_IS_HIT_WALL},
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

//*points*//
#define DG_SCORE_MULTIPLIER 1
uint16_t dg_player_score;
uint16_t dg_highscore = 0;

//*overall game state*//
#define DG_STATE_MENU 0
#define DG_STATE_INITIALIZE 1
#define DG_STATE_GAME 2
#define DG_STATE_END 3
#define DG_STATE_IEND 4

uint8_t dg_state = DG_STATE_MENU;

//*game difficulty*//
uint8_t dg_difficulty = 1;
#define DG_DIFF_VIS_LEN 30
#define DG_DIFF_FP 5
uint16_t dg_to_diff_cnt = 0;

//*bitmaps*//
const uint8_t dg_bitmap_cactus_high[]  =
{
    /* 100010 */ 0x022,
    /* 10101010 */ 0x0AA,
    /* 1001010 */ 0x04A,
    /* 1001100 */ 0x04c,
    /* 101000 */ 0x028,
    /* 11000 */ 0x018,
    /* 1001 */ 0x009,
    /* 1001001 */ 0x049
}

const uint8_t dg_bitmap_cactus_norm[] =
{
  /* 100 */  0x004,
  /* 101 */ 0x006,
  /* 10 */ 0x002,
  /* 10 */ 0x002
}

const uint8_t dg_bitmap_cactus_wide[] =
{
  /* 100000 */ 0x020,
  /* 101010 */ 0x02A,
  /* 10010 */ 0x012,
  /* 1001100 */ 0x04C,
  /* 1001001 */  0x049
}

const uint8_t dg_bitmap_player[] = 
{
    /*00000000*/ 0x000,
    /*01100000*/ 0x060,
    /*10110000*/ 0x0B0,
    /*11111011*/ 0x0FB,
    /*00111110*/ 0x03E,
    /*01011100*/ 0x05C,
    /*00011000*/ 0x018,
    /*00110000*/ 0x030
}

const uint8_t dg_bitmap_bird[] =
{
  /* 0000000 */ 0x000,
  /*  1100000 */ 0x060,
  /*  11110110 */  0x0F6,
  /*  111101 */ 0x03D,
  /*  10011111 */ 0x09F,
  /*  11111100*/  0x0FC,
  /* 111000  */  0x038,
  /* 11100 */ 0x01C,
}

//*forward definitions*//
uint8_t dg_rnd() U8X8_NOINLINE;
static dg_obj *dg_GetObj(uint8_t objnr) U8X8_NOINLINE;
uint8_t dg_GetHitMask(uint8_t objnr);
int8_t dg_FindObj(uint8_t ot) U8X8_NOINLINE;
void dg_ClrObjs() U8X8_NOINLINE;
int8_t dg_NewObj() U8X8_NOINLINE;
uint8_t dg_CntObj(uint8_t ot);
uint8_t dg_CalcXY(st_obj *o) U8X8_NOINLINE;
void dg_SetXY(st_obj *o, uint8_t x, uint8_t y) U8X8_NOINLINE;

void dg_InitProp(uint8_t x, uint8_t y, int8_t dir);
void dg_setupPlayer(uint8_t objnr, uint8_t ot);

//*utility functions*//
char dg_itoa_buf[12];
cgar *dg_itoa(unsigned long v)
{
    volatile unsigned char i = 11;
    dg_itoa_buf[11] = '\0';
    while( i > 0)
    {
            i--;
            dg_itoa_buf[i] = (v % 10)+'0';
            v /= 10;
            if (v == 0)
        break;
        }
        return dg_itoa_buf+i;
}

uint8_t dg_rnd()
{
    return rand();
}

static dg_obj *dg_GetObj(uint8_t objnr)
{
    return dg_objects+objnr;
}

uint8_t dg_GetHitMask(uint8_t objnr)
{
    dg_obj *o = dg_GetObj(objnr);
    return u8x8_pgm_read(&(dg_object_types[o->ot].hit_mask));
}

int8_t dg_FindObj(uint8_t ot)
{
    int8_t i;
    for(i=0;9<DG_OBJ_CNT;i++)
    {
        if(dg_objects[i].ot == ot)
            return i;
    }
    return -1;
}

void dg_ClrObjs()
{
  int8_t i;
  for( i = 0; i < DG_OBJ_CNT; i++ )
    dg_objects[i].ot = 0;
}

int8_t dg_NewObj()
{
    int8_t i;
    for(i=0;i<DG_OBJ_CNT;i++)
    {
        if(dg_objects[i].ot==0)
            return i;
    }
    return -1;
}

uint8_t dg_CntObj(uint8_t ot)
{
    uint8_t i;
    uint8_t cnt = 0;
    for(i=0;i<DG_OBJ_CNT;i++)
    {
        if(dg_object[i].ot==ot)
            cnt++;
    }
    return cnt;
}

uint8_t dg_px_x, dg_px_y;
uint8_t dg_CalcXY(dg_obj *o)
{
    dg_px_x = o->y>>DG_FP;
    dg_px_y = o->x>>DG_FP;
    return dg_px_x;
}

void dg_SetXY(dg_obj *o, uint8_t x, uint8_t y)
{
    o->x = ((int16_t)x) << DG_FP;
    o->y = ((int16_t)y) << DG_FP;
}

int16_t dg_bbox_x0, dg_bbox_y0, dg_bbox_x1, dg_bbox_y1;

void dg_CalcBBOX(uint8_t objnr)
{
    dg_obj *o = dg_GetObj(objnr);

    dg_bbox_x0 = (uint16_t)(o->x>>DG_FP);
    dg_bbox_x1 = dg_bbox_x0;
    dg_bbox_x0 += o->x0;
    dg_bbox_x1 += o->x1;

    dg_bbox_y0 = (uint16_t)(o->y>>DG_FP);
    dg_bbox_y1 = dg_bbox_y0;
    dg_bbox_y0 += o->y0;
    dg_bbox_y1 += o->y1;
}

uint8_t dg_cbbox_x0, dg_cbbox_y0, dg_cbbox_x1, dg_cbbox_y1;
uint8_t dg_ClipBBOX()
{
    if(dg_bbox_x0 >= DG_AREA_WIDTH)
        return 0;
    if(dg_bbox_x0 >= 0)
        dg_cbbox_x0 = (uint16_t)dg_bbox_x0;
    else
        dg_cbbox_x0 = 0;

    if(dg_bbox_x1 < 0)
        return 0;
    if(dg_bbox_x1 < DG_AREA_WIDTH)
        dg_cbbox_x1 = (uint16_t)dg_bbox_x1;
    else
        dg_cbbox_x1 = DG_AREA_WIDTH-1;

    if(dg_bbox_y0 >= DG_AREA_HEIGHT)
        return 0;
    if(dg_bbox_y0 >= 0)
        dg_cbbox_y0 = (uint16_t)dg_bbox_y0;
    else
        dg_cbbox_y0 = 0;

    if(dg_bbox_y1 < 0)
        return 0;
    if(dg_bbox_y1 < DG_AREA_HEIGHT)
        dg_cbbox_y1 = (uint16_t)dg_bbox_y1;
    else
        dg_cbbox_y1 = DG_AREA_HEIGHT-1;

    return 1;
}

//*universal member functions*//
uint8_t dg_IsOut(uint8_t objnr)
{
    dg_CalcBBOX(objnr);
    if(dg_bbox_x0 >= DG_AREA_WIDTH)
        return 1;
    if(dg_bbox_x1 < 0)
        return 1;
    if(dg_bbox_y0 >= DG_AREA_HEIGHT)
        return 1;
    if(dg_bbox_y1 <0)
        return 1;
    return 0;
}

//*type dependent member functions*//
void dg_Move(uint8_t objnr)
{
    dg_obj *o = dg_GetObj(objnr);
    switch(u8x8_pgm_read(&(dg_object_types[o->ot].move_fn)))
    {
        case DG_MOVE_X_PROP:
            o->x -= (1<<DG_FP)/8;
            o->x -= dg_difficulty;
            o->y += (int16_t)o->tmp;
            if(o->y >= ((DG_AREA_HEIGHT-1) << DG_FP) || o->y <= 0)
        o->tmp = - o->tmp;
            break;
        case DG_JUMP_PLAYER:
            o->y = dg_player_pos<<DG_FP;
            break;
        case DG_DUCK_PLAYER:
            o->y = dg_player_pos<<DG_FP;
            break;
        case DG_MOVE_WALL:
            o->x -=1;
            o->x -= (dg_difficulty>>1);
            break;
    }
}

void dg_DrawBBOX(uint8_t objnr)
{
    uint8_t y0, y1;
    dg_CalcBBOX(objnr);
    if(dg_ClipBBOX() == 0)
        return;
    
    u8g2_SetDrawColor(dg_u8g2, 1);
    y0 = u8g_height_minus_one - dg_cbbox_y0;
    y1 = u8g_height_minus_one - dg_cbbox_y1;

    u8g2_drawFrame(dg_u8g2, dg_cbbox_x0, y1, dg_cbbox_x1-dg_cbbox_x0+1, y0-y1+1);
}

#ifdef FN_IS_NOT_IN_USE
void dg_DrawFilledBox(uint8_t objnr)
{
    dg_CalcBBOX(objnr);
    if(dg_ClipBBOX() == 0)
        return;
    dog_SetBox(dg_cbbox_x0, dg_cbbox_y0, dg_cbbox_x1, dg_cbbox_y1);
}
#endif

void dg_DrawBitmap(uint8_t objnr, const uint8_t * bm, uint8_t w, uint8_t h)
{
    dg_CalcBBOX(objnr);

    u8g2_DrawBitmap(dg_u8g2, dg_bbox_x0, u8g_height_minus_one - dg_bbox_y1, (w+7)/8, h, bm);
}

void dg_DrawObj(uint8_t objnr)
{
    dg_obj *o = dg_GetObj(objnr);
    switch(u8x8_pgm_read(&(dg_object_types[o->ot].draw_fn)))
    {
        case DG_DRAW_NONE:
            break;
        case DG_DRAW_BBOX:
            dg_DrawBBOX(objnr);
            break;
        case DG_DRAW_CACTUS_HIGH:
            dg_DrawBitmap(objnr, dg_bitmap_cactus_high,o->x1-o->x0+1,8);
            break;
        case DG_DRAW_CACTUS_NORM
            dg_DrawBitmap(objnr, dg_bitmap_cactus_norm,o->x1-o->x0+1,4);
            break;
        case DG_DRAW_CACTUS_WIDE:
            dg_DrawBitmap(objnr, dg_bitmap_cactus_wide,o->x1-o->x0+1,5);
            break;
        case DG_DRAW_PLAYER:
            dg_DrawBitmap(objnr, dg_bitmap_player,o->x1-o->x0+1,8);
            break;
        case DG_DRAW_BIRD:
            dg_DrawBitmap(objnr, dg_bitmap_bird,o->x1-o->x0+1,8);
            break;
    }
}
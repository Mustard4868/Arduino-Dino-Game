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
    uint8_t destroy_fn;
    uint8_t move_fn;
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
#define DG_JUMP_NONE 0
#define DG_DUCK_NONE 0
#define DG_JUMP_PLAYER 1
#define DG_DUCK_PLAYER 1

#define DG_IS_HIT_NONE 0
#define DG_IS_HIT_BBOX 1
#define DG_IS_HIT_WALL 2

#define DG_DESTROY_NONE 0
#define DG_DESTROY_END 1

#define DG_OT_WALL_SOLID 1
#define DG_OT_CACTUS_NORM 2
#define DG_OT_PLAYER 3
#define DG_OT_CACTUS_WIDE 4
#define DG_OT_CACTUS_HIGH 5
#define DG_OT_BIRD 6

//*graphics object*//
u8g2_t *dg_u8g2;
u8g2_uint_t u8g_height_minus_one;
#define DG_AREA_HEIGHT (dg_u8g2->height - 8)
#define DG_AREA_WIDTH (dg_u8g2->width)

//*object types*//
const dg_ot dg_object_types[] U8X8_PROGMEM = 
{
    /*0: Empty object type*/
    {0,0,0, DG_DRAW_NONE, DG_DESTROY_END, DG_JUMP_NONE, DG_DUCK_NONE, DG_IS_HIT_NONE},
    /*1: Wall, player end*/
    {2,1,30, DG_DRAW_BBOX, DG_DESTROY_END, DG_MOVE_X_PROP, DG_IS_HIT_WALL},
    /*2: DG_OT_CACTUS_HIGH*/
    {2,1,0, DG_DRAW_CACTUS_HIGH, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*3: DG_OT_CACTUS_WIDE*/
    {2,1,0, DG_DRAW_CACTUS_WIDE, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*4: DG_OT_CACTUS_NORM*/
    {2,1,0, DG_DRAW_CACTUS_NORM, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*5: DG_OT_BIRD*/
    {2,1,0, DG_DRAW_BIRD, DG_DRAW_BBOX, DG_MOVE_X_PROP, DG_IS_HIT_BBOX},
    /*6: DG_OT_PLAYER*/
    {0,2,0, DG_DRAW_PLAYER, DG_JUMP_PLAYER, DG_DESTROY_END, DG_DUCK_PLAYER, DG_IS_HIT_BBOX},
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
};

const uint8_t dg_bitmap_cactus_norm[] =
{
  /* 100 */  0x004,
  /* 101 */ 0x006,
  /* 10 */ 0x002,
  /* 10 */ 0x002
};

const uint8_t dg_bitmap_cactus_wide[] =
{
  /* 100000 */ 0x020,
  /* 101010 */ 0x02A,
  /* 10010 */ 0x012,
  /* 1001100 */ 0x04C,
  /* 1001001 */  0x049
};

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
};

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
};

//*forward definitions*//
uint8_t dg_rnd() U8X8_NOINLINE;
static dg_obj *dg_GetObj(uint8_t objnr) U8X8_NOINLINE;
uint8_t dg_GetPlayerMask(uint8_t objnr);
uint8_t dg_GetHitMask(uint8_t objnr);
int8_t dg_FindObj(uint8_t ot) U8X8_NOINLINE;
void dg_ClrObjs() U8X8_NOINLINE;
int8_t dg_NewObj() U8X8_NOINLINE;
uint8_t dg_CntObj(uint8_t ot);
uint8_t dg_CalcXY(dg_obj *o) U8X8_NOINLINE;
void dg_SetXY(dg_obj *o, uint8_t x, uint8_t y) U8X8_NOINLINE;

void dg_JumpStep(uint8_t is_jump) U8X8_NOINLINE;
void dg_DuckStep(uint8_t is_duck) U8X8_NOINLINE;

void dg_InitProp(uint8_t x, uint8_t y, int8_t dir);
void dg_setupPlayer(uint8_t objnr, uint8_t ot);

//*utility functions*//
char dg_itoa_buf[12];
char *dg_itoa(unsigned long v)
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
        if(dg_objects[i].ot==ot)
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

void dg_Disappear(uint8_t objnr)
{
  dg_obj *o = dg_GetObj(objnr);
  dg_player_score += u8x8_pgm_read(&(dg_object_types[o->ot].score));
  o->ot = 0;
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

    u8g2_DrawFrame(dg_u8g2, dg_cbbox_x0, y1, dg_cbbox_x1-dg_cbbox_x0+1, y0-y1+1);
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
        case DG_DRAW_CACTUS_NORM:
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

uint8_t dg_IsHitBBOX(uint8_t objnr, uint8_t x, uint8_t y)
{
    dg_CalcBBOX(objnr);
    if(dg_ClipBBOX() == 0)
        return 0;
    if(x<dg_cbbox_x0)
        return 0;
    if(x>dg_cbbox_x1)
        return 0;
    if(y<dg_cbbox_y0)
        return 0;
    if(y>dg_bbox_y1)
        return 0;
    return 1;
}

void dg_Destroy(uint8_t objnr)
{
    int8_t nr;
    dg_obj *o = dg_GetObj(objnr);
    switch(u8x8_pgm_read(&(dg_object_types[o->ot].destroy_fn)))
    {
        case DG_DESTROY_NONE:
            break;
        case DG_DESTROY_END:
            dg_Disappear(objnr);
            dg_state = DG_STATE_END;
            o->tmp = 0;
            break;
    }
}

uint8_t dg_IsHit(uint8_t objnr, uint8_t x, uint8_t y, uint8_t player_mask)
{
    uint8_t hit_mask = dg_GetHitMask(objnr);
    dg_obj *o;

    if((hit_mask & player_mask) == 0)
        return 0;
    
    o = dg_GetObj(objnr);
    
    switch(u8x8_pgm_read(&(dg_object_types[o->ot].is_hit_fn)))
    {
        case DG_IS_HIT_NONE:
            break;
        case DG_IS_HIT_BBOX:
            if(dg_IsHitBBOX(objnr, x, y) != 0)
            {
        dg_Destroy(objnr);
        return 1;
            }
            break;
    }
}

uint8_t dg_jump_player = 0;
uint8_t dg_duck_player = 0;
uint8_t dg_jump_period = 2;
uint8_t dg_manual_jump_delay = dg_jump_period++;
uint8_t dg_is_jump_last_value = 0;

void dg_JumpStep(uint8_t is_jump)
{
    if(dg_jump_player < dg_manual_jump_delay)
    {
        dg_jump_player++;
    }
    else
    {
        if (dg_is_jump_last_value == 0)
    if (is_jump != 0)
        dg_jump_player = 0;
    }
}

void dg_DuckStep(uint8_t is_duck)
{
    dg_duck_player = 0;
}

//*object init functions*//
void dg_InitProp(uint8_t x, uint8_t y, int8_t dir)
{
    dg_obj *o;
    int8_t objnr = dg_NewObj();
    if(objnr < 0)
        return;
    o= dg_GetObj(objnr);
    if((dg_rnd()&3) == 0)
        o->ot = DG_OT_CACTUS_HIGH;
    else if((dg_rnd()&3) == 1)
        o->ot = DG_OT_CACTUS_NORM;
    else if((dg_rnd()&3) == 2)
        o->ot = DG_OT_CACTUS_WIDE;
    else   
        o->ot = DG_OT_BIRD;
    if(dir==0)
    {
        o->tmp = 0;
        if(dg_rnd()&1)
        {
            if(dg_rnd()&1)
        o->tmp++;
            else
        o->tmp--;
        }
    }
    else
    {
        o->tmp = dir;
    }
    dg_SetXY(o, x, y);
    o->x0 = -3;
    o->x1 = 1;
    o->y0 = -2;
    o->y1 = 2;
}

void dg_SetupPlayer(uint8_t objnr, uint8_t ot)
{
    dg_obj *o = dg_GetObj(objnr);
    switch(ot)
    {
        case DG_OT_PLAYER:
        o->ot = ot;
        o->y0 = -2;
        o->y1 = 2;
        break;
    }
}

void dg_NewPlayer()
{
    dg_obj *o;
    int8_t objnr = dg_NewObj();
    if(objnr<0)
        return;
    o= dg_GetObj(objnr);
    o->x = 6<<DG_FP;
    o->y = (DG_AREA_HEIGHT/2)<<DG_FP;
    o->x0 = -6;
    o->x1 = 0;
    dg_SetupPlayer(objnr, DG_OT_PLAYER);
}

//*prop creation*//
void dg_InitDeltaWall()
{
    uint8_t i;
    uint8_t cnt = 0;
    uint8_t max_x = 0;
    uint8_t max_l;

    uint8_t min_dist_for_new = 40;
    uint8_t my_difficulty = dg_difficulty;

    if(dg_difficulty >= 2)
    {
        max_l = DG_AREA_WIDTH;
        max_l -= min_dist_for_new;

        if(my_difficulty > 30)
            my_difficulty = 30;
        min_dist_for_new -= my_difficulty;

        for(i=0;i<DG_OBJ_CNT;i++)
        {
            if(dg_objects[i].ot == DG_OT_WALL_SOLID)
            {
        cnt++;
        if(max_x<(dg_objects[i].x>>DG_FP))
            max_x=(dg_objects[i].x>>DG_FP);
           }
        }
    }
}

void dg_InitDeltaProp()
{
    uint8_t i;
    uint8_t cnt = 0;
    uint8_t max_x = 0;
    uint8_t max_l;

    uint8_t upper_prop_limit = DG_OBJ_CNT-7;
    uint8_t min_dist_for_new = 20;
    uint8_t my_difficulty = dg_difficulty;

    if(my_difficulty > 14)
        my_difficulty = 14;
    min_dist_for_new -= my_difficulty;

    for(i=0;i<DG_OBJ_CNT;i++)
    {
        if(dg_objects[i].ot == DG_OT_CACTUS_HIGH || dg_objects[i].ot == DG_OT_CACTUS_NORM || dg_objects[i].ot == DG_OT_CACTUS_WIDE || dg_objects[i].ot == DG_OT_BIRD)
        {
            cnt++;
            if(max_x<(dg_objects[i].x>>DG_FP))
        max_x = (dg_objects[i].x>>DG_FP);
        }
    }

    max_l = DG_AREA_WIDTH;
    max_l -= min_dist_for_new;

    if(cnt<upper_prop_limit)
        if(max_x<max_l)
        {
            if((dg_difficulty >= 3) && ((dg_rnd()&7) ==0))
        dg_InitProp(DG_AREA_WIDTH-1, rand() & (DG_AREA_HEIGHT-1),0);
        }
}

void dg_InitDelta()
{
    dg_InitDeltaProp();
    dg_InitDeltaWall();
}

//*API: GAME DRAWING*//
void dg_DrawInGame(uint8_t fps)
{
    uint8_t i;
    for(i=0;i<DG_OBJ_CNT;i++)
        dg_DrawObj(i);

    u8g2_SetDrawColor(dg_u8g2, 0);
    u8g2_DrawBox(dg_u8g2, 0, u8g_height_minus_one - DG_AREA_HEIGHT-3, dg_u8g2->width, 4);

    u8g2_SetDrawColor(dg_u8g2, 1);
    u8g2_DrawHLine(dg_u8g2, 0, u8g_height_minus_one - DG_AREA_HEIGHT+1, DG_AREA_WIDTH);
    u8g2_DrawHLine(dg_u8g2, 0, u8g_height_minus_one, DG_AREA_WIDTH);
    u8g2_SetFont(dg_u8g2, u8g_font_4x6r);
    u8g2_DrawStr(dg_u8g2, 0, u8g_height_minus_one - DG_AREA_HEIGHT, dg_itoa(dg_difficulty));
    u8g2_DrawHLine(dg_u8g2, 10, u8g_height_minus_one - DG_AREA_HEIGHT-3, (dg_to_diff_cnt>>DG_DIFF_FP)+1);
    u8g2_DrawVLine(dg_u8g2, 10, u8g_height_minus_one - DG_AREA_HEIGHT-4, 3);
    u8g2_DrawVLine(dg_u8g2, 10+DG_DIFF_VIS_LEN, u8g_height_minus_one - DG_AREA_HEIGHT-4, 3);

    u8g2_DrawStr(dg_u8g2, DG_AREA_WIDTH-5*4-2, u8g_height_minus_one - DG_AREA_HEIGHT, dg_itoa(dg_player_score));

    if (fps > 0)
    {
        i=u8g2_DrawStr(dg_u8g2, DG_AREA_WIDTH-5*4-2-7*4, u8g_height_minus_one - DG_AREA_HEIGHT, "FPS:");
        u8g2_DrawStr(dg_u8g2, DG_AREA_WIDTH-5*4-2-7*4+i, u8g_height_minus_one - DG_AREA_HEIGHT, dg_itoa(fps));
    }
}

void dg_draw(uint8_t fps)
{
    switch(dg_state)
    {
        case DG_STATE_MENU:
        case DG_STATE_INITIALIZE:
            u8g2_SetFont(dg_u8g2, u8g_font_4x6r);
            u8g2_SetDrawColor(dg_u8g2, 1);
            u8g2_DrawStr(dg_u8g2, 0, u8g_height_minus_one - (dg_u8g2->height-6)/2, "PRESS SENSOR");
            u8g2_DrawHLine(dg_u8g2, dg_u8g2->width-dg_to_diff_cnt-10, u8g_height_minus_one - (dg_u8g2->height-6)/2+1, 11);
            break;
        case DG_STATE_GAME:
            dg_DrawInGame(fps);
            break;
        case DG_STATE_END:
        case DG_STATE_IEND:
            u8g2_SetFont(dg_u8g2, u8g_font_4x6r);
            u8g2_SetDrawColor(dg_u8g2, 1);
            u8g2_DrawStr(dg_u8g2, 0, u8g_height_minus_one - (dg_u8g2->height-6)/2, "Game Over");
            u8g2_DrawStr(dg_u8g2, 50, u8g_height_minus_one - (dg_u8g2->height-6)/2, dg_itoa(dg_player_score));
            u8g2_DrawStr(dg_u8g2, 75, u8g_height_minus_one - (dg_u8g2->height-6)/2, dg_itoa(dg_highscore));
            u8g2_DrawHLine(dg_u8g2, dg_to_diff_cnt, u8g_height_minus_one - (dg_u8g2->height-6)/2+1, 11);
            break;
    }
}

void dg_SetupInGame()
{
    dg_player_score = 0;
    dg_difficulty = 1;
    dg_to_diff_cnt = 0;
    dg_ClrObjs();
    dg_NewPlayer();
}

//*API: Game setup*//
void dg_Setup(u8g2_t *u8g)
{
    dg_u8g2 = u8g;
    u8g2_SetBitmapMode(u8g, 1);
    u8g_height_minus_one = u8g->height;
    u8g_height_minus_one--;
}

//*API: Game step exec*//
void dg_StepInGame(uint8_t player_pos, uint8_t is_jump, uint8_t is_duck)
{
    uint8_t i, j;
    uint8_t player_mask;

    if(player_pos <64)
        dg_player_pos =0;
    else if(player_pos >= 192)
        dg_player_pos = DG_AREA_HEIGHT-2-1;
    else   
        dg_player_pos = ((uint16_t)((player_pos-64)) * (uint16_t)(DG_AREA_HEIGHT-2))/128;
    dg_player_pos+=1;
    for(i=0;i<DG_OBJ_CNT;i++)
    dg_Move(i);

    for(i=0;i<DG_OBJ_CNT;i++)
        if(dg_objects[i].ot != 0)
            if(dg_IsOut(i) != 0)
    dg_Disappear(i);

    for(i=0;i<DG_OBJ_CNT;i++)
    {
        player_mask = dg_GetPlayerMask(i);
        if(player_mask != 0)
            if(dg_CalcXY(dg_objects+i) != 0)
        for(j=0; j<DG_OBJ_CNT;j++)
            if(i != j)
                if(dg_IsHit(j, dg_px_x, dg_px_y, player_mask) != 0)
                {
                    dg_Destroy(i);
                }
    }

    dg_JumpStep(is_jump);
    for(i=0;i<DG_OBJ_CNT;i++)
        dg_Move(i);

    dg_DuckStep(is_duck);
    for(i=0;i<DG_OBJ_CNT;i++)
        dg_Move(i);

    dg_InitDelta();

    dg_to_diff_cnt++;
    if(dg_to_diff_cnt == (DG_DIFF_VIS_LEN<<DG_DIFF_FP))
    {
        dg_to_diff_cnt = 0;
        dg_difficulty++;
    }
}

void dg_Step(uint8_t player_pos, uint8_t is_jump, uint8_t is_duck)
{
    switch(dg_state)
    {
        case DG_STATE_MENU:
            dg_to_diff_cnt = dg_u8g2->width-10;
            u8g2.firstPage();
            do{
                u8g2.setFont(u8g2_font_6x10_tr);
                u8g2.drawStr(20,20,"Press 'X' to play!");
            }while(u8g2.nextPage());
            while(digitalRead(button_x)!=LOW){}//wait for input
            dg_state = DG_STATE_INITIALIZE;
            break;
        case DG_STATE_INITIALIZE:
            dg_to_diff_cnt--;
            if(dg_to_diff_cnt == 0)
            {
        dg_state = DG_STATE_GAME;
        dg_SetupInGame();
            }
            break;
            case DG_STATE_GAME:
                dg_StepInGame(player_pos, is_jump, is_duck);
                break;
            case DG_STATE_END:
                dg_to_diff_cnt = dg_u8g2->width-10;
                if(dg_highscore < dg_player_score)
            dg_highscore = dg_player_score;
                dg_state = DG_STATE_IEND;
                break;
            case DG_STATE_IEND:
                dg_to_diff_cnt--;
                if(dg_to_diff_cnt == 0)
            dg_state = DG_STATE_MENU;
                break;
    }
}

void setup()    {
    u8g2.begin();
}

uint8_t a;
uint8_t b;
uint8_t y = 128;

void loop() {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setFontDirection(0);

    dg_Setup(u8g2.getU8g2());
    for(;;)
    {
        u8g2.firstPage();
        do
        {
            dg_draw(0);
        }while(u8g2.nextPage());

        if(digitalRead(button_x)){
            y++;
        }

        if(digitalRead(button_y)){
            y--;
        }
    }
}
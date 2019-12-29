
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"
#include "player.h"

#define GREEN_LED BIT6
#define SW1 BIT0
#define SW4 BIT3

int playerHorOffset = 0;
int playerVerOffset = 0;

//Initialize the shape
player player1={playerGetBounds, drawPlayer}; //player drawing
AbRect playerBody = {abRectGetBounds, abRectCheck, {6 ,8}}; // player hitbox 
AbRect rain = {abRectGetBounds, abRectCheck, {10,10}}; // rain
AbRectOutline fieldOutline = { abRectOutlineGetBounds, abRectOutlineCheck, {screenWidth/2, screenHeight/2 + 20} }; //playable area

//Initialize layers
Layer fieldLayer = {
  (AbShape *) &fieldOutline, {screenWidth/2, screenHeight/2-20},
  {0,0}, {0,0},
  COLOR_WHITE, 0
};

Layer square = {
  (AbShape *) &playerBody, {screenWidth/2, screenHeight-10},
  {0,0}, {0,0},
  COLOR_WHITE, &fieldLayer
};

Layer playerLayer = {
  (AbShape *)&player1, {screenWidth/2, screenHeight-10},
  {0,0}, {0,0},
  COLOR_BLACK, &square
};

Layer rainLeftSide = {
  (AbShape *)&circle4, {screenWidth/2 - 30, -13},
  {0,0}, {0,0},
  COLOR_BLUE, &playerLayer
};
Layer rainRightSide = {
  (AbShape *)&circle4, {screenWidth/2 + 30, -13},
  {0,0}, {0,0},
  COLOR_BLUE, &rainLeftSide
};
Layer rainCenter = {
  (AbShape *)&circle4, {screenWidth/2, -13},
  {0,0}, {0,0},
  COLOR_BLUE, &rainRightSide
};

//Moving Layer
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/** player Moving Layer Linked List
 */
MovLayer ml0 = { &square, {0,0}, 0 };
MovLayer ml1 = {&playerLayer, {0,0}, &ml0};

/** rain Moving Layer Linked List
 */
MovLayer rainMl2 = { &rainLeftSide, {0,5}, 0};
MovLayer rainMl1 = { &rainRightSide, {0,4}, &rainMl2};
MovLayer rainMl0 = { &rainCenter, {0,7}, &rainMl1};
 
short transitionSpeed = 30;      
char isGameOver = 0;             
u_int bgColor =COLOR_WHITE; 
int redrawScreen = 1;            
char showInstruction = 1;        
Region fieldFence;		 

//move layers 
void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) {
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  }
	} 
	lcd_writeColor(color); 
      } 
    } 
  } 
}	  

//rain route
void rainAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    if(shapeBoundary.topLeft.axes[1] > screenHeight-20){
      newPos.axes[1] = -10;
      ml->velocity.axes[1] += 1;
    }
    ml->layer->posNext = newPos;
  } /**< for ml */
}


// player movement
void moveplayer(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    //moves the player
    if(newPos.axes[0] + playerHorOffset > 20 &&
       newPos.axes[0] + playerHorOffset < screenWidth -20){
      newPos.axes[0] = newPos.axes[0] + playerHorOffset;
    }
    ml->layer->posNext = newPos;
  }
}

//Collision Check
char checkForCollision(MovLayer *rain, MovLayer *player){
  
  Region playerBounday;
  Region rainBoundary;
  Vec2 coordinates;
  
  abShapeGetBounds(player->layer->abShape, &player->layer->posNext, &playerBounday);
  for (; rain; rain = rain->next) {
    vec2Add(&coordinates, &rain->layer->pos, &rain->velocity);
    abShapeGetBounds(rain->layer->abShape, &coordinates, &rainBoundary);

    //if statement for collision
    if( abShapeCheck(player->layer->abShape, &player->layer->pos, &rainBoundary.topLeft) ||
        abShapeCheck(player->layer->abShape, &player->layer->pos, &rainBoundary.botRight) ){
      isGameOver = 1;
    }
  }  
  return 0;
}

//player controls
void readSwitches(){
  char switches =  p2sw_read();
  char s1 = (switches & SW1) ? 0 : 1;
  char s4 = (switches & SW4) ? 0 : 1;

  if(s1){  //move left
    playerHorOffset = -30;
  }
  if(s4){  //move right
    playerHorOffset = 30;
  }
  //redraw after input
  if(s1 || s4){
    moveplayer(&ml1, &fieldFence);
    redrawScreen = 1;
    buzzer_play_player_move();
  }
}

//main body, code initialization
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	       

  configureClocks();
  lcd_init();
  buzzer_init();
  p2sw_init(15);
  
  layerInit(&rainCenter);
  layerDraw(&rainCenter);
  layerGetBounds(&fieldLayer, &fieldFence);

  drawString5x7(screenWidth/2 -40, screenHeight/2 -30, "Let it rain!!!", COLOR_BLACK, COLOR_WHITE );

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);/**< CPU OFF */
      readSwitches();
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    playerHorOffset = 0;
    playerVerOffset = 0;
    movLayerDraw(&ml1, &playerLayer);
    movLayerDraw(&rainMl0, &rainCenter);

  }
}

void state_advance(){
  static enum {preGame = 0, play = 1, gameOver = 2} currentState =preGame;
  static char switches;
  static char s1;
      
  switch(currentState){
  case preGame:  
    switches =  p2sw_read();
    s1 = (switches & SW1) ? 0 : 1;
    if(s1){        
      buzzer_set_period(0);
      currentState = play;
      transitionSpeed = 30;
      layerDraw(&rainCenter);
    }
    break;
  case play:
    buzzer_set_period(0); 
    if(!isGameOver){ 
      rainAdvance(&rainMl0, &fieldFence);
      checkForCollision(&rainMl0, &ml0);
      redrawScreen = 1;
    }
    else{ //game over banner
      layerDraw(&rainCenter);
      drawString5x7(screenWidth/2 -25, screenHeight/2, "Game Over", COLOR_BLACK, COLOR_WHITE);
      currentState = gameOver;
      transitionSpeed = 30;
    }
    break;
  case gameOver:
    switches =  p2sw_read();
    s1 = (switches & SW1) ? 0 : 1;
    if(!s1){
    
    }
    else{ //reset the game
  
      playerLayer.posNext.axes[1] = screenHeight-20;
      playerLayer.posNext.axes[0] = screenWidth/2;
      rainLeftSide.posNext.axes[1] = -10;
      rainRightSide.posNext.axes[1] = -10;
      rainCenter.posNext.axes[1] = -10;
      rainMl0.velocity.axes[1] = 4;
      rainMl1.velocity.axes[1] = 2;
      rainMl2.velocity.axes[1] = 3;
      layerDraw(&rainCenter);
      currentState = play;
      redrawScreen = 1;
      isGameOver = 0;
      transitionSpeed = 30;
    }
    break;
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count++;
  
  if (count == transitionSpeed) {
    state_advance();
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6


AbRect rect10 = {abRectGetBounds, abRectCheck, {10,10}}; /**< 10x10 rectangle */
//AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 30};

AbRect rect5 =  {abRectGetBounds, abRectCheck, {8,5}};
AbRect rect2 =  {abRectGetBounds, abRectCheck, {2,8}};
AbRect bullet = {abRectGetBounds, abRectCheck, {1,2}};

int score = 0;

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 10, screenHeight/2 - 10}
};

Layer layer7 = {
  (AbShape *)&bullet,
  {(screenWidth/2), (screenHeight/2)+62}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0
  };


Layer layer6 = {
  (AbShape *)&bullet,
  {(screenWidth/2), (screenHeight/2)+62}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &layer7
  };


Layer layer5 = {
  (AbShape *)&rect2,
  {(screenWidth/2), (screenHeight/2)+62}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_SEA_GREEN,
  &layer6
  };


Layer layer4 = {
  (AbShape *)&rect5,
  {(screenWidth/2), (screenHeight/2)+65}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_SEA_GREEN,
  &layer5
  };
  

Layer layer3 = {		/**< Layer with an orange circle */
  (AbShape *)&circle8,
  {70, 33}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_VIOLET,
  &layer4,
};


Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &layer3
};

Layer layer1 = {		/**< Layer with a red square */
  (AbShape *)&circle20,
  {30, 30}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GOLD,
  &fieldLayer,
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle14,
  {(screenWidth/2)+10, 35}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_SKY_BLUE,
  &layer1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */

MovLayer ml7 = { &layer7, {1,2}, 0};
MovLayer ml6 = { &layer6, {1,2}, &ml7 };
MovLayer ml5 = { &layer5, {1,2}, &ml6 };
MovLayer ml4 = { &layer4, {2,2}, &ml5 };
MovLayer ml3 = { &layer3, {1,1}, &ml4 }; /**< not all layers move */
MovLayer ml1 = { &layer1, {1,2}, &ml3 }; 
MovLayer ml0 = { &layer0, {2,1}, &ml1 }; 



int isGameOver = 0;

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  if(!isGameOver){
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
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
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
  }
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for(int i = 0; i < 3; i++){
  //for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    if((shapeBoundary.botRight.axes[1] == fence->botRight.axes[1])){
      isGameOver = 1;
      char gameOver[10];
      gameOver[0] = 'G';
      gameOver[1] = 'A';
      gameOver[2] = 'M';
      gameOver[3] = 'E';
      gameOver[4] = ' ';
      gameOver[5] = 'O';
      gameOver[6] = 'V';
      gameOver[7] = 'E';
      gameOver[8] = 'R';
      gameOver[9] = 0;
      drawString5x7(40,70, gameOver, COLOR_RED, COLOR_BLACK);
    }
    ml->layer->posNext = newPos;
    ml = ml ->next;
  } /**< for ml */
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */
u_int shotFired = 0;
u_int shot2Fired = 0;

Region fieldFence;		/**< fence around playing field  */

void moveShipRight(Layer *layer, Layer *layer2, Layer *bullet1, Layer *bullet2){
  Vec2 nextPosition = layer->pos;
  Vec2 nextPos = layer2->pos;
  Vec2 bulletNextPos = bullet1->posNext;
  Vec2 bullet2NextPos = bullet2->posNext;
  if(layer->pos.axes[0] < 105){
    nextPosition.axes[0] = layer->pos.axes[0] + 5;
    nextPos.axes[0] = layer2->pos.axes[0] + 5;
    if(!shotFired)
      bulletNextPos.axes[0] = bullet1->pos.axes[0] + 5;
    if(!shot2Fired)
      bullet2NextPos.axes[0] = bullet2->pos.axes[0] + 5;
  }
  layer->posNext = nextPosition;
  layer2->posNext = nextPos;
  bullet1->posNext = bulletNextPos;
  bullet2->posNext = bullet2NextPos;
  
}

void moveShipLeft(Layer *layer, Layer *layer2, Layer *bullet1, Layer *bullet2){

  Vec2 nextPosition = layer->pos;
  Vec2 nextPos = layer2->pos;
  Vec2 bulletNextPos = bullet1->posNext;
  Vec2 bullet2NextPos = bullet2->posNext;
  if(layer->pos.axes[0] > 20){
    nextPosition.axes[0] = layer->pos.axes[0] - 5;
    nextPos.axes[0] = layer2->pos.axes[0] - 5;
    if(!shotFired)
      bulletNextPos.axes[0] = bullet1->pos.axes[0] - 5;
    if(!shot2Fired)
      bullet2NextPos.axes[0] = bullet2->pos.axes[0] - 5;
  }
  layer->posNext = nextPosition;
  layer2->posNext = nextPos;
  bullet1->posNext = bulletNextPos;
  bullet2->posNext = bullet2NextPos;
}

void shoot(Layer *layer, u_int isFirstShot, Layer *shipLayer, MovLayer *list){
  Vec2 nextPos = layer->pos;
  if(layer->pos.axes[1] > 15){
    nextPos.axes[1] = layer->pos.axes[1] - 5;
  }else{
    if(isFirstShot)
      shotFired = 0;
    else
      shot2Fired = 0;
    nextPos = shipLayer->posNext;
  }
  Region shapeBoundary;
  for(int i = 0; i < 3; i++){
  //for (; ml; ml = ml->next) {
      abShapeGetBounds(list->layer->abShape, &list->layer->pos, &shapeBoundary);
   
      if ((shapeBoundary.topLeft.axes[0] <= layer->pos.axes[0]) &&
	  (shapeBoundary.botRight.axes[0] >= layer->pos.axes[0]) &&
	  (shapeBoundary.topLeft.axes[1] <= layer->pos.axes[1]) &&
	  (shapeBoundary.botRight.axes[1] >= layer->pos.axes[1])){
	
	buzzer_set_period(880);
        buzzer_set_period(0);
	score++;
	if(isFirstShot)
	  shotFired = 0;
	else
	  shot2Fired = 0;
	nextPos = shipLayer->posNext;
       
	list->layer->posNext.axes[0] = 30;
	list->layer->posNext.axes[1] = 30;
	 
      } /**< for axis */
      list = list ->next;
  }
  layer->posNext = nextPos;
}

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();
  buzzer_init();
  
  layerInit(&layer0);
  layerDraw(&layer0);


  layerGetBounds(&fieldLayer, &fieldFence);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  int count = 0;
  
  for(;;) {
    if(score == 9){
      isGameOver = 1;
      drawString5x7(40,70, "YOU WIN! \0", COLOR_GREEN, COLOR_BLACK);
    }
    char points[9];
    points[0] = 'S';
    points[1] = 'c';
    points[2] = 'o';
    points[3] = 'r';
    points[4] = 'e';
    points[5] = ':';
    points[6] = ' ';
    points[7] ='0' + score;
    points[8] = 0;
    drawString5x7(10,0, points, COLOR_GREEN, COLOR_BLACK);
    
    u_int switches = p2sw_read();
    
    //if(!switches & (1<< 1)){
    if(switches == 14){
      moveShipLeft(&layer4, &layer5, &layer6, &layer7);
    }
    // if(!switches & (1<< 0))
    if(switches == 7){
      moveShipRight(&layer4, &layer5, &layer6, &layer7);
    }

    if(switches == 13 || switches == 11){
      if(count==0){
	shotFired = 1;
	count++;
      }else{
	shot2Fired = 1;
	count = 0;
      }
	//shoot(&layer6);
    }
    
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 30) {
    mlAdvance(&ml0, &fieldFence);
    if(shotFired)
      shoot(&layer6, 1, &layer4, &ml0);
    if(shot2Fired)
      shoot(&layer7, 0, &layer4, &ml0);
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

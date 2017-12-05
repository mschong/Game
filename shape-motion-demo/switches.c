#include <msp430.h>
#include "switches.h"
#include "buzzer.h"
#include "led.h"
#include "wdInterruptHandler.h"

char switch3_state_down,switch1_state_down,switch2_state_down, switch0_state_changed,switch1_state_changed,switch2_state_changed; /* effectively boolean */

static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);	/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);	/* if switch down, sense up */
  return p2val;
}

void 
switch_init()			/* setup switch */
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE = SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
  switch_interrupt_handler();	/* to initially read the switches */
}

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switch3_state_down = (p2val & SW3) ? 0 : 1; /* 0 when SW0 is up */
  //  switch0_state_changed = 0;

  switch1_state_down = (p2val & SW1) ? 0 : 1; /* 0 when SW1 is up */
  // switch1_state_changed = 0;

  switch2_state_down = (p2val & SW2) ? 0 : 1; /* 0 when SW2 is up */
  //switch2_state_changed = 0;
  
  buzzer_init();  
  led_init();
}

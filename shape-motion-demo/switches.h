#ifndef switches_included
#define switches_included

#define SW0 BIT0 	/* switch1 is p1.3 */
#define SW1 BIT1
#define SW2 BIT2
#define SW3 BIT3
#define SWITCHES SW0 | SW1 | SW2 | SW3		/* only 1 switch on this board */

void switch_init();
void switch_interrupt_handler();

extern char switch3_state_down,switch1_state_down,switch2_state_down, switch0_state_changed,switch1_state_changed,switch2_state_changed; /* effectively boolean */

#endif // included

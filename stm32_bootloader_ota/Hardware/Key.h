#ifndef __KEY_H
#define __KEY_H


//extern uint8_t Key_adjust ;
//extern uint8_t Key_change ;

void Key_Init(void);
uint8_t Key_Press(void);
uint8_t Key_Adjust_Light(uint8_t light_old);
uint8_t Key_Adjust_Color();
uint8_t Key_Adjust_Clock(void);
void Key_Control(uint8_t mode_old, uint8_t light_old, uint8_t color_old,
				uint8_t* mode_new, uint8_t* light_new, uint8_t* adjust_clock, uint8_t* color_new);

#endif

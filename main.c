/* STM32C011F6 clock divider change & TIM1 software PWM
 * G. Witkowski 2023
 * gwitkowski2000@gmail.com
 
 * GPIO A4 (PWM) & A5 (while loop square wave for debug) as signal outputs 
 */

#include "stm32c0xx.h"

/* SysTick variable */
volatile uint64_t tick = 0;

void delay_ms(uint64_t ms){
	tick = 0;
	while(tick < ms){
		__NOP();
	}
}

void TIM1_config(void){
	RCC->APBENR2 |= RCC_APBENR2_TIM1EN;
	
	/* System Clock = 3MHz, so to make updates every 2us PSC + 1 has to be 5 */
	TIM1->PSC = 5;
	/* Signal frequency - 50Hz (period 20ms) - 10000 x 2us */
	TIM1->ARR = 10000;
	/* Signal duty cycle 25% */
	TIM1->CCR1 = 0.25 * 10000;	// 75% DC test
	
	/* Enable overflow (reaching ARR) and compare (reaching CCR1) interrupts */
	TIM1->DIER |= TIM_DIER_UIE | TIM_DIER_CC1IE;
	/* Counter enable */
	TIM1->CR1 |= TIM_CR1_CEN;
	
	/* Enable interrupts in NVIC */
	NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
	NVIC_EnableIRQ(TIM1_CC_IRQn);
	
}

int main() {
	/* Default frequency of System Clock is 12MHz (HSI 48MHz / 4)*/
	/* Clear HSI divider and set it to 16 instead of 4, so the System Clock frequency is 3MHz. Wait until HSI is ready */
	RCC->CR &= ~RCC_CR_HSIDIV_Msk;
	RCC->CR |= RCC_CR_HSIDIV_2;	// HSI / 16 -> 3MHZ
	while((RCC->CR & RCC_CR_HSIRDY) != RCC_CR_HSIRDY){
		__NOP();
	}
	
	/* Update SystemCoreClock variable and configure SysTick frequency*/
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);
	
	/* Configure "debug" GPIOs as outputs */
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
	GPIOA->MODER &= ~GPIO_MODER_MODE5_1;
	GPIOA->MODER &= ~GPIO_MODER_MODE4_1;
	
	TIM1_config();
	
    while (1) {
			delay_ms(10);
			/* Toggle one of the GPIOs to see if something crashed or not */
			GPIOA->ODR ^= GPIO_ODR_OD5;
		}
}

void TIM1_CC_IRQHandler(void){
	/* If the counter reached the value stored in TIM1-> CC1F (PWM high level time in this case) */
	if((TIM1->SR & TIM_SR_CC1IF) == TIM_SR_CC1IF){
		/* Set PWM / oscilloscope GPIO to low level */
		GPIOA->ODR &= ~GPIO_ODR_OD4;
		
		/* Clear capture compare flag */
		TIM1->SR &= ~TIM_SR_CC1IF;
	}
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void){
	/* If the counter reached the value stored in TIM1->ARR */
	if((TIM1->SR & TIM_SR_UIF) == TIM_SR_UIF){
		/* Set PWM / oscilloscope GPIO to high level */
		GPIOA->ODR |= GPIO_ODR_OD4;
	
		/* Clear overflow flag */
		TIM1->SR &= ~TIM_SR_UIF;
	}
}

void SysTick_Handler(void){
	tick++;
}

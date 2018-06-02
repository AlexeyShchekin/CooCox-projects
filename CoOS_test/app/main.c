#include <CoOS.h>
#include <stm32f10x.h>
#include "AppConfig.h"
#include "OsConfig.h"

#define TASK_STK_SIZE		128	 		/*!< Define stack size.				  */

#define	TIME_SET_PRI 		2		   	/*!< Priority of 'time_set' task.	  */
#define UART_PRINT_PRI 		4		   	/*!< Priority of 'tart_print' task.	  */
#define LED_DISPLAY_PRI		5			/*!< Priority of 'led_display' task.  */
#define TIME_DISRPLAY_PRI 	5			/*!< Priority of 'time_display' task. */


/*---------------------------------- Variable Define -------------------------*/
OS_STK   task_init_Stk[TASK_STK_SIZE];	 	/*!< Stack of 'task_init' task.	  */
OS_STK   uart_print_Stk[TASK_STK_SIZE];		/*!< Stack of 'uart_print' task.  */
OS_STK   led_display_Stk[TASK_STK_SIZE];	/*!< Stack of 'led_display' task. */

OS_MutexID	mut_uart;	  /*!< Save id of mutex.			 		  */
OS_FlagID	button_sel_flg;		  /*!< Flag id,related to 'EVT_BUTTON_SEL'.	  */
OS_FlagID	button_add_flg;		  /*!< Flag id,related to 'EEVT_BUTTON_ADD'.  */

U16 EVT_BUTTON_SEL = 0x0001;	 		/*!< Flag related to Select button.	  */
U16 EVT_BUTTON_ADD = 0x0002;			/*!< Flag related to add button.	  */

OS_EventID		mbox0;			   		/*!< Save id of mailbox.			  */
unsigned char 	errinfo[30];	   		/*!< Error information.				  */

unsigned char timeflag = 0;

unsigned char time[3] = {55,58,23};		/*!< Format: { second,minute,hour }	  */


//char chart[9] = {'0','0',':','0','0',':','0','0','\0'};		/*!< Time buffer  */

const char led[19] = {0xFF,0x7E,0x3C,0x18,
                      0x18,0x24,0x42,0x81,0x42,0x24,
					  0x18,0x24,0x42,0x81,
					  0x99,0xA5,0xC3,0xDB,0xE7};

extern unsigned short int  ADC_ConvertedValue;/*!< The value which AD converted*/


/*---------------------------------- Function declare ------------------------*/
void uart_print		(void *pdata);	  /*!< Print value of adc convert by UART.*/
void led_blink		(void *pdata);	  /*!< LED blinky.						  */
void task_init (void *pdata); /*!< Initialization task.				  */

void uart_print(void *pdata)
{
	int AD_value;
	pdata = pdata;
	for (;;)
	{
		CoEnterMutexSection(mut_uart);			/*!< Enter mutex area.		 */

		AD_value = ADC_ConvertedValue;
		uart_printf ("\r Time = ");
		uart_print_hex(RTC_GetCounter(),8);
		uart_printf (", AD value = 0x");
		uart_print_hex (AD_value,4);

		CoLeaveMutexSection(mut_uart); 			/*!< Leave mutex area.		  */

		CoTickDelay(20);
  	}
}

void led_blink(void *pdata)
{
	char i;
	pdata = pdata;
	for (;;)
	{
		for (i=0;i<19;i++)
		{
			GPIOB->ODR = (GPIOB->ODR & 0xFFFF00FF) | (led[i] << 8);
			CoTickDelay (20);
		}
  	}
}

void task_init(void *pdata)
{
	uart_printf (" [OK]. \n\r\n\r");
	uart_printf ("\r \"task_init\" task enter.		\n\r\n\r ");
	pdata = pdata;

	uart_printf ("\r Create the \"mut_uart\" mutex...      ");
	mut_uart = CoCreateMutex();
	if(mut_uart != E_CREATE_FAIL)
		uart_printf (" [OK]. \n");
	else
		uart_printf (" [Fail]. \n");


	uart_printf ("\r Create the \"button_sel_flg\" flag... ");

	/*!< Manual reset flag,initial state:0 */
	button_sel_flg	= CoCreateFlag(Co_FALSE,0);
	if(button_sel_flg != E_CREATE_FAIL)
		uart_printf (" [OK]. \n");
	else
		uart_printf (" [Fail]. \n");


	uart_printf ("\r Create the \"button_add_flag\" flag...");

	/*!< Manual reset flag,initial state:0	*/
	button_add_flg = CoCreateFlag(Co_FALSE,0);
	if(button_add_flg != E_CREATE_FAIL)
		uart_printf (" [OK]. \n\n");
	else
		uart_printf (" [Fail]. \n\n");


	uart_printf ("\r Create the first mailbox...         ");
	mbox0 = CoCreateMbox(EVENT_SORT_TYPE_FIFO);
 	if(mbox0 == E_CREATE_FAIL)
		uart_printf (" [Fail]. \n\n");
	else
	    uart_printf (" [OK]. \n\n");




	/* Configure Peripheral */
	uart_printf ("\r Initial hardware in Board :     \n\r");

	uart_printf ("\r ADC initial...                      ");
	ADC_Configuration  ();
	uart_printf (" [OK]. \n");

	uart_printf ("\r RTC initial...                      ");
	RTC_Configuration  ();
	uart_printf (" [OK]. \n");

	uart_printf ("\r GPIO initial...                     ");
	GPIO_Configuration ();
	uart_printf (" [OK]. \n");

	uart_printf ("\r External interrupt initial...       ");
	EXIT_Configuration ();
	uart_printf (" [OK]. \n");


	/* Create Tasks */

    CoCreateTask(    	              uart_print ,
	                                   (void *)0 ,
				                  UART_PRINT_PRI ,
				 &uart_print_Stk[TASK_STK_SIZE-1],
					                TASK_STK_SIZE
		        );
	CoCreateTask(                       led_blink ,
	                                    (void *)0 ,
	                                    LED_DISPLAY_PRI ,
	             &led_display_Stk[TASK_STK_SIZE-1],
	                                 TASK_STK_SIZE
	             );

	CoExitTask();	 /*!< Delete 'task_init' task. 	*/
}

void RTC_IRQHandler (void)
{
    CoEnterISR();                 /* Tell CooCox that we are starting an ISR. */

	time[0]++;
	if (time[0] == 60)
	{
		time[0] = 0;
		time[1]++;
		if (time[1] == 60)
		{
			time[1] = 0;
			time[2]++;
			if (time[2] == 24)
				time[2] = 0;
		}
	}

	isr_PostMail (mbox0,time);

	RTC->CRL &= ~(unsigned short)1<<0;
	CoExitISR();
}

void EXTI0_IRQHandler(void)
{
    CoEnterISR();

	isr_SetFlag(button_sel_flg);
	EXTI_ClearITPendingBit(EXTI_Line0);

	CoExitISR();
}

void EXTI15_10_IRQHandler(void)
{
    CoEnterISR();

	isr_SetFlag(button_add_flg);
	EXTI_ClearITPendingBit(EXTI_Line13);

	CoExitISR();
}

int main(void)
{
	RCC_Configuration();				/*!< Configure the system clocks      */
	  	NVIC_Configuration();			    /*!< NVIC Configuration               */
		UART_Configuration ();				/*!< UART configuration               */

		uart_printf ("\n\r\n\r");
		uart_printf ("\r System initial...                  ");
		uart_printf (" [OK]. \n");
		uart_printf ("\r System Clock have been configured as 60MHz!\n\n");

	    uart_printf ("\r +------------------------------------------------+ \n");
		uart_printf ("\r | CooCox RTOS Demo for Cortex-M3 MCU(STM32F10x). |	\n");
		uart_printf ("\r | Demo in Keil MCBSTM32 Board.                   | \n");
		uart_printf ("\r +------------------------------------------------+ \n\n");



		uart_printf ("\r Initial CooCox RTOS...              ");
		CoInitOS();							/*!< Initial CooCox RTOS 			  */
		uart_printf (" [OK]. \n");


		uart_printf ("\r Create a \"task_init\" task...        ");
		CoCreateTask(task_init, (void *)0, 10,&task_init_Stk[TASK_STK_SIZE-1], TASK_STK_SIZE);
		uart_printf (" [OK]. \n");


		uart_printf ("\r Start multi-task.                   ");
		CoStartOS();


	while (1);
}

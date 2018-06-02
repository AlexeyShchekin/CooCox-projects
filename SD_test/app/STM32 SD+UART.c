#include "stm32f10x.h" 

GPIO_InitTypeDef GPIO_InitStructure;					//	структура для конфигурирования портов ввода-вывода
USART_InitTypeDef USART_InitStructure;				//	структура для конфигурирования UART

#define GO_IDLE_STATE 0 											//Программная перезагрузка
#define SEND_IF_COND 8 												//Для SDC V2 - проверка диапазона напряжений
#define READ_SINGLE_BLOCK 17 									//Чтение указанного блока данных
#define WRITE_SINGLE_BLOCK 24 								//Запись указанного блока данных
#define SD_SEND_OP_COND 41 										//Начало процесса инициализации
#define APP_CMD 55 														//Главная команда из ACMD команд
#define READ_OCR 58 													//Чтение регистра OCR

//глобальная переменная для определения типа карты 
uint8_t  SDHC;            
//макроопределения для управления выводом SS
#define CS_ENABLE         GPIOA->BSRR = GPIO_BSRR_BR4;         
#define CS_DISABLE    	  GPIOA->BSRR = GPIO_BSRR_BS4;  
//*********************************************************************************************
//function  обмен данными по SPI1                                                            //
//argument  передаваемый байт                                                                //
//return    принятый байт                                                                    //
//*********************************************************************************************
uint8_t spi_send (uint8_t data)
{ 
  while (!(SPI1->SR & SPI_SR_TXE));      //убедиться, что предыдущая передача завершена
  SPI1->DR = data;                       //загружаем данные для передачи
  while (!(SPI1->SR & SPI_SR_RXNE));     //ждем окончания обмена
  return (SPI1->DR);		         //читаем принятые данные
}

//*********************************************************************************************
//function  прием байт по SPI1                                                               //
//argument  none                                                                             //
//return    принятый байт                                                                    //
//*********************************************************************************************
uint8_t spi_read (void)
{ 
  return spi_send(0xff);		  //читаем принятые данные
}

//*********************************************************************************************
//function  инициализация  SPI1                                                              //
//argument  none                                                                             //
//return    none                                                                             //
//*********************************************************************************************
void spi_init(void)
{
  RCC->APB2ENR |=  RCC_APB2ENR_AFIOEN;//включить тактирование альтернативных функций
  RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN;//включить тактирование порта А
 
  //вывод управления SS: выход двухтактный, общего назначения,50MHz
  GPIOA->CRL   |=  GPIO_CRL_MODE4;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF4;     //
  GPIOA->BSRR   =  GPIO_BSRR_BS4;     //
 
  //вывод SCK: выход двухтактный, альтернативная функция, 50MHz
  GPIOA->CRL   |=  GPIO_CRL_MODE5;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF5;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF5_1;   //
 
  //вывод MISO: вход цифровой с подтягивающим резистором, подтяжка к плюсу
  GPIOA->CRL   &= ~GPIO_CRL_MODE6;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF6;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF6_1;   //
  GPIOA->BSRR   =  GPIO_BSRR_BS6;     //
 
  //вывод MOSI: выход двухтактный, альтернативная функция, 50MHz
  GPIOA->CRL   |=  GPIO_CRL_MODE7;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF7;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF7_1;   //
 
  //настроить модуль SPI
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; //подать тактирование
  SPI1->CR2     = 0x0000;             //
  SPI1->CR1     = SPI_CR1_MSTR;       //контроллер должен быть мастером,конечно    
  SPI1->CR1    |= SPI_CR1_BR;         //для начала зададим маленькую скорость
  SPI1->CR1    |= SPI_CR1_SSI;
  SPI1->CR1    |= SPI_CR1_SSM;
  SPI1->CR1    |= SPI_CR1_SPE;        //разрешить работу модуля SPI
}

//********************************************************************************************
//function	 посылка команды в SD                                		            //
//Arguments	 команда и ее аргумент                                                      //
//return	 0xff - нет ответа   			                                    //
//********************************************************************************************
uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response, wait=0, tmp;     
 
  //для карт памяти SD выполнить коррекцию адреса, т.к. для них адресация побайтная 
  if(SDHC == 0)		
  if(cmd == READ_SINGLE_BLOCK || cmd == WRITE_SINGLE_BLOCK )  {arg = arg << 9;}
  //для SDHC коррекцию адреса блока выполнять не нужно(постраничная адресация)	
 
  CS_ENABLE;
 
  //передать код команды и ее аргумент
  spi_send(cmd | 0x40);
  spi_send(arg>>24);
  spi_send(arg>>16);
  spi_send(arg>>8);
  spi_send(arg);
 
  //передать CRC (учитываем только для двух команд)
  if(cmd == SEND_IF_COND) spi_send(0x87);            
  else                    spi_send(0x95); 
 
  //ожидаем ответ
  while((response = spi_read()) == 0xff) 
   if(wait++ > 0xfe) break;                //таймаут, не получили ответ на команду
 
  //проверка ответа если посылалась команда READ_OCR
  if(response == 0x00 && cmd == 58)     
  {
    tmp = spi_read();                      //прочитать один байт регистра OCR            
    if(tmp & 0x40) SDHC = 1;               //обнаружена карта SDHC 
    else           SDHC = 0;               //обнаружена карта SD
    //прочитать три оставшихся байта регистра OCR
    spi_read(); 
    spi_read(); 
    spi_read(); 
  }
 
  spi_read();
 
  CS_DISABLE; 
 
  return response;
}

//********************************************************************************************
//function	 инициализация карты памяти                         			    //
//return	 0 - карта инициализирована  					            //
//********************************************************************************************
uint8_t SD_init(void)
{
  uint8_t   i;
  uint8_t   response;
  uint8_t   SD_version = 2;	          //по умолчанию версия SD = 2
  uint16_t  retry = 0 ;
 
  spi_init();                            //инициализировать модуль SPI                        
  for(i=0;i<10;i++) spi_send(0xff);      //послать свыше 74 единиц   
 
  //выполним программный сброс карты
  CS_ENABLE;
  while(SD_sendCommand(GO_IDLE_STATE, 0)!=0x01)                                   
    if(retry++>0x20)  return 1;                    
  CS_DISABLE;
  spi_send (0xff);
  spi_send (0xff);
 
  retry = 0;                                     
  while(SD_sendCommand(SEND_IF_COND,0x000001AA)!=0x01)
  { 
    if(retry++>0xfe) 
    { 
      SD_version = 1;
      break;
    } 
  }
 
 retry = 0;                                     
 do
 {
   response = SD_sendCommand(APP_CMD,0); 
   response = SD_sendCommand(SD_SEND_OP_COND,0x40000000);
   retry++;
   if(retry>0xffe) return 1;                     
 }while(response != 0x00);                      
 
 
 //читаем регистр OCR, чтобы определить тип карты
 retry = 0;
 SDHC = 0;
 if (SD_version == 2)
 { 
   while(SD_sendCommand(READ_OCR,0)!=0x00)
	 if(retry++>0xfe)  break;
 }
 
 return 0; 
}

//********************************************************************************************
//function	 чтение выбранного сектора SD                         			    //
//аrguments	 номер сектора,указатель на буфер размером 512 байт                         //
//return	 0 - сектор прочитан успешно   					            //
//********************************************************************************************
uint8_t SD_ReadSector(uint32_t BlockNumb,uint8_t *buff)
{ 
  uint16_t i=0;
 
  //послать команду "чтение одного блока" с указанием его номера
  if(SD_sendCommand(READ_SINGLE_BLOCK, BlockNumb)) return 1;  
  CS_ENABLE;
  //ожидание  маркера данных
  while(spi_read() != 0xfe)                
  if(i++ > 0xfffe) {CS_DISABLE; return 1;}       
 
  //чтение 512 байт	выбранного сектора
  for(i=0; i<512; i++) *buff++ = spi_read();
 
  spi_read(); 
  spi_read(); 
  spi_read(); 
 
  CS_DISABLE;
 
  return 0;
}

//********************************************************************************************
//function	 запись выбранного сектора SD                         			    //
//аrguments	 номер сектора, указатель на данные для записи                              //
//return	 0 - сектор записан успешно   					            //
//********************************************************************************************
uint8_t SD_WriteSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint8_t     response;
  uint16_t    i,wait=0;
 
  //послать команду "запись одного блока" с указанием его номера
  if( SD_sendCommand(WRITE_SINGLE_BLOCK, BlockNumb)) return 1;
 
  CS_ENABLE;
  spi_send(0xfe);    
 
  //записать буфер сектора в карту
  for(i=0; i<512; i++) spi_send(*buff++);
 
  spi_send(0xff);                //читаем 2 байта CRC без его проверки
  spi_send(0xff);
 
  response = spi_read();
 
  if( (response & 0x1f) != 0x05) //если ошибка при приеме данных картой
  { CS_DISABLE; return 1; }
 
  //ожидаем окончание записи блока данных
  while(!spi_read())             //пока карта занята,она выдает ноль
  if(wait++ > 0xfffe){CS_DISABLE; return 1;}
 
  CS_DISABLE;
  spi_send(0xff);   
  CS_ENABLE;         
 
  while(!spi_read())               //пока карта занята,она выдает ноль
  if(wait++ > 0xfffe){CS_DISABLE; return 1;}
  CS_DISABLE;
 
  return 0;
}

void init()	//	инициализация периферии
{
//------------------------------------------------------------
// GPIO
//------------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);			//	сконфигурировать пины порта C
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;					//	на максимально скорости в 2МГц
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;		//	PC8 и PC9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;					//	на выход
	GPIO_Init(GPIOC, &GPIO_InitStructure);
//------------------------------------------------------------
//	UART
//------------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
//Configure GPIO pin 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;									//	Tx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;						//	альтернативный режим выхода
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;								//	Rx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;			//	вход с открытым коллектором
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	Configure UART
	USART_InitStructure.USART_BaudRate = 9600;									//	скорость 9600
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//	пакеты по 8 бит
	USART_InitStructure.USART_StopBits = USART_StopBits_1;			//	1 стоп бит
	USART_InitStructure.USART_Parity = USART_Parity_No;					//	нет контроля чётности
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//	нет аппаратного контроля передачи данных
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//	используются линии Tx и Rx

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
}

//	Задержка
void delay(unsigned long p)
{
	while(p>0){p--;}
}

//	Отправить байт в UART
void send_Uart(USART_TypeDef* USARTx, unsigned char c)
{
	while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE)== RESET){}
		USART_SendData(USART1, c);
}

//	Получить байт из UART
unsigned char getch_Uart(USART_TypeDef* USARTx)
{
	while(USART_GetFlagStatus(USARTx,USART_FLAG_RXNE) == RESET){}
	return USART_ReceiveData(USARTx);
}

//	Отправка строки в USTR
void send_Uart_str(USART_TypeDef* USARTx, unsigned char *s)
{
  while (*s != 0) 
    send_Uart(USART1, *s++);
}

uint8_t Buff[512];														//	Буфер для SD карты

void buff_clear()															//	очищаем буфер
{
	int i;
	for(i=0;i<512;i++)
	{
		Buff[i]=0;
	}
}

//	выводим число в UART. Максимальная длина числа 6 цифр
void send_int_Uart(USART_TypeDef* USARTx,unsigned long c)
{
	unsigned long d=10000000;
	do
	{
		c=c%d;
		d=d/10;
		send_Uart(USARTx,c/d+'0');
	}
	while(d>1);
}

//	Читаем число (в ASCII) с UART. Максимальной длиной в 6 цифр. 
//	Завершение ввода по вводу любого не цифрового символа
unsigned long read_int_uart(USART_TypeDef* USARTx)
{
	unsigned char temp=0,index=0;
	unsigned long value=0;
	while(index<8)
	{
		value=value*10+temp;
		temp=getch_Uart(USARTx);
		index++;
		if((47<temp)&&(temp<58))
		{
			temp=temp-48;
		}
		else
		{
			index=255;
		}
	}
	return value;
}

//	читаем текстовую строку с UART, до ввода <enter>. Максимальная длина строки 512Байт
void read_str_uart(USART_TypeDef* USARTx,unsigned char* s)
{
	unsigned char temp;
	unsigned int index=0;
	while(index<512)
	{
		temp=getch_Uart(USARTx);
		if(temp!=13)
		{
			*s++=temp;
		}
		else
		{
			index=512;
		}
		index++;
	}
}

int main(void)
{
	unsigned long address,i;
	unsigned char c;
	
	init();																		//	инициализация переферии
	if(SD_init()==0)													//	инициализация SD карты памяти
	{
		send_Uart_str(USART1,"init sd ok\n");
	}
	else
	{
		send_Uart_str(USART1,"init sd fail\n");
	}

	send_Uart_str(USART1,"alex-exe.ru");		//	выводим сообщение в UART
	while(1)
	{
		send_Uart_str(USART1,"\n----------------------------\n");
		send_Uart_str(USART1,"Read or write sd card r/w = ");
		c=getch_Uart(USART1);										//	читаем байт с uart
		while(getch_Uart(USART1)!=13){}					//	дожидаемся ввода <enter> с uart
		send_Uart_str(USART1,"\nPress enter address = ");
		buff_clear();														//	очистка буфера
		address=read_int_uart(USART1);					//	читаем число с uart, адрес сектора на sd карте
		send_Uart_str(USART1,"\nAddress = ");
		send_int_Uart(USART1,address);					//	выводим для проверки введенное число	
		send_Uart(USART1,'\n');
		if(c=='w')															//	проверка на запись или чтение карты памяти
		{
			send_Uart_str(USART1,"Press enter data blok (max 512B), to exit press <enter>\n");
			read_str_uart(USART1,Buff);						//	читаем строку с uart, окончание <enter>
			i=0;
			while((i<512)&&(Buff[i]!=0))i++;			//	ищем конец текстовой строки
			send_Uart_str(USART1,"Length text data = ");
			send_int_Uart(USART1,i);							//	выводим длину текстового послания
			send_Uart(USART1,'\n');
			if(SD_WriteSector(address, Buff)==0)	//	запись буфера на SD карту
			{
				send_Uart_str(USART1,"write sd ok\n");
			}
			else
			{
				send_Uart_str(USART1,"write sd fail\n");
			}
		}
		else
		{
			if(SD_ReadSector(address, Buff)==0)		//	чтения SD карты в буфер
			{
				send_Uart_str(USART1,"read sd ok\n");
			}
			else
			{
				send_Uart_str(USART1,"read sd fail\n");
			}
			for(i=0;i<512;i++)										//	вывод содержимого буфера в терминал
			{
				send_Uart(USART1,Buff[i]);
			}
		}
																						//	на последок мигнём светодиодом
		GPIO_SetBits(GPIOC, GPIO_Pin_8); 				//	включить синий светодиод на PC8
		delay(1000000);
		GPIO_ResetBits(GPIOC, GPIO_Pin_8);			//	выключить синий светодиод на  PC8
	}
}

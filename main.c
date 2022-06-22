#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char  buffer [33];  //用于存放转换好的十六进制字符串，可根据需要定义长度

static unsigned int g_RunTicks = 0;
/* 系统运行时间 */
static unsigned int g_PowerOnSecs  = 0;

/*
 * @fn:		void InitSystemClock(void)
 * @brief:	初始化系统时钟
 * @para:	none
 * @return:	none
 * @comment:初始化系统时钟
 */
void InitSystemClock(void)
{
	/*配置DCO为1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
   /*配置SMCLK的时钟源为DCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK的分频系数置为1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:		void InitADC(void)
 * @brief:	初始化ADC
 * @para:	none
 * @return:	none
 * @comment:初始化ADC
 */
void InitADC(void)
{
	  /*设置ADC时钟MCLK*/
	  ADC10CTL1 |= ADC10SSEL_2;
	  /*ADC 2分频*/
	  ADC10CTL1 |= ADC10DIV_0;
	  /*设置ADC基准源*/
	  ADC10CTL0 |= SREF_1;
	  /*设置ADC采样保持时间64CLK*/
	  ADC10CTL0 |= ADC10SHT_3;
	  /*设置ADC采样率200k*/
	  ADC10CTL0 &= ~ADC10SR;
	  /*ADC基准选择2.5V*/
	  ADC10CTL0 |= REF2_5V;
	  /*开启基准*/
	  ADC10CTL0 |= REFON;
	  /*选择ADC输入通道A4*/
	  ADC10CTL1 |= INCH_4;
	  /*允许A4模拟输入*/
	  ADC10AE0 |= 1 << 4;
	  /*开启ADC*/
	  ADC10CTL0 |= ADC10ON;
}

/*
 * @fn:		uint16_t GetADCValue(void)
 * @brief:	进行一次ADC转换并返回ADC转换结果
 * @para:	none
 * @return:	ADC转换结果
 * @comment:ADC转换结果为10bit，以uint16_t类型返回，低10位为有效数据
 */
uint16_t GetADCValue(void)
{
	  /*开始转换*/
	  ADC10CTL0 |= ADC10SC|ENC;
	  /*等待转换完成*/
	  while(ADC10CTL1&ADC10BUSY);
	  /*返回结果*/
	  return ADC10MEM;
}

/*
 * @fn:		void InitUART(void)
 * @brief:	初始化串口，包括设置波特率，数据位，校验位等
 * @para:	none
 * @return:	none
 * @comment:初始化串口
 */
void InitUART(void)
{
    /*复位USCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*选择USCI_Ax为UART模式*/
    UCA0CTL0 &= ~UCSYNC;

    /*配置UART时钟源为SMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*配置波特率为9600@1MHz*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;
    /*使能端口复用*/
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    /*清除复位位，使能UART*/
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:		void UARTSendString(uint8_t *pbuff,uint8_t num)
 * @brief:	通过串口发送字符串
 * @para:	pbuff:指向要发送字符串的指针
 * 			num:要发送的字符个数
 * @return:	none
 * @comment:通过串口发送字符串
 */
void UARTSendString(uint8_t *pbuff,uint8_t num)
{
	uint8_t cnt = 0;
	for(cnt = 0;cnt < num;cnt ++)
	{
		while(UCA0STAT & UCBUSY);
		UCA0TXBUF = *(pbuff + cnt);
	}
}

/*
 * @fn:		void PrintNumber(uint16_t num)
 * @brief:	通过串口发送数字
 * @para:	num:变量
 * @return:	none
 * @comment:通过串口发送数字
 */
void PrintNumber(uint16_t num)
{
	uint8_t buff[6] = {0,0,0,0,0,'\n'};
	uint8_t cnt = 0;
	for(cnt = 0;cnt < 5;cnt ++)
	{
		buff[4 - cnt] = (uint8_t)(num % 10 + '0');
		num /= 10;
	}
	UARTSendString(buff,6);
}

/*
 * @fn:		void PrintFloat(float num)
 * @brief:	通过串口发送浮点数
 * @para:	num:变量
 * @return:	none
 * @comment:通过串口发送浮点数，可发送1位整数位+3位小数位
 */
void PrintFloat(float num)
{
	uint8_t charbuff[] = {0,'.',0,0,0};
	uint16_t temp = (uint16_t)(num * 1000);
	charbuff[0] = (uint8_t)(temp / 1000) + '0';
	charbuff[2] = (uint8_t)((temp % 1000) / 100) + '0';
	charbuff[3] = (uint8_t)((temp % 100) / 10) + '0';
	charbuff[4] = (uint8_t)(temp % 10) + '0';
	UARTSendString(charbuff,5);
}

static int Timer_Init()
{
    //BCSCTL2 |=SELS;//SMCLK设置为外部32768HZ
    TACTL = TASSEL_2 + TACLR ;//定时器A时钟源为SMLCK,计数器清零
    CCTL0 = CCIE;//使能捕获/比较中断
    //CCR0 = 32768;//定时1S
    CCR0 = 5000;  // 2500: 5ms; 5000: 10ms; 1000: 2ms (2ms once interrupt)
    TACTL |= MC_1;//增计数模式
    
    return 0;
}

void initGPIO()
{	
	P2DIR |= BIT5;	/* 运行灯 */						
	P2OUT &= ~BIT5;
        
        /*初始化P2.3为SYNC*/
	P2DIR |= BIT3;
	/*初始化P2.3输出高电平*/
	P2OUT |= BIT3;
        
        P1DIR |= BIT5 | BIT7;//时钟输出和MOSI输出
        P1DIR &= ~BIT6; //MISO输入
        
        P1OUT &= ~(BIT5 + BIT7);  /* 输出低 */
}

void SPISendByte(uint16_t dat)//发送16位数据(高位最先发送)
{
    unsigned char i;
    
    P2OUT &= ~BIT3; //SYNC拉低
    
    for (i = 16; i > 0; i--)
    {
        P1OUT |= BIT5;
        __delay_cycles(2);
        
        if ((dat & 0x8000) == 0x8000) //高位是1把data中的高位发送出去
            P1OUT |= BIT7;      //P1.7OUT1
        else
            P1OUT &=~ BIT7;     //P1.7OUT0

        P1OUT &=~ BIT5; //下降沿
        __delay_cycles(2);
        
        dat = dat << 1;//data中的数据左移
    }
    P2OUT |= BIT3;
    __delay_cycles(1);
    
    return;
}

uint16_t SPIReceiveByte(void)
{
    unsigned char i;
    uint16_t tempbit = 0, tempData = 0;
    
    P2OUT &= ~BIT3; //SYNC拉低
    __delay_cycles(1);
    for (i = 16; i > 0; i--)
    {
        P1OUT |= BIT5;
        __delay_cycles(5);
        
        
        P1OUT &= ~BIT5;         //下降沿读
        __delay_cycles(2);
        
        if ( (P1IN & BIT6) == 0x00)//判断收到的电平信号
        {
            tempbit = 0;
        }
        else
            tempbit = 1;
        
        tempData = ((tempData << 1) | tempbit);//数据读出操作
    }
    
    P2OUT |= BIT3;
    __delay_cycles(1);
    
    return tempData;
}

char  *inttohex( int  aa)
{
     sprintf (buffer, "%x" ,aa);
     return  (buffer);
}

/*
 * main.c
 */
int main(void)
{
	float voltage0 = 0;
        float voltage1 = 0;
	float r = 0;
	uint16_t adcvalue = 0;
                     
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
        
        InitSystemClock();
        initGPIO();
        InitUART();
        Timer_Init();
	InitADC();
        _EINT();//开总中断  
        SPISendByte(0x1803);
        SPISendByte(0x7FF);
	while (1)
	{           
            adcvalue = GetADCValue();
            voltage0 = adcvalue * 2.5 / 1023; //测到的实际电压值
            voltage1 = voltage0 / 2.143 * 1023; //要输出给电位器的值
           
            r = round(voltage1);//四舍五入取整          
            int x = (int)r;
            x +=1024;           
            SPISendByte(0x1803);
            SPISendByte(x);
            SPISendByte(0x800);
            
            
            
            __delay_cycles(500000);
	}
}


#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A(void)
{
	g_RunTicks++;

    if ( g_RunTicks % 100 == 0 )
    {
        g_PowerOnSecs++;  
        if ( g_PowerOnSecs % 2 == 0 )
        {
          g_RunTicks = 0;
          g_PowerOnSecs = 0;
            P2OUT |= BIT5;
        }
        else
        {
            P2OUT &= ~BIT5;
        }
    }
}


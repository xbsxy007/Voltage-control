#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char  buffer [33];  //���ڴ��ת���õ�ʮ�������ַ������ɸ�����Ҫ���峤��

static unsigned int g_RunTicks = 0;
/* ϵͳ����ʱ�� */
static unsigned int g_PowerOnSecs  = 0;

/*
 * @fn:		void InitSystemClock(void)
 * @brief:	��ʼ��ϵͳʱ��
 * @para:	none
 * @return:	none
 * @comment:��ʼ��ϵͳʱ��
 */
void InitSystemClock(void)
{
	/*����DCOΪ1MHz*/
    DCOCTL = CALDCO_1MHZ;
    BCSCTL1 = CALBC1_1MHZ;
   /*����SMCLK��ʱ��ԴΪDCO*/
    BCSCTL2 &= ~SELS;
    /*SMCLK�ķ�Ƶϵ����Ϊ1*/
    BCSCTL2 &= ~(DIVS0 | DIVS1);
}

/*
 * @fn:		void InitADC(void)
 * @brief:	��ʼ��ADC
 * @para:	none
 * @return:	none
 * @comment:��ʼ��ADC
 */
void InitADC(void)
{
	  /*����ADCʱ��MCLK*/
	  ADC10CTL1 |= ADC10SSEL_2;
	  /*ADC 2��Ƶ*/
	  ADC10CTL1 |= ADC10DIV_0;
	  /*����ADC��׼Դ*/
	  ADC10CTL0 |= SREF_1;
	  /*����ADC��������ʱ��64CLK*/
	  ADC10CTL0 |= ADC10SHT_3;
	  /*����ADC������200k*/
	  ADC10CTL0 &= ~ADC10SR;
	  /*ADC��׼ѡ��2.5V*/
	  ADC10CTL0 |= REF2_5V;
	  /*������׼*/
	  ADC10CTL0 |= REFON;
	  /*ѡ��ADC����ͨ��A4*/
	  ADC10CTL1 |= INCH_4;
	  /*����A4ģ������*/
	  ADC10AE0 |= 1 << 4;
	  /*����ADC*/
	  ADC10CTL0 |= ADC10ON;
}

/*
 * @fn:		uint16_t GetADCValue(void)
 * @brief:	����һ��ADCת��������ADCת�����
 * @para:	none
 * @return:	ADCת�����
 * @comment:ADCת�����Ϊ10bit����uint16_t���ͷ��أ���10λΪ��Ч����
 */
uint16_t GetADCValue(void)
{
	  /*��ʼת��*/
	  ADC10CTL0 |= ADC10SC|ENC;
	  /*�ȴ�ת�����*/
	  while(ADC10CTL1&ADC10BUSY);
	  /*���ؽ��*/
	  return ADC10MEM;
}

/*
 * @fn:		void InitUART(void)
 * @brief:	��ʼ�����ڣ��������ò����ʣ�����λ��У��λ��
 * @para:	none
 * @return:	none
 * @comment:��ʼ������
 */
void InitUART(void)
{
    /*��λUSCI_Ax*/
    UCA0CTL1 |= UCSWRST;

    /*ѡ��USCI_AxΪUARTģʽ*/
    UCA0CTL0 &= ~UCSYNC;

    /*����UARTʱ��ԴΪSMCLK*/
    UCA0CTL1 |= UCSSEL1;

    /*���ò�����Ϊ9600@1MHz*/
    UCA0BR0 = 0x68;
    UCA0BR1 = 0x00;
    UCA0MCTL = 1 << 1;
    /*ʹ�ܶ˿ڸ���*/
    P1SEL |= BIT1 + BIT2;
    P1SEL2 |= BIT1 + BIT2;
    /*�����λλ��ʹ��UART*/
    UCA0CTL1 &= ~UCSWRST;
}

/*
 * @fn:		void UARTSendString(uint8_t *pbuff,uint8_t num)
 * @brief:	ͨ�����ڷ����ַ���
 * @para:	pbuff:ָ��Ҫ�����ַ�����ָ��
 * 			num:Ҫ���͵��ַ�����
 * @return:	none
 * @comment:ͨ�����ڷ����ַ���
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
 * @brief:	ͨ�����ڷ�������
 * @para:	num:����
 * @return:	none
 * @comment:ͨ�����ڷ�������
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
 * @brief:	ͨ�����ڷ��͸�����
 * @para:	num:����
 * @return:	none
 * @comment:ͨ�����ڷ��͸��������ɷ���1λ����λ+3λС��λ
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
    //BCSCTL2 |=SELS;//SMCLK����Ϊ�ⲿ32768HZ
    TACTL = TASSEL_2 + TACLR ;//��ʱ��Aʱ��ԴΪSMLCK,����������
    CCTL0 = CCIE;//ʹ�ܲ���/�Ƚ��ж�
    //CCR0 = 32768;//��ʱ1S
    CCR0 = 5000;  // 2500: 5ms; 5000: 10ms; 1000: 2ms (2ms once interrupt)
    TACTL |= MC_1;//������ģʽ
    
    return 0;
}

void initGPIO()
{	
	P2DIR |= BIT5;	/* ���е� */						
	P2OUT &= ~BIT5;
        
        /*��ʼ��P2.3ΪSYNC*/
	P2DIR |= BIT3;
	/*��ʼ��P2.3����ߵ�ƽ*/
	P2OUT |= BIT3;
        
        P1DIR |= BIT5 | BIT7;//ʱ�������MOSI���
        P1DIR &= ~BIT6; //MISO����
        
        P1OUT &= ~(BIT5 + BIT7);  /* ����� */
}

void SPISendByte(uint16_t dat)//����16λ����(��λ���ȷ���)
{
    unsigned char i;
    
    P2OUT &= ~BIT3; //SYNC����
    
    for (i = 16; i > 0; i--)
    {
        P1OUT |= BIT5;
        __delay_cycles(2);
        
        if ((dat & 0x8000) == 0x8000) //��λ��1��data�еĸ�λ���ͳ�ȥ
            P1OUT |= BIT7;      //P1.7OUT1
        else
            P1OUT &=~ BIT7;     //P1.7OUT0

        P1OUT &=~ BIT5; //�½���
        __delay_cycles(2);
        
        dat = dat << 1;//data�е���������
    }
    P2OUT |= BIT3;
    __delay_cycles(1);
    
    return;
}

uint16_t SPIReceiveByte(void)
{
    unsigned char i;
    uint16_t tempbit = 0, tempData = 0;
    
    P2OUT &= ~BIT3; //SYNC����
    __delay_cycles(1);
    for (i = 16; i > 0; i--)
    {
        P1OUT |= BIT5;
        __delay_cycles(5);
        
        
        P1OUT &= ~BIT5;         //�½��ض�
        __delay_cycles(2);
        
        if ( (P1IN & BIT6) == 0x00)//�ж��յ��ĵ�ƽ�ź�
        {
            tempbit = 0;
        }
        else
            tempbit = 1;
        
        tempData = ((tempData << 1) | tempbit);//���ݶ�������
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
        _EINT();//�����ж�  
        SPISendByte(0x1803);
        SPISendByte(0x7FF);
	while (1)
	{           
            adcvalue = GetADCValue();
            voltage0 = adcvalue * 2.5 / 1023; //�⵽��ʵ�ʵ�ѹֵ
            voltage1 = voltage0 / 2.143 * 1023; //Ҫ�������λ����ֵ
           
            r = round(voltage1);//��������ȡ��          
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


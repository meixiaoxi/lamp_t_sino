#include <mc30p011.h>


unsigned char gLedWave;   //��¼��ʱ��led���ȱ仯����
unsigned char gLedStrength;    //��ǰled����ֵ
unsigned short g3STick;   // 3S �ļ���
unsigned char gSysReadPA;
unsigned int gCountINT;
unsigned char gCountCHAR;
unsigned char gLampMode;


unsigned char gStrengthTemp;

unsigned int gCrcCode;
unsigned char gLedStatus;


#define EnWatchdog()            WDTEN = 1
#define DisWatchdog()   WDTEN = 0

#define key_interrupt_enable()  KBIE = 1
#define key_interrupt_disable() KBIE = 0

#define pwm_stop()    PWMOUT = 0  //�ر�PWM
#define pwm_start()   PWMOUT = 1  //����PWM

#define Stop()  __asm__("stop")
#define ClrWdt()        __asm__("clrwdt")
#define NOP()   __asm__("nop")


#define PWM_NUM_START_LOAD              70

#define LED_MIN_LEVEL   7
#define LED_MAX_LEVEL   254
#define LED_DEFAULT_LEVEL       200

#define KEY_PRESS       0
#define KEY_RELEASE 1

#define LED_STRENGTH_UP 0
#define LED_STRENGTH_DN 1

//EEPROM ��ַ����
#define ADDR_STRENGTH_FLAG      0x00   //led strengtrh �Ƿ���Ч
#define ADDR_STRENGTH 0x09              //led ����

#define  ADDR_ONOFF_FLAG        0x12    //����״̬

#define LED_PRE_ON      0x33
#define LED_NOW_ON 0x33

#define LED_NOW_OFF 0xAA
#define LED_PRE_OFF 0xAA

//led ��/��״̬
#define LED_ON  0
#define LED_OFF 1

//lampģʽ
#define ADJUST_MODE 0  //����
#define HYPNOSIS_MODE   1 //����
#define BREATHE_MODE    1 //����
#define NIGHT_MODE              2 //Сҹ��

#define LOAD_CTL_TICK           183   //   3000/16.384
        
                                                        //488 // 8000/16.384

                                                        //610  // 10000/16.384  
                                                        //305            //  5000/16.384
                                                
#define SHORT_PRESS_TICK        30  // 500/16.384

#define POWEROFF_SHORT_PRESS    0
#define POWEROFF_LONG_PRESS     1
#define LED_POWEROFF_TYPE  0   // 0 short press to poweroff
                                                     // 1  long press to poweroff

#define POWER_ON        0
#define POWER_OFF       1
#define POWER_NIGHT     2


#define P_KEY   P13
#define LoadCtl                          P14
#define CurCtl                  P15
#define PWM_IO                  P12
// p17 data
//p00 clk

/*
#define SDA_H()                 (P17 = 1)
#define SDA_L()                 (P17 = 0)

#define SCL_H()                 (P00 = 1)
#define SCL_L()                 (P00 = 0)

#define GET_SDA()               (P17)

#define CHG_SDA_OUT()   (DDR17 = 0)
#define CHG_SDA_IN()    (DDR17 = 1)
*/
#define SDA_H()                 (P10 = 1)
#define SDA_L()                 (P10 = 0)

#define SCL_H()                 (P11 = 1)
#define SCL_L()                 (P11 = 0)

#define GET_SDA()               (P10)

#define CHG_SDA_OUT()   (DDR10 = 0)
#define CHG_SDA_IN()    (DDR10 = 1)


#define IIC_ADDR  0xA0

#define _nop_() __asm__("nop")
#define swait_uSec(n) Wait_uSec(n)

const unsigned char markbit[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};


unsigned int temp_int_1;
unsigned char temp_char_1;
unsigned char temp_char_2;
unsigned char temp_char_3;
unsigned char temp_char_delay;

unsigned char   StatusBuf,ABuf;
void isr(void) __interrupt
{
         __asm
                movra   _ABuf
                swapar  _STATUS
                movra   _StatusBuf
        __endasm;
        if( KBIF&&KBIE) 
        {
                KBIF = 0;               //���KINT�ⲿ�����жϱ�־
        }

        if(T0IF)
        {
                T0IF = 0;

                g3STick++;
                gCountCHAR++;
                if(gCountCHAR == 0xFF)
                        gCountINT++;
        }
        __asm
                swapar  _StatusBuf
                movra   _STATUS
                swapr   _ABuf
                swapar  _ABuf
        __endasm;     
}

void    Wait_uSec(unsigned int DELAY)
{
     

         temp_int_1 = DELAY<<1;                          // by 16MHz IRC OSC
      
         while(temp_int_1>0)  temp_int_1--;                    //
}
void delay_ms(unsigned int count)
{

        while(count>0)
        {
                count--;
                for(temp_char_delay =0 ;temp_char_delay < 250; temp_char_delay++);
                ClrWdt();
        }
        ClrWdt();
}

static void IIC_START()
{
        SDA_H();
        SCL_H();
        _nop_();
        SDA_L();
        Wait_uSec(1);   
        SCL_L();
        //_nop_();
}       


static void IIC_STOP()
{
        SCL_L();
        SDA_L();
        CHG_SDA_OUT();
        Wait_uSec(1);
        SCL_H();
        Wait_uSec(1);
        SDA_H();
        //Wait_uSec(2);
}

static void IIC_SEND_ACK()
{
        SCL_L();
        CHG_SDA_OUT();
        SDA_H();
        swait_uSec(4);  
        SCL_H();
        Wait_uSec(1);
        //SCL_L();
}

static char IIC_CHECK_ACK()
{
        //char ret=0;

        SCL_L();
        CHG_SDA_IN();
        swait_uSec(3);  
        SCL_H();
        swait_uSec(3);
        if(GET_SDA() == 0)
        {
                temp_char_1= 1;
        }
        else
        {
                temp_char_1 = 0;
        }
        //SCL_L();
        //SDA_H();
        //CHG_SDA_OUT();
        
        return temp_char_1;
}


static char IIC_SEND_BYTE(unsigned char val)
{
        //signed char i; temp_char_1
        CHG_SDA_OUT();
        for (temp_char_1 = 8; temp_char_1 > 0; temp_char_1--)
        {
                SCL_L();
                if(val & markbit[temp_char_1-1])
                {
                        SDA_H();
                }
                else
                {
                        SDA_L();
                }
                /*
                if(i == 7)
                {
                        CHG_SDA_OUT();
                }
                else
                {
                        _nop_();
                }
                */
                _nop_();
                _nop_();
                _nop_();
                SCL_H();
                swait_uSec(2);          
                //SCL_L();
                //Wait_uSec(2);
        }
        return IIC_CHECK_ACK();
}

static unsigned char IIC_GET_BYTE()
{
        //signed char i;   temp_char_1
        //unsigned char rdata = 0;   temp_char_2

        temp_char_2 =0;
        
        SCL_L();
        //CHG_SDA_IN();
        for(temp_char_1=8; temp_char_1>0; temp_char_1--)
        {
                SCL_L();
                swait_uSec(3);
                SCL_H();
                _nop_();
                _nop_();
                _nop_();
                if(GET_SDA())
                {
                   temp_char_2 = temp_char_2 | markbit[temp_char_1-1];
                }
        }

        return temp_char_2;
}


void  I2C_write(unsigned char reg, unsigned char val)
{
        //signed char ret = 0;

        GIE = 0;
        
        IIC_START();

        #if 0
        if(!IIC_SEND_BYTE(IIC_ADDR))
                ret = -1;
        if(!IIC_SEND_BYTE(reg))
                ret = -2;
        if(!IIC_SEND_BYTE(val))
                ret = -3;
        #else
        IIC_SEND_BYTE(IIC_ADDR);
        IIC_SEND_BYTE(reg);
        IIC_SEND_BYTE(val);
        #endif
        IIC_STOP();
        Wait_uSec(2);
        delay_ms(100);
        GIE = 1;
        //return ret;
}

short I2C_read(unsigned char reg)
{
        //unsigned char val;    temp_char_3
        //signed short ret = 0; temp_char_2

        temp_char_2 = 0;

        GIE = 0;

        IIC_START();
        #if 0
        if(!IIC_SEND_BYTE(IIC_ADDR)) ret = -1;
        if(!IIC_SEND_BYTE(reg)) ret = -2;
        IIC_STOP();
        Wait_uSec(1);
        IIC_START();
        if(!IIC_SEND_BYTE(IIC_ADDR+1)) ret = -3;
        val =IIC_GET_BYTE();
        IIC_SEND_ACK();
        IIC_STOP();
        Wait_uSec(10);
        #else
        IIC_SEND_BYTE(IIC_ADDR);
        IIC_SEND_BYTE(reg);
        IIC_STOP();
        Wait_uSec(1);
        IIC_START();    
        IIC_SEND_BYTE(IIC_ADDR+1);
        temp_char_3 =IIC_GET_BYTE();
        IIC_SEND_ACK();
        IIC_STOP();
        #endif
        Wait_uSec(10);
        GIE = 1;

        return (short)temp_char_3;
}

void EnterNightMode()
{
        gStrengthTemp = T1DATA;
        LoadCtl =1;
        //SlowChangeStrength(POWER_NIGHT);
        pwm_stop();
        T1DATA = LED_MIN_LEVEL;
        pwm_start();
        CurCtl= 0;
        gLampMode = NIGHT_MODE;
}

void SlowChangeStrength(unsigned char type)
{

     //    temp  = 0;
       //  count = 0;

        temp_char_1 =0 ;
        temp_char_2 = 0;
         
                if(type == POWER_ON)
                {
                        while(1)
                        {
                                if(temp_char_1> gLedStrength)
                                        break;
                                T1DATA = temp_char_1;
                                    temp_char_1++;

                                /*
                                if(temp < 70)
                                        delay_ms(30);
                                else
                                        delay_ms(30);
                                */
                                delay_ms(20);
                                if(P_KEY == 0)
                                {       
                                        temp_char_2++;
                                }
                                else if(temp_char_2 == 0)
                                {
                                        continue;
                                }
                                else 
                                {
                                        if((temp_char_2>2) && (temp_char_2 <50))   //short key press
                                        {
                                                //enter Сҹ��ģʽ
                                                //EnterNightMode();
                                                return;
                                        }
                                        temp_char_2 =0;       
                                }
                        }

                }
                else if(type == POWER_OFF)
                {
                        temp_char_1 = T1DATA;
                        
                        while(1)
                        {
                                if(type == POWER_OFF)
                                {
                                        if(temp_char_1 == 0)
                                                break;
                                }
                                else
                                {
                                        if(temp_char_1==LED_MIN_LEVEL-1)
                                                break;
                                }
                                pwm_stop();
                                T1DATA = temp_char_1;
                                   temp_char_1--;
                                pwm_start();

                                delay_ms(20);


                                if(P_KEY == 0)
                                {       
                                        temp_char_2++;
                                }
                                else if(temp_char_2 == 0)
                                {
                                        continue;
                                }
                                else
                                {
                                        if(temp_char_2>=2 && temp_char_2 <50)   //short key press
                                        {
                                                if(type == POWER_NIGHT)
                                                {
                                                        #if defined(BREATHE_MODE_SUPPORT)
                                                        EnterBreatheMode();
                                                        #elif defined(HYPNOSIS_MODE_SUPPORT)
                                                        EnterHypnosisMode();
                                                        #endif
                                                        return;
                                                }
                                        }
                                }
                        /*
                                if(temp < 50)
                                {
                                        delay_ms(40);
                                        temp --;
                                }
                                else
                                {
                                        temp = temp -4;
                                        delay_ms(1);
                                }
                        */
/*                              if(PA3 == 0)
                                {       
                                        delay_ms(10);
                                        if(PA3 == 0)
                                                return;
                                }
                                */
                        }
                }

        
}

void factoryReset()
{
        I2C_write(ADDR_STRENGTH_FLAG, 0x00);   //clear our flag
        GIE =0;
        
        T1DATA = 150;
        pwm_start();

        do{
                delay_ms(300);
                pwm_start();
                delay_ms(300);
                pwm_stop();
                temp_char_1++;
           }while(temp_char_1<3);
        while(1)
                Stop();
}

void LampPowerOFF()
{
//      pwm_stop();
        LoadCtl = 0;
        gCountCHAR = gCountINT= 0;
        g3STick = 0;
        gLampMode = ADJUST_MODE;
        PWM_IO = 0;
        gLedStrength = gStrengthTemp;
        if(gLedStrength<LED_MIN_LEVEL|| gLedStrength>LED_MAX_LEVEL)
                gLedStrength = LED_DEFAULT_LEVEL;
        DisWatchdog();

        SlowChangeStrength(POWER_OFF);

        gLedStatus = LED_NOW_OFF;
        
        pwm_stop();
        CurCtl= 1;
        while(P_KEY==0){};
        //delay_ms(5);
        key_interrupt_enable();
        NOP();
        Stop();
        
        key_interrupt_disable();
        
        DisWatchdog();

        g3STick =0;
        while(P_KEY == 0) //wait key release
        {
                if(g3STick > 183)    //   3s/16.384ms  factoryReset();
                {
                        factoryReset();
                }
        }
        
        EnWatchdog();
        
        pwm_start();

        gLedStatus = LED_NOW_ON;
        SlowChangeStrength(POWER_ON);

        if(T1DATA != gLedStrength)
        {
                EnterNightMode();
        }
        if(T1DATA <=PWM_NUM_START_LOAD)
        {
                LoadCtl = 1;
        }
}



void changeLampStrength()
{
//      unsigned char temp = 0;
        temp_char_3 = 0;
        if(gLedWave == LED_STRENGTH_UP)
        {
                gLedStrength = gLedStrength + 1;
                if(gLedStrength >= LED_MAX_LEVEL)
                {
                        #if 1
                                        gLedStrength = gLedStrength -1;
                                        do{
                                                pwm_stop();
                                                delay_ms(400);
                                                pwm_start();
                                                delay_ms(400);
                                                temp_char_3++;
                                        }while(temp_char_3<3);
                                        gLedWave = LED_STRENGTH_DN;
                        #endif
                }               
        }
        else
        {
                gLedStrength = gLedStrength - 1;
                if(gLedStrength  == (LED_MIN_LEVEL -1))
                {
                        #if 1
                                        gLedStrength = LED_MIN_LEVEL;
                                        do{
                                                pwm_stop();
                                                delay_ms(400);
                                                pwm_start();
                                                delay_ms(400);
                                                temp_char_3++;
                                        }while(temp_char_3<3);

                                        gLedWave = LED_STRENGTH_UP;
                        #endif
                }       
        }

        pwm_stop();
        T1DATA  = gLedStrength;//gStrengthBuf[gLedStrength];
        pwm_start();
        if(T1DATA<=PWM_NUM_START_LOAD)
                LoadCtl=1;
        else
                LoadCtl=0;
}


void delay_with_key_detect()
{
//      int mCount = 0;
//      unsigned char isLongPress = 0;
        temp_char_1 = 0;
        temp_char_2 = 3;
        delay_ms(10);
        if(P_KEY != 0)  //��������
                return;
         temp_char_2 = 0;
        while(1)
        {
                delay_ms(10);
                if(P_KEY == 1)   //key is released
                        break;
                temp_char_1++;
                if(temp_char_2 == 0)
                {
                        if(temp_char_1>=100)    // 1s
                        {
                                if(gLampMode != ADJUST_MODE)
                                        return;                 
                                temp_char_2 = 1;
                                temp_char_1 =0;
                                changeLampStrength();
                        }
                }
                else
                {
                        if(gLedStrength >150)
                        {
                                if(temp_char_1 >= 4)  //10ms
                                {
                                        temp_char_1 = 0;
                                        changeLampStrength();
                                }
                        }
                        else
                        {
                                if(temp_char_1 >= 6)  //60ms
                                {
                                        temp_char_1 = 0;
                                        changeLampStrength();
                                }
                        }
                }

        //      LoadCtlDetect();

        }

        

}


void InitConfig()
{
        g3STick = 0;
        gLampMode = ADJUST_MODE;
         gCountCHAR =0;
         gCountINT= 0;
         gLedWave = LED_STRENGTH_UP;
        //KIN mask
        //initial value, code space
        /*
        KMSK0 = 0;
        KMSK2 = 0;
        KMSK4 = 0;

        KMSK3 = 0;  //key
        */
        KBIE = 0;  
        KBIM3 = 1; 

        DDR0 = 0;
        DDR1 = 0x48;   // 01001000

   //     PUCON = 0x0;    //������

       // P1 = 0;

         CurCtl=1;

        //PWM
        T1CR   = 0xC0;
        T1LOAD = 249;
        T1DATA = 0;

        T0CR = 0x07;
        T0IE = 1;
        
        
        GIE = 1;                    //ʹ��ȫ���ж�
}


void main()
{
        
        
        
        
        
        InitConfig();    //��ʼ������
        DisWatchdog();
/*
        while(1)
        {
                if(gCountINT >=1)
                {
                        //      I2C_write(0x00,0xAA);

                        delay_ms(500);

                           readByte = I2C_read(0x00);
                        gCountINT = 0;
                }
                        
        }

*/
        if(gCrcCode  != 0x51AE)
        {
                gCrcCode = 0x51AE; 
                gLedStatus = LED_NOW_ON;
        }
        else if(gLedStatus == LED_PRE_ON)
        {
                gLedStatus = LED_NOW_OFF;
                key_interrupt_enable();
                Stop();
                key_interrupt_disable();

                DisWatchdog();   //�����жϻ���֮��ϵͳ��Ĭ�Ͻ�RCEN��1
                g3STick = 0;
                while(P_KEY == 0) //wait key release
                {
                        if(g3STick > 183)    //   3s/16.384ms  factoryReset();
                        {
                                factoryReset();
                        }
                }
        //      g3STick =0;
        }


        EnWatchdog();
        
        //check whether strength exists, if not, use default strength
        gLedStrength = I2C_read(ADDR_STRENGTH_FLAG);
        if(gLedStrength == 0xAB)  //ok, it's our flag
        {
                //delay_ms(5);
                gLedStrength = I2C_read(ADDR_STRENGTH);
        }
        else
        {
                I2C_write(ADDR_STRENGTH_FLAG, 0xAB);  //write our flag
                gLedStrength = LED_MAX_LEVEL;  //max level
                //delay_ms(5);
                I2C_write(ADDR_STRENGTH,254);
        }

        //�ߴ�����  
   if(gLedStrength > LED_MAX_LEVEL|| gLedStrength < LED_MIN_LEVEL)
   {
        gLedStrength = LED_DEFAULT_LEVEL;
   }

         pwm_start();

        gLedStatus = LED_NOW_ON;
        SlowChangeStrength(POWER_ON);
        if(T1DATA != gLedStrength)
        {
                EnterNightMode();
        }
        if(T1DATA <=PWM_NUM_START_LOAD)
        {
                LoadCtl = 1;
        }
        
        
        //main loop
        while(1)
        {
                if(P_KEY == 0)   //key press
                {
                        delay_with_key_detect();

                         if(temp_char_2 == 0)   //short press
                {
                                if(gLampMode == ADJUST_MODE)
                                {
                                //enter Сҹ��ģʽ
                                        EnterNightMode();
                         }
                                else            
                                        LampPowerOFF();
                        }
                        else if(gLampMode ==  ADJUST_MODE)  //clear
                        {       
                                        gCountCHAR =0;
                                gCountINT =0;
                                        // lamp strength has been changed, we need save
                                        I2C_write(ADDR_STRENGTH,gLedStrength);                                          
                        }
               }
                
                 if(gLampMode == ADJUST_MODE) 
                {
                        if(gCountINT >= 2647)      // 3 Hour  3*60*60*1000/16.384/255 = 2647
                                LampPowerOFF();
                }

                //LoadCtlDetect();


                ClrWdt();
        }
     
}


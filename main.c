#include <mc30p011.h>


unsigned char gLedWave;   //记录此时的led亮度变化趋势
unsigned char gLedStrength;    //当前led亮度值
unsigned short g3STick;   // 3S 的计数
unsigned char gSysReadPA;

unsigned char gLampMode;


unsigned char gStrengthTemp;

unsigned int gCrcCode;
unsigned char gLedStatus;


#define EnWatchdog()            WDTEN = 1
#define DisWatchdog()   WDTEN = 0

#define key_interrupt_enable()  KBIE = 1
#define key_interrupt_disable() KBIE = 0

#define pwm_stop()    PWMOUT = 0  //关闭PWM
#define pwm_start()   PWMOUT = 1  //启动PWM

#define Stop()  __asm__("stop")
#define ClrWdt()        __asm__("clrwdt")
#define NOP()   __asm__("nop")


#define PWM_NUM_START_LOAD              70

#define LED_MIN_LEVEL   7
#define LED_MAX_LEVEL   255
#define LED_DEFAULT_LEVEL       200

#define KEY_PRESS       0
#define KEY_RELEASE 1

#define LED_STRENGTH_UP 0
#define LED_STRENGTH_DN 1

//EEPROM 地址分配
#define ADDR_STRENGTH_FLAG      0x00   //led strengtrh 是否有效
#define ADDR_STRENGTH 0x09              //led 亮度

#define  ADDR_ONOFF_FLAG        0x15    //亮灭状态

#define LED_PRE_ON      0x33
#define LED_NOW_ON 0x33

#define LED_NOW_OFF 0xAA
#define LED_PRE_OFF 0xAA

//led 亮/灭状态
#define LED_ON  0
#define LED_OFF 1

//lamp模式
#define ADJUST_MODE 0  //调光
#define HYPNOSIS_MODE   1 //催眠
#define BREATHE_MODE    1 //呼吸
#define NIGHT_MODE              2 //小夜灯

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


#define SDA_H()                 (P17 = 1)
#define SDA_L()                 (P17 = 0)

#define SCL_H()                 (P00 = 1)
#define SCL_L()                 (P00 = 0)

#define GET_SDA()               (P17)

#define CHG_SDA_OUT()   (DDR17 = 0)
#define CHG_SDA_IN()    (DDR17 = 1)

/*
#define SDA_H()                 (P10 = 1)
#define SDA_L()                 (P10 = 0)

#define SCL_H()                 (P11 = 1)
#define SCL_L()                 (P11 = 0)

#define GET_SDA()               (P10)

#define CHG_SDA_OUT()   (DDR10 = 0)
#define CHG_SDA_IN()    (DDR10 = 1)
*/

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
                KBIF = 0;               //清除KINT外部按键中断标志
        }

        if(T0IF)
        {
                T0IF = 0;

                g3STick++;
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
        CHG_SDA_IN();   //xyl change
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
/*
void EnterNightMode()
{
        gStrengthTemp = gLedStrength;
        LoadCtl =1;
        //SlowChangeStrength(POWER_NIGHT);
        pwm_stop();
        T1DATA = LED_MIN_LEVEL;
        pwm_start();
        CurCtl= 0;
        gLampMode = NIGHT_MODE;
}
*/
void SlowChangeStrength(unsigned char type)
{

     //    temp  = 0;
       //  count = 0;

        temp_char_1 =0 ;
        temp_char_2 = 0;

        if(gLedStrength ==  8)
                CurCtl = 0;
        
                if(type == POWER_ON)
                {
                        while(1)
                        {
                                if(temp_char_1> gLedStrength)
                                        break;
                                T1DATA = temp_char_1;
                                    if(temp_char_1 == 255)
                                                break;
                                                                
                                 temp_char_1++;

                                if(temp_char_1 < 170)
                                        delay_ms(60);
                                else
                                        delay_ms(30);

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
                                                //enter 小夜灯模式
                                                //EnterNightMode();
                                                return;
                                        }
                                        temp_char_2 =0;       
                                }
                        }

                }
                else if(type == POWER_OFF)
                {
                        T1DATA = 0;
                         #if 0
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

                                delay_ms(10);


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
                                                        /*
                                                        #if defined(BREATHE_MODE_SUPPORT)
                                                        EnterBreatheMode();
                                                        #elif defined(HYPNOSIS_MODE_SUPPORT)
                                                        EnterHypnosisMode();
                                                        #endif
                                                        return;
                                                        */
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
                    #endif
                }

        
}

void factoryReset()
{
        GIE =0;
        
        //pwm_start();
          temp_char_1=0;
        do{
                  T1DATA = 150;
                  delay_ms(1);
                pwm_start();
                  delay_ms(300);
                    T1DATA = 0;
                    delay_ms(1);
                  pwm_stop();
                delay_ms(300);
                temp_char_1++;
           }while(temp_char_1<3);
        while(1)
                Stop();
}

void LampPowerOFF()
{
//      pwm_stop();
        LoadCtl = 0;
        g3STick = 0;
        gLampMode = ADJUST_MODE;
        PWM_IO = 0;
        if(gLedStrength<8)
                gLedStrength = LED_DEFAULT_LEVEL;
        DisWatchdog();

        I2C_write(ADDR_ONOFF_FLAG, LED_NOW_OFF);
        SlowChangeStrength(POWER_OFF);

        //gLedStatus = LED_NOW_OFF;

        T1DATA = 0;
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
                if(g3STick > 122)    //   3s/16.384ms  factoryReset();
                {
                           I2C_write(ADDR_STRENGTH, 255);   //clear our flag
                        factoryReset();
                }
        }
        
        EnWatchdog();

        //gLedStatus = LED_NOW_ON;
        I2C_write(ADDR_ONOFF_FLAG, LED_NOW_ON);

        pwm_start();

        SlowChangeStrength(POWER_ON);
         
        if(T1DATA <=PWM_NUM_START_LOAD)
        {
                LoadCtl = 1;
        }
}



void changeLampStrength()
{
        if(gLedWave == LED_STRENGTH_UP)
        {
                CurCtl = 1;
                  if(gLedStrength != 255)
                        gLedStrength = gLedStrength + 1;
                else
                {
                        for(temp_char_3 =0; temp_char_3 <3; temp_char_3++)
                     {
                           T1DATA = 0;
                                delay_ms(1);
                               pwm_stop();
                                delay_ms(400);
                                   T1DATA = gLedStrength;
                                        delay_ms(1);
                                pwm_start();
                               delay_ms(400);
                     }
                     gLedWave = LED_STRENGTH_DN;
                }               
        }
        else
        {
                  if(gLedStrength != 8)
                 gLedStrength = gLedStrength - 1;
                else
                {
                     CurCtl=0;                
                     for(temp_char_3 =0; temp_char_3 <3; temp_char_3++)
                     {
                           T1DATA = 0;
                                delay_ms(1);
                               pwm_stop();
                                delay_ms(400);
                                    T1DATA = gLedStrength;
                                        delay_ms(1);
                                pwm_start();
                               delay_ms(400);
                     }
                   gLedWave = LED_STRENGTH_UP;
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
        if(P_KEY != 0)  //按键防抖
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
                                 {
                                                temp_char_2 = 1;
                                                return;
                                 }
                                temp_char_2 = 1;
                                temp_char_1 =0;
                                changeLampStrength();
                        }
                }
                else
                {
                        if(gLedStrength >150)
                        {
                                        if(temp_char_1 >= 2)  //10ms
                                        {
                                        temp_char_1 = 0;
                                        changeLampStrength();
                                }
                        }
                       
                        else
                      {
                                if(temp_char_1 >= 5)  //60ms
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
        DDR1 = 0x4B;   // 01001011

       PUCON = 0x00;    //0x7F;    //打开上拉 P17

         P1 = 0;

         CurCtl=1;

        //PWM
        T1CR   = 0xC0;
        T1LOAD = 254;
        T1DATA = 0;

        T0CR = 0x07;
        T0IE = 1;
        
        
        GIE = 1;                    //使能全局中断
}


void main()
{
        
        
        
        
        
        InitConfig();    //初始化配置
        DisWatchdog();

        gLedStatus = I2C_read(ADDR_ONOFF_FLAG);

        if(gLedStatus == LED_PRE_ON)
       {
                  I2C_write(ADDR_ONOFF_FLAG,LED_NOW_OFF);
                key_interrupt_enable();
                Stop();
                key_interrupt_disable();

                DisWatchdog();   //按键中断唤醒之后，系统会默认将RCEN置1
                g3STick = 0;
                while(P_KEY == 0) //wait key release
                {
                        if(g3STick > 122)    //   3s/16.384ms  factoryReset();
                        {
                                    I2C_write(ADDR_STRENGTH, 255);   //clear our flag
                                    factoryReset();
                        }

                }
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
                gLedStrength = 8;  //max level
                //delay_ms(5);
                I2C_write(ADDR_STRENGTH,8);
        }

        //冗错处理  
         if(gLedStrength < 8)
        {
                gLedStrength = LED_DEFAULT_LEVEL;
        }

         I2C_write(ADDR_ONOFF_FLAG,LED_NOW_ON);

         pwm_start();
        SlowChangeStrength(POWER_ON);
        
       while(1)
       {
                if(T1DATA <=PWM_NUM_START_LOAD)
                {
                  LoadCtl = 1;
                }    
                  if(T1DATA != gLedStrength)
                {
                        LampPowerOFF();
                        if(T1DATA == gLedStrength)
                                break;
                }
                else
                        break;
                ClrWdt();
        }

         //it's a dummy code. fuck compiler
        gLedStatus = LED_NOW_ON;

        while(1)
        {
                if(P_KEY == 0)   //key press
                {
                        delay_with_key_detect();

                        if(temp_char_2 == 0)   //short press
                         {
                                while(1)
                                {
                                        LampPowerOFF();
                                        if(T1DATA <=PWM_NUM_START_LOAD)
                                        {
                                                LoadCtl = 1;
                                        }
                                        if(T1DATA == gLedStrength)
                                                break;

                                            ClrWdt();
                                }
                        }
                        else if(gLampMode ==  ADJUST_MODE && temp_char_2 == 1) 
                        {       
                                // lamp strength has been changed, we need save
                                I2C_write(ADDR_STRENGTH,gLedStrength);                                          
                        }

                        temp_char_2 = 3;   //reset press flag
               }

                ClrWdt();
        }
     
}



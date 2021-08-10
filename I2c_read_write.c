#include <SI_EFM8BB2_Register_Enums.h>
#include "InitDevice.h"
#include <stdio.h>

#define uchar unsigned char

#define uint unsigned int 

//#define AddWr 0xa0 /*器件地址选择及写标志*/

//#define AddRd 0xa1 /*器件地址选择及读标志*/

//#define Hidden 0x0e /*显示器的消隐码*/ 

/*有关全局变量*/

//sbit SDA= P1^0; /*串行数据*/

//sbit SCL= P1^1; /*串行时钟*/ 

void delay_us(uint delay);

bit iI2c_Ack;          /*应答标志位*/
/*
                     起动总线函数               
函数原型: void  I2c_Start();  
功能:       启动I2C总线,即发送I2C起始条件.
  
*/
void delay_ms(uint mdelay)
   {while(mdelay>0)
   	 {
   	 	delay_us(1000);
   	 	mdelay--;
   	}
  }
void delay_us(uint delay)
  {
  	while(delay>0)
  	  {_nop_();    
       _nop_();
       _nop_();
       _nop_();
       _nop_(); 
       _nop_();    
       _nop_();
       _nop_();
       _nop_();
       _nop_(); 
       
       _nop_();    
       _nop_();
       _nop_();
       _nop_();
       _nop_(); 
       _nop_();    
       _nop_();
       _nop_();
       _nop_();
       _nop_(); 
       
       _nop_();    
       _nop_();
       _nop_();
       _nop_(); 
       delay--;
  	  }
  }
void I2c_Start(void)
{
   SDA=1;   /*发送起始条件的数据信号*/
   delay_us(10);
   SCL=1;
   delay_us(50);    /*起始条件建立时间大于4.7us,延时*/
   
   SDA=0;   /*发送起始信号*/
   delay_us(50);   /* 起始条件锁定时间大于4μs*/
        
   SCL=0;   /*钳住I2C总线，准备发送或接收数据 */
   delay_us(20);
}

/*
                      结束总线函数               
函数原型: void  I2c_Stop();  
功能:       结束I2C总线,即发送I2C结束条件.
  
*/

void I2c_Stop(void)
{
  SDA=0;  /*发送结束条件的数据信号*/
  delay_us(10);   /*发送结束条件的时钟信号*/
  SCL=1;  /*结束条件建立时间大于4μs*/
  delay_us(50);

  SDA=1;  /*发送I2C总线结束信号*/
  delay_us(40);
}

bit I2C_CheckAck(void) 
{ 
uchar errtime = 255;     // 因故障接收方无 Ack,超时值为255 

  SDA = 1;                  //发送器先释放SDA 
  delay_us(50); 
  
  SCL = 1; 
  delay_us(50);

   while(1)     //判断SDA是否被拉低
{ 
   errtime--;
  // delay_us(10); 
   if(errtime==0) 
   { 
	I2c_Stop();
    return(0); 
   } 
   if(SDA==0)
   	   break;
} 

  SCL = 0;  
  delay_us(10);

return(1); 

}
/*
                 字节数据传送函数               
函数原型: void  I2c_SendByte(uchar c);
功能:  将数据c发送出去,可以是地址,也可以是数据,发完后等待应答,并对
     此状态位进行操作.(不应答或非应答都使ack=0 假)     
     发送数据正常，ack=1; ack=0表示被控器无应答或损坏。
*/

void  I2c_SendByte(uchar c)
{
 uchar BitCnt;
 uchar temp=c; /*中间变量控制*/ 
 for(BitCnt=0;BitCnt<8;BitCnt++)  /*要传送的数据长度为8位*/
    {
    // if((temp&0x80)==0x80)  
    if((temp<<BitCnt)&0x80)   
     	 SDA=1;
     	else  
     	 SDA=0;                
      delay_us(10);
      SCL=1;               /*置时钟线为高，通知被控器开始接收数据位*/
      delay_us(50); 
              
      SCL=0; 
    // temp=temp<<1;
    }
    
     //delay_us(20);
   
    iI2c_Ack=I2C_CheckAck();//检验应答信号,作为发送方,所以要检测接收器反馈的应答信号.?_nop_();
    delay_us(20);
}

/*
                 字节数据传送函数               
函数原型: uchar  I2c_RcvByte();
功能:  用来接收从器件传来的数据,并判断总线错误(不发应答信号)，
     发完后请用应答函数。  
*/ 

uchar  I2c_RcvByte()
{
  uchar retc;
  uchar BitCnt;
  
  retc=0; 
  SDA=1;             /*置数据线为输入方式*/
  for(BitCnt=0;BitCnt<8;BitCnt++)
      {
        delay_us(10);           
        SCL=0;       /*置时钟线为低，准备接收数据位*/
        delay_us(50);
        
        SCL=1;       /*置时钟线为高使数据线上数据有效*/
        delay_us(20);
         
        retc=retc<<1;
        if(SDA==1)retc=retc+1; /*读数据位,接收的数据位放入retc中 */
        delay_us(20);  
      }
        SCL=0;    
        delay_us(20);
         
  return(retc);
}

/*
                     应答子函数
原型:  void I2c_Ack(bit a);
 
功能:主控器进行应答信号,(可以是应答或非应答信号)
*/

void I2c_Ack(bit a)
{
  
  if(a==0)
  	SDA=0;     /*在此发出应答或非应答信号 */
  else 
  	SDA=1;
    delay_us(30);  
         
    SCL=1;
    delay_us(50); 
      
    SCL=0;                /*清时钟线，钳住I2C总线以便继续接收*/
    delay_us(20);    
}

/*
                    向无子地址器件发送字节数据函数               
函数原型: bit  I2C_WriteRegister(uchar sla,uchar reg,ucahr c);  
功能:     从启动总线到发送地址，数据，结束总线的全过程,从器件地址sla.寄存器地址reg，寄存器的值C
           如果返回1表示操作成功，否则操作有误。
注意：    使用前必须已结束总线。
*/
bit  I2C_WriteRegister(uchar sla,uchar reg,uchar c)
{
	
	 I2c_Start();               /*启动总线*/
   I2c_SendByte(sla);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);  	
   I2c_SendByte(c);               /*发送数据*/
     if(iI2c_Ack==0)return(0);
     I2c_Stop();                 /*结束总线*/
 
  return(1);
  
}

bit  I2C_WriteRegistermult(uchar sla,uchar reg,uchar NumByteToWrite,uchar *c)
{
	
	 I2c_Start();               /*启动总线*/
   I2c_SendByte(sla);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);  	
      if(NumByteToWrite)
  {
    while(NumByteToWrite--)
    {															
   I2c_SendByte(*c++);               /*发送数据*/
     if(iI2c_Ack==0)return(0);
    }
  }
     I2c_Stop();                 /*结束总线*/
 
  return(1);
  
}
/*
                    向无子地址器件读字节数据函数               
函数原型: uchar  I2C_ReadRegister(uchar sla,uchar reg)  
功能:     从启动总线到发送地址，寄存器地址,读寄存器数据，结束总线的全过程,从器件地
          址sla，返回值在c.
           如果返回1表示操作成功，否则操作有误。
注意：    使用前必须已结束总线。
*/
uchar  I2C_ReadRegistermult(uchar sla,uchar reg,uchar NumByteToRead,uchar *c)
{
	uchar utemp=0;
	I2c_Start();                /*启动总线*/
	I2c_SendByte(sla);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);  	   	
   I2c_Start();                /*启动总线*/  	
   I2c_SendByte(sla+1);           /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
     	 if (NumByteToRead > 2) 
     	 	{
     	 		while(NumByteToRead > 3)       			// not last three bytes?
             {
			       *c++ = I2c_RcvByte(); 
			       I2c_Ack(0);                   				// Reading next data byte
            --NumByteToRead;																		// Decrease Numbyte to reade by 1
           } 
   *c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(0);               /*发送非就答位*/
   *c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(0);               /*发送非就答位*/
   *c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(1);               /*发送非就答位*/    
     I2c_Stop();                  /*结束总线*/
   }
   else  if(NumByteToRead == 2)  
   	{*c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(0);               /*发送非就答位*/
   *c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(1);               /*发送非就答位*/    
     I2c_Stop();  
   	}
   	else  if(NumByteToRead == 1)  
   	{
   *c++=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(1);               /*发送非就答位*/    
     I2c_Stop();  
   	}
  return(utemp);
  
}

uchar  I2C_ReadRegister(uchar sla,uchar reg)
{
	uchar utemp=0;
	I2c_Start();                /*启动总线*/
	I2c_SendByte(sla);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*发送器件地址*/
     if(iI2c_Ack==0)return(0);  	   	
   I2c_Start();                /*启动总线*/  	
   I2c_SendByte(sla+1);           /*发送器件地址*/
     if(iI2c_Ack==0)return(0);
   utemp=I2c_RcvByte();               /*读取数据*/
     I2c_Ack(1);               /*发送非就答位*/
     I2c_Stop();                  /*结束总线*/
  return(utemp);
  
}
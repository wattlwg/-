#include <SI_EFM8BB2_Register_Enums.h>
#include "InitDevice.h"
#include <stdio.h>

#define uchar unsigned char

#define uint unsigned int 

//#define AddWr 0xa0 /*������ַѡ��д��־*/

//#define AddRd 0xa1 /*������ַѡ�񼰶���־*/

//#define Hidden 0x0e /*��ʾ����������*/ 

/*�й�ȫ�ֱ���*/

//sbit SDA= P1^0; /*��������*/

//sbit SCL= P1^1; /*����ʱ��*/ 

void delay_us(uint delay);

bit iI2c_Ack;          /*Ӧ���־λ*/
/*
                     �����ߺ���               
����ԭ��: void  I2c_Start();  
����:       ����I2C����,������I2C��ʼ����.
  
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
   SDA=1;   /*������ʼ�����������ź�*/
   delay_us(10);
   SCL=1;
   delay_us(50);    /*��ʼ��������ʱ�����4.7us,��ʱ*/
   
   SDA=0;   /*������ʼ�ź�*/
   delay_us(50);   /* ��ʼ��������ʱ�����4��s*/
        
   SCL=0;   /*ǯסI2C���ߣ�׼�����ͻ�������� */
   delay_us(20);
}

/*
                      �������ߺ���               
����ԭ��: void  I2c_Stop();  
����:       ����I2C����,������I2C��������.
  
*/

void I2c_Stop(void)
{
  SDA=0;  /*���ͽ��������������ź�*/
  delay_us(10);   /*���ͽ���������ʱ���ź�*/
  SCL=1;  /*������������ʱ�����4��s*/
  delay_us(50);

  SDA=1;  /*����I2C���߽����ź�*/
  delay_us(40);
}

bit I2C_CheckAck(void) 
{ 
uchar errtime = 255;     // ����Ͻ��շ��� Ack,��ʱֵΪ255 

  SDA = 1;                  //���������ͷ�SDA 
  delay_us(50); 
  
  SCL = 1; 
  delay_us(50);

   while(1)     //�ж�SDA�Ƿ�����
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
                 �ֽ����ݴ��ͺ���               
����ԭ��: void  I2c_SendByte(uchar c);
����:  ������c���ͳ�ȥ,�����ǵ�ַ,Ҳ����������,�����ȴ�Ӧ��,����
     ��״̬λ���в���.(��Ӧ����Ӧ��ʹack=0 ��)     
     ��������������ack=1; ack=0��ʾ��������Ӧ����𻵡�
*/

void  I2c_SendByte(uchar c)
{
 uchar BitCnt;
 uchar temp=c; /*�м��������*/ 
 for(BitCnt=0;BitCnt<8;BitCnt++)  /*Ҫ���͵����ݳ���Ϊ8λ*/
    {
    // if((temp&0x80)==0x80)  
    if((temp<<BitCnt)&0x80)   
     	 SDA=1;
     	else  
     	 SDA=0;                
      delay_us(10);
      SCL=1;               /*��ʱ����Ϊ�ߣ�֪ͨ��������ʼ��������λ*/
      delay_us(50); 
              
      SCL=0; 
    // temp=temp<<1;
    }
    
     //delay_us(20);
   
    iI2c_Ack=I2C_CheckAck();//����Ӧ���ź�,��Ϊ���ͷ�,����Ҫ��������������Ӧ���ź�.?_nop_();
    delay_us(20);
}

/*
                 �ֽ����ݴ��ͺ���               
����ԭ��: uchar  I2c_RcvByte();
����:  �������մ���������������,���ж����ߴ���(����Ӧ���ź�)��
     ���������Ӧ������  
*/ 

uchar  I2c_RcvByte()
{
  uchar retc;
  uchar BitCnt;
  
  retc=0; 
  SDA=1;             /*��������Ϊ���뷽ʽ*/
  for(BitCnt=0;BitCnt<8;BitCnt++)
      {
        delay_us(10);           
        SCL=0;       /*��ʱ����Ϊ�ͣ�׼����������λ*/
        delay_us(50);
        
        SCL=1;       /*��ʱ����Ϊ��ʹ��������������Ч*/
        delay_us(20);
         
        retc=retc<<1;
        if(SDA==1)retc=retc+1; /*������λ,���յ�����λ����retc�� */
        delay_us(20);  
      }
        SCL=0;    
        delay_us(20);
         
  return(retc);
}

/*
                     Ӧ���Ӻ���
ԭ��:  void I2c_Ack(bit a);
 
����:����������Ӧ���ź�,(������Ӧ����Ӧ���ź�)
*/

void I2c_Ack(bit a)
{
  
  if(a==0)
  	SDA=0;     /*�ڴ˷���Ӧ����Ӧ���ź� */
  else 
  	SDA=1;
    delay_us(30);  
         
    SCL=1;
    delay_us(50); 
      
    SCL=0;                /*��ʱ���ߣ�ǯסI2C�����Ա��������*/
    delay_us(20);    
}

/*
                    �����ӵ�ַ���������ֽ����ݺ���               
����ԭ��: bit  I2C_WriteRegister(uchar sla,uchar reg,ucahr c);  
����:     ���������ߵ����͵�ַ�����ݣ��������ߵ�ȫ����,��������ַsla.�Ĵ�����ַreg���Ĵ�����ֵC
           �������1��ʾ�����ɹ��������������
ע�⣺    ʹ��ǰ�����ѽ������ߡ�
*/
bit  I2C_WriteRegister(uchar sla,uchar reg,uchar c)
{
	
	 I2c_Start();               /*��������*/
   I2c_SendByte(sla);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);  	
   I2c_SendByte(c);               /*��������*/
     if(iI2c_Ack==0)return(0);
     I2c_Stop();                 /*��������*/
 
  return(1);
  
}

bit  I2C_WriteRegistermult(uchar sla,uchar reg,uchar NumByteToWrite,uchar *c)
{
	
	 I2c_Start();               /*��������*/
   I2c_SendByte(sla);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);  	
      if(NumByteToWrite)
  {
    while(NumByteToWrite--)
    {															
   I2c_SendByte(*c++);               /*��������*/
     if(iI2c_Ack==0)return(0);
    }
  }
     I2c_Stop();                 /*��������*/
 
  return(1);
  
}
/*
                    �����ӵ�ַ�������ֽ����ݺ���               
����ԭ��: uchar  I2C_ReadRegister(uchar sla,uchar reg)  
����:     ���������ߵ����͵�ַ���Ĵ�����ַ,���Ĵ������ݣ��������ߵ�ȫ����,��������
          ַsla������ֵ��c.
           �������1��ʾ�����ɹ��������������
ע�⣺    ʹ��ǰ�����ѽ������ߡ�
*/
uchar  I2C_ReadRegistermult(uchar sla,uchar reg,uchar NumByteToRead,uchar *c)
{
	uchar utemp=0;
	I2c_Start();                /*��������*/
	I2c_SendByte(sla);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);  	   	
   I2c_Start();                /*��������*/  	
   I2c_SendByte(sla+1);           /*����������ַ*/
     if(iI2c_Ack==0)return(0);
     	 if (NumByteToRead > 2) 
     	 	{
     	 		while(NumByteToRead > 3)       			// not last three bytes?
             {
			       *c++ = I2c_RcvByte(); 
			       I2c_Ack(0);                   				// Reading next data byte
            --NumByteToRead;																		// Decrease Numbyte to reade by 1
           } 
   *c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(0);               /*���ͷǾʹ�λ*/
   *c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(0);               /*���ͷǾʹ�λ*/
   *c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(1);               /*���ͷǾʹ�λ*/    
     I2c_Stop();                  /*��������*/
   }
   else  if(NumByteToRead == 2)  
   	{*c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(0);               /*���ͷǾʹ�λ*/
   *c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(1);               /*���ͷǾʹ�λ*/    
     I2c_Stop();  
   	}
   	else  if(NumByteToRead == 1)  
   	{
   *c++=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(1);               /*���ͷǾʹ�λ*/    
     I2c_Stop();  
   	}
  return(utemp);
  
}

uchar  I2C_ReadRegister(uchar sla,uchar reg)
{
	uchar utemp=0;
	I2c_Start();                /*��������*/
	I2c_SendByte(sla);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);
   I2c_SendByte(reg);            /*����������ַ*/
     if(iI2c_Ack==0)return(0);  	   	
   I2c_Start();                /*��������*/  	
   I2c_SendByte(sla+1);           /*����������ַ*/
     if(iI2c_Ack==0)return(0);
   utemp=I2c_RcvByte();               /*��ȡ����*/
     I2c_Ack(1);               /*���ͷǾʹ�λ*/
     I2c_Stop();                  /*��������*/
  return(utemp);
  
}
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "lcd.h" 
#include "sram.h"   
#include "malloc.h" 
#include "usmart.h"     
#include "malloc.h"     	
#include "string.h"	
#include "math.h"	
#include "dcmi.h"	
#include "ov2640.h"	
#include "timer.h"
#include "iwdg.h"

///////////////////////////////////////////////////////////////////////////////////
//img_proc
/* Include Files */
#include "ConnectedDomain.h"

///////////////////////////////////////////////////////////////////////////////////
/* Function Declarations */
void main_lightspot(unsigned char* uv0, double* pos);
void buf2im(u16* buf,unsigned char dst[320*240]);
void DrawLine(double* pos, unsigned char frame[320*240]);
void im2buf(u16* buf,unsigned char frame[320*240]);
void imDilate(u8* binmap);

int main(void)
{   
	u8* binmap = NULL;
	u16* im = NULL;
	double pos[6] ;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  			//初始化延时函数
	uart_init(115200);			//初始化串口波特率为115200
	LED_Init();					//初始化LED 
	usmart_dev.init(84);		//初始化USMART
	TIM3_Int_Init(10000-1,8400-1);//10Khz计数,1秒钟中断一次
 	LCD_Init();					//LCD初始化  
	FSMC_SRAM_Init();			//初始化外部SRAM. 
	my_mem_init(SRAMIN);		//初始化内部内存池 
	my_mem_init(SRAMEX);		//初始化内部内存池  
	my_mem_init(SRAMCCM);		//初始化CCM内存池 
	//IWDG_Init(5,2000); //与分频数为64,重载值为500,溢出时间为8s	
	
	printf("mallocing\r\n");
	im = mymalloc(SRAMEX, 320*240*sizeof(u16)*2);
	binmap = mymalloc(SRAMEX, 320*240 * sizeof(u8));
	
	while(!im);
	printf("databuf\r\n");
	while(!binmap);
	printf("imgbuf\r\n");
	
	while(OV2640_Init());	
	OV2640_RGB565_Mode();	//RGB565模式
	My_DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)im,320 * 240 *sizeof(u16)*2,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Enable);//DCMI DMA配置  
 	OV2640_OutSize_Set(240,320); 	
 	IWDG_Feed();
	
	DCMI_Start(); 			//启动传输 	
	while(1)
	{	
		DCMI_Stop(); //停止显示 	
//		for(x=0; x<240; x++){
//			for(y=0; y<320; y++){
//				LCD_Fast_DrawPoint(x, y, im[x+240*y]);
//			}
//		}
		/* Invoke the entry-point functions.
		You can call entry-point functions multiple times. */
		buf2im(im, binmap);
		main_lightspot(binmap, pos);
		im2buf(im, binmap);
		delay_ms(100);
		IWDG_Feed();
		DCMI_Start(); 	//开始显示
		LED0=!LED0;
	}
}

/* Function Definitions */
void main_lightspot(unsigned char* uv0, double* pos)
{
	u32 sumX[3] = {0};
    u32 sumY[3] = {0};
	u32 num[3]= {0};
	u32 x, y;
	u16* label = mymalloc(SRAMEX, 320*240*2*sizeof(u16));
	//连通域标定
	bwLabel(uv0, label, 320, 240);	
	//质心
	for(x=0; x<240; x++){
		for(y=0; y < 320; y++){
			if(1<=label[x+y*240] && label[x+y*240] <= 3){
				uv0[x+y*240] = 255;				
				sumX[label[x+y*240]-1] += x;
				sumY[label[x+y*240]-1] += y;
				num[label[x+y*240]-1]++;
			}else{
				uv0[x+y*240] = 0;
			}
		}
	}

	printf("Area %d %d %d\r\n", num[0], num[1], num[2]);
	printf("SumX %d %d %d\r\n", sumX[0], sumX[1], sumX[2]);
	printf("SumY %d %d %d\r\n", sumY[0], sumY[1], sumY[2]);
	for(x = 0; x < 3; x++){
		pos[x*2] = (double)(sumX[x*2]*1.0/num[x]);
		pos[x*2+1] = (double)(sumY[x*2 + 1]*1.0/num[x]);		
		printf("%d, %d\r\n",sumX[x*2]/num[x], sumY[x*2]/num[x]);		
		POINT_COLOR = 120;
		LCD_Draw_Circle(sumX[x*2]/num[x], sumY[x*2]/num[x],5);						 			//画圆
	}
	printf("\r\n");
	myfree(SRAMEX, label);
}

void buf2im(u16* buf,unsigned char dst[320*240]){
  int idx0;
  int idx1;
  u8  temp[3];
  u16  color; 

  /* Loop over the array to initialize each element. */
  for (idx0 = 0; idx0 < 240; idx0++) {
    for (idx1 = 0; idx1 < 320; idx1++) {
	  color = buf[idx0 + 240*idx1];
	  temp[0] = ((color&0xF800) >> 8);//R
	  temp[1] = ((color&0x07E0) >> 3);//G
	  temp[2] = ((color&0x001F) << 3);//B
	  color = (temp[0]*30 + temp[1]*59 + temp[2]*11 + 50) / 100;
      if(color > 60){
		  dst[(idx0 + 240 * idx1)] = 255;
	  }
	  else{
		  dst[(idx0 + 240 * idx1)] = 0;	
	  }		
	}
  }
}

void im2buf(u16* buf,unsigned char frame[320*240]){
  int idx0;
  int idx1;
  u16 temp;
  /* Loop over the array to initialize each element. */
  for (idx0 = 0; idx0 < 240; idx0++) {
    for (idx1 = 0; idx1 < 320; idx1++) {
        temp =        ((frame[(idx0 + 240 * idx1)] & 0xf8)>>3);//B
		temp = temp | ((frame[(idx0 + 240 * idx1)] & 0xfc)<<3);//G
		temp = temp | ((frame[(idx0 + 240 * idx1)] & 0xf8)<<8);//R
		LCD_Fast_DrawPoint(idx0,idx1 ,temp);								//快速画点
    }
  }
}	

void imDilate(u8* binmap){
	u16 i,j;
	
	for(i = 0; i < 240; i++){
		for(j=0; j < 320; j++){
			if(i==0 || i == 239 || j == 0 || j==319){
				binmap[i+240*j] = 0;
			}
			else if(binmap[i+240*j] || binmap[i+240*(j-1)] || binmap[i+240*(j+1)]
					|| binmap[(i+1)+240*j] || binmap[(i-1)+240*j]){
				binmap[i+240*j] = 255;
			}else{
				binmap[i+240*j] = 0;
			}
		}
	}
}

/*
(C)2015 NPLink

Description: app task implementation，本APP例程可实现LORAMAC、PHYMAC、低功耗3个应用之间的切换
						用户应该根据需要进行逻辑修改
License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Robxr
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"
#include "stm32l0xx_hal_rtc.h"
#include <string.h>
#include <stdio.h>

#include "osal_memory.h"
#include "osal.h"
#include "oled_board.h"
#include "app_osal.h"
#include "loraMac_osal.h"
#include "LoRaMacUsr.h"

#include "radio.h"
#include "timer.h"
#include "uart_board.h"
#include "led_board.h"
#include "rtc_board.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u8 APP_taskID;
char Rx_buf[64]; //buffer for oled display
u32 Rxpacket_count = 0 ;
u8* RecieveBuff_flag = NULL;
__IO ITStatus UartReady = RESET;
u8 aTxBuffer[] = "uart test, hello!\n";
u8 aRxBuffer[RXBUFFERSIZE];
u8 g_number = 0;

LoRaMacAppPara_t g_appData;//定义APP 参数结构体
LoRaMacMacPara_t g_macData;//定义MAC参数结构体

u8 send_num = 10;
uint8_t txuartdataflag ;
u8 uucount = 5;
u8 debugEnable = FALSE;

/* variables -----------------------------------------------------------*/
extern UART_HandleTypeDef UartHandle;
extern u8 send_num ;
extern u8 APP_taskID;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void APP_Init(u8 task_id)
{
	APP_taskID = task_id;

	//开启或关闭MAC层的串口打印信息
	#ifdef USE_DEBUG
	debugEnable = TRUE;
	#endif

  //显示Mote ID
	LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_ADDR);
	APP_ShowMoteID(g_appData.devAddr);

	memset( &g_macData , 0 , sizeof(g_macData) );

#if 1
  //设置LORAMAC工作模式的参数(LoRa调制)
	//设置信道1
	g_macData.channels[0].Frequency = 779500000;//频点
	g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //速率范围:((最高速率<<4) | (最低速率))
	g_macData.channels[0].Band = 0;
	//设置信道2
	g_macData.channels[1].Frequency = 779700000;
	g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
	g_macData.channels[1].Band = 0;
	//设置信道3
	g_macData.channels[2].Frequency = 779900000;
	g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
	g_macData.channels[2].Band = 0;
	//ADR开启或关闭
	g_macData.lora_mac_adr_switch = TRUE;
	//发送速率
	g_macData.datarate = DR_5;
	LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE);
	//设置使用LoRaMAC
	LoRaMac_setMode(MODE_LORAMAC);
#endif

#if 0
		//设置LORAMAC工作模式的参数(FSK调制)
		//设置信道1
		g_macData.channels[0].Frequency = 779700000;//频点
		g_macData.channels[0].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 ); //速率范围:((最高速率<<4) | (最低速率))
		g_macData.channels[0].Band = 0;
		//设置信道2
		g_macData.channels[1].Frequency = 779700000;
		g_macData.channels[1].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 );
		g_macData.channels[1].Band = 0;
		//设置信道3
		g_macData.channels[2].Frequency = 779700000;
		g_macData.channels[2].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 );
		g_macData.channels[2].Band = 0;
		//发送速率
		g_macData.datarate = DR_7;
		//ADR开启或关闭
		g_macData.lora_mac_adr_switch = FALSE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE);
		//设置使用LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
#endif

#if 0
	//设置PHYMAC工作模式的参数(LoRa调制方式)
	g_macData.phyFrequency = 779700000;//频率(Hz)
	g_macData.phySF = 7; //扩频因子(7-12)
	g_macData.phymodulation = MODULATION_LORA;//调制方式(FSK or LORA)
	LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_PHY_FREQUENCY | PARAMETER_PHY_SPREADING_FACTOR | PARAMETER_PHY_MODULATION_MODE );
	//设置使用PhyMAC
	LoRaMac_setMode(MODE_PHY);
#endif

#if 0
		//设置PHYMAC工作模式的参数(FSK调制方式)
		g_macData.phyFrequency = 779700000;//频率(Hz)
		g_macData.phymodulation = MODULATION_FSK;//调制方式(FSK or LORA)
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_PHY_FREQUENCY | PARAMETER_PHY_MODULATION_MODE );
		//设置使用PhyMAC
		LoRaMac_setMode(MODE_PHY);
#endif


 	osal_set_event(APP_taskID,APP_PERIOD_SEND);//启动发包
}

u16 APP_ProcessEvent( u8 task_id, u16 events )
{
 loraMAC_msg_t* pMsgSend = NULL;
 loraMAC_msg_t* pMsgRecieve = NULL;

 u8 len = 0 ;

  //system event
  if(events & SYS_EVENT_MSG)
  {
		//receive msg loop
		while(NULL != (pMsgRecieve = (loraMAC_msg_t*)osal_msg_receive(APP_taskID)))
		{
		//pMsgRecieve[0] is system event type
		switch(pMsgRecieve->msgID)
		{
		//tx done
		case TXDONE :
		case TXERR_STATUS:

				HalLedSet (HAL_LED_1, HAL_LED_MODE_ON);
				if(send_num > 0)//通过LORA MAC模式发包
				{
					send_num--;
					//send a packet to LoRaMac osal (then can be send by the radio)
					pMsgSend = (loraMAC_msg_t*)osal_msg_allocate(sizeof(loraMAC_msg_t));
					if(pMsgSend != NULL)
					{
						osal_memset(pMsgSend,0,sizeof(loraMAC_msg_t));
						pMsgSend->msgID = TXREQUEST;
						pMsgSend->msgLen = 70;
						for(u8 dataCount = 0; dataCount < 70; dataCount++)
						{
							pMsgSend->msgData[dataCount] = dataCount;
						}
						osal_msg_send(LoraMAC_taskID,(u8*)pMsgSend);
						osal_msg_deallocate((u8*)pMsgSend);

						#ifdef USE_DEBUG
						HAL_UART_SendBytes("app send start...\n", osal_strlen("app send start...\n"));
						#endif
					}
				}
				else
				{

					#if  0  //PHYMAC 模式发包

					if(mode != MODE_PHY)    //通过PHY模式发包
					{
						LoRaMac_setMode(MODE_PHY);
						uucount = 5;
					}
					else
					{
						uucount--;
						if(uucount == 0)
						{
							send_num = 10;
							LoRaMac_setMode(0);
							osal_set_event(APP_taskID,0x0001);
							//Radio.Sleep();
						}
					}

					#else//低功耗测试

					#ifdef USE_LOW_POWER_MODE
					RtcSetTimeout(20000000);
					LoRaMac_setlowPowerMode(TRUE);
					RtcEnterLowPowerStopMode();
					#endif
					LoRaMac_setMode(MODE_LORAMAC);

					send_num = 10;
					osal_set_event(APP_taskID,APP_PERIOD_SEND);
					Radio.Sleep();

					#endif
				}

				HalLedSet (HAL_LED_1, HAL_LED_MODE_OFF);

				break;

				//rx done
			case RXDONE:

				HalLedSet (HAL_LED_2, HAL_LED_MODE_ON);
				OLED_Clear_Half();//先把屏幕下一半清空
				APP_ShowMoteID(g_appData.devAddr);
				len = 0 ;
				g_number++ ;
				memset(Rx_buf , 0 ,sizeof(Rx_buf));
				osal_memcpy(Rx_buf,pMsgRecieve->msgData,pMsgRecieve->msgLen);
				len = pMsgRecieve->msgLen;
				Rx_buf[len] = 0;
				OLED_ShowString( 0,36, (u8*)Rx_buf,12 );
				OLED_Refresh_Gram();
				#ifdef USE_DEBUG
				HAL_UART_SendBytes("\n",1);
				HAL_UART_SendBytes((uint8_t *)Rx_buf,strlen(Rx_buf));
				#endif
				HalLedSet (HAL_LED_2, HAL_LED_MODE_OFF);

				break;
				/*
				//发送失败
             case TXERR_STATUS:
             {
                //TODO MOTE send packet error deal
                memset( tmp_buf ,0 ,sizeof(tmp_buf) );
                sprintf( (char *)tmp_buf,"send err ret=%d,no=%d",pMsgRecieve->msgData[0],
                                                                 pMsgRecieve->msgData[1]+( pMsgRecieve->msgData[2]<<8 ) );
                OLED_ShowString( 0,36,tmp_buf,12 );
                OLED_Refresh_Gram();
                break;
             }
				*/

        default:
			    break;
			}

			osal_msg_deallocate((u8*)pMsgRecieve);
		}
		return (events ^ SYS_EVENT_MSG);

	}

	//send a packet event
	if(events & APP_PERIOD_SEND)
	{
		//RedLED(OFF);
		 HalLedSet (HAL_LED_1, HAL_LED_MODE_OFF);
		//send a packet to LoRaMac osal (then can be send by the radio)
		pMsgSend = (loraMAC_msg_t*)osal_msg_allocate(sizeof(loraMAC_msg_t));
		if(pMsgSend != NULL)
		{
			osal_memset(pMsgSend,0,sizeof(loraMAC_msg_t));
			pMsgSend->msgID = TXREQUEST;
			pMsgSend->msgLen = 70;
			for(u8 dataCount = 0; dataCount < 70; dataCount++)
			{
				pMsgSend->msgData[dataCount] = dataCount;
			}
				osal_msg_send(LoraMAC_taskID,(u8*)pMsgSend);
		}

		#ifdef USE_DEBUG
		HAL_UART_SendBytes("app send start...\n", osal_strlen("app send start...\n"));
		#endif
	  //osal_start_timerEx(APP_taskID, APP_PERIOD_SEND,1000);//延时继续发送
		return (events ^ APP_PERIOD_SEND);
	}

	return 0 ;
}

//display NPLink mote ID on the OLED
void APP_ShowMoteID( u32 moteID )
{
	u8 	MoteIDString[32] ;
	u8* pIDString = MoteIDString;
	u32 ZeroNum = 0 ;

	//count the zero num in front of moteID string
	for(u8 i = 28; i > 0; i = i - 4)
	{
		if((moteID >> i ) % 16 == 0)
		{
			ZeroNum = ZeroNum + 1 ;
		}
		else
		{
			break;
		}
	}

	sprintf((char*)pIDString,"ID:");
	pIDString += 3;
	while(ZeroNum--)
	{
		sprintf((char*)pIDString,"0");
		pIDString++;
	}
	sprintf((char*)pIDString,"%x",moteID);

	OLED_ShowString( 0,0,MoteIDString,12 );
	OLED_Refresh_Gram();
}

u16 Onboard_rand(void)
{
	return 0; //return TIM_GetCounter(TIM5);
}

/******************* (C) COPYRIGHT 2015 NPLink *****END OF FILE****/


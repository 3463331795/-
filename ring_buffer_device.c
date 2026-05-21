
#include "ring_buffer_device.h"

stRingBufferDeviceParamTdf astRingBufferDeviceParam[emRingBufferDevNum];		// 编号 0 - 7

/// @brief    获取 RING_BUFFER 设备参数
///
/// @param    emDevNum   ：设备号
///
/// @note     注意，返回值是 stRingBufferDeviceParamTdf 型的指针，且指针指向的内容是不可更改的（只读的）
const stRingBufferDeviceParamTdf *c_pstGetRingBufferDeviceParam(emRingBufferDevNumTdf emDevNum)
{
	return &astRingBufferDeviceParam[emDevNum];
}

/// @brief		拷贝运行参数
///
/// @param		emDevNum		：设备号
///
/// @note
void vRingBufferDeviceRunningParamInit(stRingBufferRunningParamTdf *pstInit, emRingBufferDevNumTdf emDevNum)
{
	memcpy(&astRingBufferDeviceParam[emDevNum].stRunningParam, pstInit, sizeof(stRingBufferRunningParamTdf));
}

 
/// @brief		  写入单个元素
///
/// @param		  pvElement		：要写入元素的首地址
///							emDevNum		：设备号

///	@retval			错误码，详见 emRingBufferErrorCodeTdf 定义
/// @note
emRingBufferErrorCodeTdf vRingBufferWriteSingleElement(void * pvElement, emRingBufferDevNumTdf emDevNum)
{
	uint32_t i;
	
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus == emRingBufferStatus_FULL)
	{
		return emRingBufferError_WriteFull;
	}
	
	// 1. 按Byte 将数据写入缓冲区
	for(i = 0; i < astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength; i++)
  {
		*((uint8_t *)(astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite) + i) = *((uint8_t *)(pvElement) + i);
	}
	
	// 2. 更新写指针
	// 2.1. 写指针自增
	astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite = (void *)((uint8_t *)astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite +
																																								astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength);
	
	// 2.2 写指针大于尾指针，则将写指针更新为头指针
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite > astRingBufferDeviceParam[emDevNum].stStaticParam.pvTail)
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite = astRingBufferDeviceParam[emDevNum].stStaticParam.pvHead;
	
	}
	
	// 3. 更新状态
	// 3.1 更新写指针后，写指针和读指针相等，则说明缓冲区已满了
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite == astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead)
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus = emRingBufferStatus_FULL;
	}
	// 3.2 缓冲区正常
	else
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus = emRingBufferStatus_NotEmptyNotFull;
	}
	return emRingBufferError_None;
}

/// @brief		  读取单个元素
///
/// @param		  pvElement		：要读取元素的首地址
///							emDevNum		：设备号

///	@retval			错误码，详见 emRingBufferErrorCodeTdf 定义
/// @note
emRingBufferErrorCodeTdf vRingBufferReadSingleElement(void * pvElement, emRingBufferDevNumTdf emDevNum)
{
	uint32_t i;
	
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus == emRingBufferStatus_Empty)
	{
		return emRingBufferError_ReadEmpty;
	}
	
	// 1. 按Byte 将数据从缓冲区读取出来
	for(i = 0; i < astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength; i++)
  {
	  *((uint8_t *)(pvElement) + i) =	*((uint8_t *)(astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead) + i) ;
	}
	
	// 2. 更新读指针
	// 2.1. 读指针自增
	astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead = (void *)((uint8_t *)astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead +
																																								astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength);
	
	// 2.2 读指针大于尾指针，则将读指针更新为头指针
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead > astRingBufferDeviceParam[emDevNum].stStaticParam.pvTail)
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead = astRingBufferDeviceParam[emDevNum].stStaticParam.pvHead;
	
	}
	
	// 3. 更新状态
	// 3.1 更新读指针后，写指针和读指针相等，则说明缓冲区已空
	if(astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite == astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead)
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus = emRingBufferStatus_Empty;
	}
	// 3.2 缓冲区正常
	else
	{
		astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus = emRingBufferStatus_NotEmptyNotFull;
	}
	return emRingBufferError_None;
}


/// @brief		  查找单个元素
///
/// @param		  c_pvTargetElement		：要匹配的目标元素首地址
///							emDevNum						：设备号

///	@retval			元素首次出现的位置，如果没有符合要求的元素，则此指针指向NULL
/// @note
void *pvRingBufferFindElementFirstPosition(void *c_pvTargetElement, emRingBufferDevNumTdf emDevNum)
{
	void *p = astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead;		// 暂存 pvRead 指针
	
	// 1、从pvRead 开始查找，直到 pvWrite 结束，逐个查找是否有符合要求的元素
	while(p != astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite)
	{
		//	1.1 有匹配元素，则直接返回其地址
		if(memcmp(p, c_pvTargetElement, astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength) == 0)
		{
			return p;
		}
		
		p = (void *)((uint8_t *)p + astRingBufferDeviceParam[emDevNum].stStaticParam.ulElementLength);
		if(p > astRingBufferDeviceParam[emDevNum].stStaticParam.pvTail)
		{
			p = astRingBufferDeviceParam[emDevNum].stStaticParam.pvHead;
		}
	}
	
	return NULL;
}


/// @brief     RING_BUFFER 设备初始化
///
/// @param     pstInit     ：初始化参数结构体的首地址
/// @param     emDevNum    ：设备编号
/// @note
void vRingBufferDeviceInit(stRingBufferStaticParamTdf *pstInit, uint8_t emDevNum)
{
	// 1. 初始化静态参数
	memcpy(&astRingBufferDeviceParam[emDevNum].stStaticParam, pstInit, sizeof(stRingBufferStaticParamTdf));

	//  2. 初始化运行参数	
	astRingBufferDeviceParam[emDevNum].stRunningParam.emStatus = emRingBufferStatus_Empty;
	astRingBufferDeviceParam[emDevNum].stRunningParam.pvRead	 = astRingBufferDeviceParam[emDevNum].stStaticParam.pvHead;
	astRingBufferDeviceParam[emDevNum].stRunningParam.pvWrite	 = astRingBufferDeviceParam[emDevNum].stStaticParam.pvHead;	
}

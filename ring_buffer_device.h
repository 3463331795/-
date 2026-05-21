
#ifndef __RING_BUFFER_DEVICE_H__
#define __RING_BUFFER_DEVICE_H__

#include "stm32f4xx_hal.h"
#include "string.h"

/// @brief		设备号枚举
///
/// @note
typedef enum
{
	emRingBufferDevNum0 	=0,
	emRingBufferDevNum1,
	emRingBufferDevNum2,
	emRingBufferDevNum3,
	emRingBufferDevNum4,
	emRingBufferDevNum5,
	emRingBufferDevNum6,
	emRingBufferDevNum7,
	emRingBufferDevNum
}
emRingBufferDevNumTdf;


/// @brief		状态定义
///
/// @note
typedef enum
{
	emRingBufferStatus_Empty								=	0,		// 0 空
	emRingBufferStatus_FULL									=	1,		// 1 满
	emRingBufferStatus_NotEmptyNotFull			=	2,		// 2 非空非满
}
emRingBufferStatusTdf;


/// @brief		错误编码定义
///
/// @note
typedef enum
{
	emRingBufferError_None									 = 0,		// 0 无错误
	emRingBufferError_WriteFull							 = 1,		// 1 写入满，即缓冲区已满，写入失败
	emRingBufferError_ReadEmpty							 = 2,		// 2 读取空，即缓冲区为空，读取失败
}
emRingBufferErrorCodeTdf;

/// @brief		静态参数定义
///
/// @note
typedef struct
{
	void			*pvHead;								//	缓冲区头，指向缓冲区地址的第一个元素
	void		 	*pvTail;								//	缓冲区尾，指向缓冲区地址的最后一个元素
	uint32_t 	ulElementLength;				//	元素长度
}
stRingBufferStaticParamTdf;


/// @brief		运行参数定义
///
/// @note
typedef struct
{
	void	*pvWrite;										//	写指针，指向第一个可写的地址
	void 	*pvRead;										//	读指针，指向第一个可读的地址
	emRingBufferStatusTdf	emStatus; 	//	缓冲区状态
}
stRingBufferRunningParamTdf;



/// @brief		结构参数定义
///
/// @note
typedef struct
{
	stRingBufferStaticParamTdf  stStaticParam;
	stRingBufferRunningParamTdf stRunningParam;
	
}
stRingBufferDeviceParamTdf;

const stRingBufferDeviceParamTdf *c_pstGetRingBufferDeviceParam(emRingBufferDevNumTdf emDevNum);
emRingBufferErrorCodeTdf vRingBufferWriteSingleElement(void * pvElement, emRingBufferDevNumTdf emDevNum);
emRingBufferErrorCodeTdf vRingBufferReadSingleElement(void * pvElement, emRingBufferDevNumTdf emDevNum);
void *pvRingBufferFindElementFirstPosition(void *c_pvTargetElement, emRingBufferDevNumTdf emDevNum);
void vRingBufferDeviceInit(stRingBufferStaticParamTdf *pstInit, uint8_t emDevNum);
#endif


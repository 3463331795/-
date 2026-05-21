/**
  * @file       nlecloud_device.h
  * @author     Your Name
  * @version    V1.0.0
  * @date       20260518
  * @brief      新大陆云平台通信驱动（纯JSON解析/拼装）
  * 
  * @note       本驱动只负责JSON数据的构建和解析，
  *             与ESP8266解耦，数据发送由调用者决定
  */

#ifndef __NLECLOUD_DEVICE_H__
#define __NLECLOUD_DEVICE_H__

#include "stm32f4xx_hal.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

/// @brief          NleCloud 设备号枚举
typedef enum
{
    emNleCloudDevNum0,
    emNleCloudDevNum1,
    emNleCloudDevNum2,
    emNleCloudDevNum3,
    emNleCloudDevNum
}
emNleCloudDevNumTdf;

/// @brief          NleCloud 操作结果枚举
typedef enum
{
    emNleCloudResult_Err,
    emNleCloudResult_Ok
}
emNleCloudResultTdf;

/// @brief          NleCloud 静态参数定义
///
/// @note           云平台配置信息，初始化后不变
typedef struct
{
    char        acDeviceId[32];             // 设备ID
    char        acApiKey[64];               // APIKEY
    char        acProtocolVer[16];          // 协议版本，如 "v1.1"
}
stNleCloudStaticParamTdf;

/// @brief          NleCloud 运行参数定义
///
/// @note           运行时动态变化的参数
typedef struct
{
    uint32_t    ulMsgId;                   // 消息ID（递增）
}
stNleCloudRunningParamTdf;

/// @brief          NleCloud 设备参数定义
typedef struct
{
    stNleCloudStaticParamTdf   stStaticParam;     // 静态参数
    stNleCloudRunningParamTdf  stRunningParam;    // 运行参数
}
stNleCloudDeviceParamTdf;


// -----------------------------------------
//          外部变量声明
// -----------------------------------------
extern stNleCloudDeviceParamTdf astNleCloudDeviceParam[emNleCloudDevNum];


// -----------------------------------------
//          内部函数声明
// -----------------------------------------
static uint32_t ulNleCloudGenerateMsgId(emNleCloudDevNumTdf emDevNum);


// -----------------------------------------
//          函数声明
// -----------------------------------------

/// @brief      获取 NleCloud 设备参数
///
/// @param      emDevNum   ：设备号
///
/// @note       返回值是 stNleCloudDeviceParamTdf 型的指针
const stNleCloudDeviceParamTdf *c_pstGetNleCloudDeviceParam(emNleCloudDevNumTdf emDevNum);

/// @brief      NleCloud 设备初始化
void vNleCloudDeviceInit(stNleCloudStaticParamTdf *pstInit, emNleCloudDevNumTdf emDevNum);

/// @brief      NleCloud 构建单数据点上传统消息
///
/// @param      emDevNum    ：设备号
/// @param      pcDataName  ：数据点名称
/// @param      fValue      ：数据值
/// @param      pcBuf       ：输出缓冲区
/// @param      pusLen      ：输出长度
///
/// @note       构建数据上传JSON字符串，由调用者发送
void vNleCloudDeviceBuildUploadSingleJson(emNleCloudDevNumTdf emDevNum,
                                          const char *pcDataName,
                                          float fValue,
                                          char *pcBuf,
                                          uint16_t *pusLen);

/// @brief      NleCloud 构建多数据点上传统消息（可变参数版）
///
/// @param      emDevNum    ：设备号
/// @param      ...         ：可变参数，数据点名称和值的交替序列，以 NULL 结束
///
/// @return     生成的JSON字符串
///
/// @note
///             示例调用：
///             const char *json = pcNleCloudDeviceBuildUploadMultipleJson(emNleCloudDevNum0,
///                 "temperature", 25.5f,
///                 "humidity", 60.0f,
///                 "pressure", 101.3f,
///                 NULL);
const char *pcNleCloudDeviceBuildUploadMultipleJson(emNleCloudDevNumTdf emDevNum, ...);

/// @brief      NleCloud 解析JSON命令数据
///
/// @param      pcRxData    ：接收到的JSON数据字符串
/// @param      pcApitag    ：要匹配的apitag标识符（如 "led"）
/// @param      pcDataBuf   ：输出的数据值字符串缓冲区
/// @param      usBufLen    ：缓冲区长度
///
/// @return     emNleCloudResultTdf
///
/// @note
///             支持字符串类型（如 "on"、"OFF"）和数值类型（如 1、0）。
///             示例用法：
///             char acDataBuf[32];
///             if (emNleCloudDeviceParseCmdData(acRxBuf, "led", acDataBuf, sizeof(acDataBuf)) == emNleCloudResult_Ok)
///             {
///                 if (strcmp(acDataBuf, "1") == 0 || strcmp(acDataBuf, "on") == 0)
///                 {
///                     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET);
///                 }
///                 else
///                 {
///                     HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET);
///                 }
///             }
emNleCloudResultTdf emNleCloudDeviceParseCmdData(const char *pcRxData, const char *pcApitag, char *pcDataBuf, uint16_t usBufLen);

/// @brief      NleCloud 构建执行器状态上传统消息（可变参数版）
///
/// @param      emDevNum    ：设备号
/// @param      ...         ：可变参数，执行器名称和状态字符串的交替序列，以 NULL 结束
///
/// @return     生成的JSON字符串
///
/// @note       用于上报执行器（继电器、LED等）的当前状态，支持字符串类型值
///             示例调用：
///             const char *json = pcNleCloudDeviceBuildUploadActuatorJson(emNleCloudDevNum0,
///                 "led1", "on",
///                 "relay2", "off",
///                 NULL);
const char *pcNleCloudDeviceBuildUploadActuatorJson(emNleCloudDevNumTdf emDevNum, ...);

#endif

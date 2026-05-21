/**
  * @file       esp8266_device.h
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260518
  * @brief      ESP8266 WiFi 模块驱动，基于 STM32 HAL 库
  * 
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#ifndef _ESP8266_DEVICE_H_
#define _ESP8266_DEVICE_H_

#include "stm32f4xx_hal.h"
#include "ring_buffer_device.h"

/// @brief          ESP8266 设备号枚举
typedef enum
{
    emEsp8266DevNum0,
    emEsp8266DevNum1,
    emEsp8266DevNum2,
    emEsp8266DevNum3,
}
emEsp8266DevNumTdf;

/// @brief          ESP8266 工作模式枚举
typedef enum
{
    emEsp8266Mode_Station       = 1,
    emEsp8266Mode_SoftAP         = 2,
    emEsp8266Mode_SoftAP_Station = 3
}
emEsp8266ModeTdf;

/// @brief          ESP8266 透传状态枚举
typedef enum
{
    emEsp8266TransMode_Normal,
    emEsp8266TransMode_Unvarnished
}
emEsp8266TransModeTdf;

/// @brief          ESP8266 连接状态枚举
typedef enum
{
    emEsp8266ConnStatus_Disconnected,
    emEsp8266ConnStatus_Connected,
    emEsp8266ConnStatus_GotIP
}
emEsp8266ConnStatusTdf;

/// @brief          ESP8266 返回结果枚举
typedef enum
{
    emEsp8266Result_Err,
    emEsp8266Result_Ok,
    emEsp8266Result_Timeout,
    emEsp8266Result_Error,
    emEsp8266Result_Fail
}
emEsp8266ResultTdf;

/// @brief          ESP8266 静态参数定义
///
/// @note           硬件相关，初始化后不变
typedef struct
{
    UART_HandleTypeDef *pstUartHandle;       // UART 句柄
    GPIO_TypeDef       *pstRstPort;          // RESET 引脚端口
    uint16_t            usRstPin;             // RESET 引脚号
    uint32_t            ulTimeout;            // AT 命令超时时间 (ms)
    emRingBufferDevNumTdf emRingBufferDevNum; // 关联的环形缓冲区设备号
}
stEsp8266StaticParamTdf;

/// @brief          ESP8266 运行参数定义
///
/// @note           运行时动态变化的参数
typedef struct
{
    emEsp8266TransModeTdf    emTransMode;         // 透传模式
    emEsp8266ConnStatusTdf   emConnStatus;        // 连接状态
    
    char                    acSsid[32];           // WiFi SSID
    char                    acPassword[64];        // WiFi 密码
    
    char                    acServerIP[16];        // 服务器 IP
    uint16_t                usServerPort;         // 服务器端口
    
    uint8_t                 ucLinkId;              // TCP/UDP 连接 ID (0-4)
}
stEsp8266RunningParamTdf;

/// @brief          ESP8266 设备参数定义
///
/// @note           包含静态参数和运行参数
typedef struct
{
    stEsp8266StaticParamTdf     stStaticParam;     // 静态参数
    stEsp8266RunningParamTdf    stRunningParam;    // 运行参数
}
stEsp8266DeviceParamTdf;

/// @brief          ESP8266 接收数据结构定义
///
/// @note           用于存储接收到的数据
typedef struct
{
    char    acRxBuf[1024];                        // 接收缓冲区
    uint16_t usRxLen;                             // 接收数据长度
}
stEsp8266RxDataTdf;

/// -----------------------------------------
///          外部变量声明
/// -----------------------------------------
extern stEsp8266DeviceParamTdf astEsp8266DeviceParam[1];
extern stEsp8266RxDataTdf      stEsp8266RxData;

/// -----------------------------------------
///          函数声明
/// -----------------------------------------

/// @brief      获取 ESP8266 设备参数
///
/// @param      emDevNum   ：设备号
///
/// @note       返回值是 stEsp8266DeviceParamTdf 型的指针，且指针指向的内容是不可更改的（只读的）
const stEsp8266DeviceParamTdf *c_pstGetEsp8266DeviceParam(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 设备初始化
///
/// @param      pstInit     ：静态参数结构体的首地址
/// @param      emDevNum    ：设备号
///
/// @note
void vEsp8266DeviceInit(stEsp8266StaticParamTdf *pstInit, emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 硬件复位
///
/// @param      emDevNum    ：设备号
///
/// @note       通过拉低 RST 引脚复位模块
void vEsp8266HardwareReset(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 发送 AT 命令（通用）
///
/// @param      emDevNum    ：设备号
/// @param      pcCmd       ：AT 命令字符串
///
/// @note       发送命令后自动添加 \r\n
emEsp8266ResultTdf emEsp8266SendAtCmd(emEsp8266DevNumTdf emDevNum, const char *pcCmd);

/// @brief      ESP8266 发送 AT 命令并等待响应（通用）
///
/// @param      emDevNum    ：设备号
/// @param      pcCmd       ：AT 命令字符串
/// @param      pcAck       ：期望的响应字符串
///
/// @note       发送命令后自动添加 \r\n
emEsp8266ResultTdf emEsp8266SendAtCmdWithAck(emEsp8266DevNumTdf emDevNum, const char *pcCmd, const char *pcAck);

/// @brief      ESP8266 测试命令
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "AT" 测试模块是否正常
emEsp8266ResultTdf emEsp8266Test(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 设置工作模式
///
/// @param      emDevNum    ：设备号
/// @param      emMode      ：工作模式
///
/// @note       1 = Station, 2 = SoftAP, 3 = SoftAP + Station
emEsp8266ResultTdf emEsp8266SetMode(emEsp8266DevNumTdf emDevNum, emEsp8266ModeTdf emMode);

/// @brief      ESP8266 连接 WiFi
///
/// @param      emDevNum    ：设备号
/// @param      pcSsid      ：WiFi SSID
/// @param      pcPassword  ：WiFi 密码
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectWifi(emEsp8266DevNumTdf emDevNum, const char *pcSsid, const char *pcPassword);

/// @brief      ESP8266 断开 WiFi 连接
///
/// @param      emDevNum    ：设备号
///
/// @note
emEsp8266ResultTdf emEsp8266DisconnectWifi(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 查询连接状态
///
/// @param      emDevNum    ：设备号
///
/// @note       返回连接状态
emEsp8266ConnStatusTdf emEsp8266QueryConnectionStatus(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 建立 TCP 连接
///
/// @param      emDevNum    ：设备号
/// @param      pcServerIP  ：服务器 IP 地址
/// @param      usPort      ：服务器端口
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectTcp(emEsp8266DevNumTdf emDevNum, const char *pcServerIP, uint16_t usPort);

/// @brief      ESP8266 建立 UDP 连接
///
/// @param      emDevNum    ：设备号
/// @param      pcServerIP  ：服务器 IP 地址
/// @param      usPort      ：服务器端口
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectUdp(emEsp8266DevNumTdf emDevNum, const char *pcServerIP, uint16_t usPort);

/// @brief      ESP8266 关闭连接
///
/// @param      emDevNum    ：设备号
///
/// @note       关闭当前 TCP/UDP 连接
emEsp8266ResultTdf emEsp8266CloseConnection(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 进入透传模式
///
/// @param      emDevNum    ：设备号
///
/// @note
emEsp8266ResultTdf emEsp8266EnterTransMode(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 退出透传模式
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "+++" 退出透传
void vEsp8266ExitTransMode(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 发送数据
///
/// @param      emDevNum    ：设备号
/// @param      pcData      ：数据指针
/// @param      usLen       ：数据长度
///
/// @note
emEsp8266ResultTdf emEsp8266SendData(emEsp8266DevNumTdf emDevNum, const uint8_t *pcData, uint16_t usLen);

/// @brief      ESP8266 发送字符串
///
/// @param      emDevNum    ：设备号
/// @param      pcStr       ：字符串指针
///
/// @note
emEsp8266ResultTdf emEsp8266SendString(emEsp8266DevNumTdf emDevNum, const char *pcStr);

/// @brief      ESP8266 环形缓冲区读取单个字节
///
/// @param      emDevNum    ：设备号
/// @param      pucData     ：读取到的数据存放地址
///
/// @return     成功返回 1，环形缓冲区空返回 0
///
/// @note       从关联的环形缓冲区读取一个字节
uint8_t ucEsp8266RingBufferReadByte(emEsp8266DevNumTdf emDevNum, uint8_t *pucData);

/// @brief      ESP8266 从环形缓冲区读取一行（以 '\n' 结尾）
///
/// @param      emDevNum    ：设备号
/// @param      pcLine      ：存储读取行的缓冲区
/// @param      usMaxLen    ：缓冲区最大长度
///
/// @return     返回读取的字节数（不含换行符），无可用行返回 0
///
/// @note       读取到 '\n' 时停止，不含 '\r' 和 '\n'
uint16_t usEsp8266RingBufferReadLine(emEsp8266DevNumTdf emDevNum, char *pcLine, uint16_t usMaxLen);

/// @brief      ESP8266 非阻塞读取所有可用数据（不等待换行）
///
/// @param      emDevNum    ：设备号
/// @param      pcBuf       ：存储数据的缓冲区
/// @param      usMaxLen    ：缓冲区最大长度
///
/// @return     返回读取的字节数
///
/// @note       读取环形缓冲区中所有可用数据，用于解析无换行的JSON响应
uint16_t usEsp8266RingBufferReadAll(emEsp8266DevNumTdf emDevNum, char *pcBuf, uint16_t usMaxLen);

/// @brief      ESP8266 配置自动连接
///
/// @param      emDevNum    ：设备号
/// @param      ucEnable    ：使能标志 (0: 禁止, 1: 使能)
///
/// @note       上电自动连接上次保存的 WiFi
emEsp8266ResultTdf emEsp8266SetAutoConnect(emEsp8266DevNumTdf emDevNum, uint8_t ucEnable);

/// @brief      ESP8266 保存配置到 Flash
///
/// @param      emDevNum    ：设备号
///
/// @note       保存当前配置到 Flash
emEsp8266ResultTdf emEsp8266SaveConfig(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 恢复出厂设置
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "AT+RESTORE" 恢复出厂设置
emEsp8266ResultTdf emEsp8266Restore(emEsp8266DevNumTdf emDevNum);

/// @brief      ESP8266 NleCloud 设备注册
///
/// @param      emDevNum        ：设备号
/// @param      pcDeviceId      ：设备ID
/// @param      pcApiKey        ：API密钥
///
/// @return     emEsp8266ResultTdf
///
/// @note       向 NleCloud 云平台发送设备注册信息
emEsp8266ResultTdf emEsp8266NleCloudRegister(emEsp8266DevNumTdf emDevNum, const char *pcDeviceId, const char *pcApiKey);

/// @brief      ESP8266 周期执行（用于状态检测）
///
/// @param      emDevNum    ：设备号
///
/// @note       定期调用检测连接状态
void vEsp8266DevicePeriodExecute(emEsp8266DevNumTdf emDevNum);

#endif

/**
  * @file       esp8266_device.c
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260518
  * @brief      ESP8266 WiFi 模块驱动，基于 STM32 HAL 库
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#include "esp8266_device.h"
#include <string.h>
#include <stdio.h>

/// @brief          ESP8266 设备参数表
stEsp8266DeviceParamTdf astEsp8266DeviceParam[1];

/// @brief          ESP8266 接收数据结构体
stEsp8266RxDataTdf stEsp8266RxData;

/// @brief          ESP8266 AT 命令发送缓冲区
#define ESP8266_CMD_BUF_SIZE     256
static char s_acCmdBuf[ESP8266_CMD_BUF_SIZE];

/// @brief          获取 ESP8266 设备参数
///
/// @param      emDevNum   ：设备号
///
/// @note       注意，返回值是 stEsp8266DeviceParamTdf 型的指针，且指针指向的内容是不可更改的（只读的）
const stEsp8266DeviceParamTdf *c_pstGetEsp8266DeviceParam(emEsp8266DevNumTdf emDevNum)
{
    return &astEsp8266DeviceParam[emDevNum];
}

/// @brief      ESP8266 设备初始化
///
/// @param      pstInit     ：静态参数结构体的首地址
/// @param      emDevNum    ：设备号
///
/// @note
void vEsp8266DeviceInit(stEsp8266StaticParamTdf *pstInit, emEsp8266DevNumTdf emDevNum)
{
    // 1. 初始化静态参数
    memcpy(&astEsp8266DeviceParam[emDevNum].stStaticParam, pstInit, sizeof(stEsp8266StaticParamTdf));
    
    // 2. 初始化运行参数
    astEsp8266DeviceParam[emDevNum].stRunningParam.emTransMode = emEsp8266TransMode_Normal;
    astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_Disconnected;
    astEsp8266DeviceParam[emDevNum].stRunningParam.usServerPort = 0;
    astEsp8266DeviceParam[emDevNum].stRunningParam.ucLinkId = 0;
    
    // 3. 清空接收缓冲区
    memset(&stEsp8266RxData, 0, sizeof(stEsp8266RxDataTdf));
}

/// @brief      ESP8266 硬件复位
///
/// @param      emDevNum    ：设备号
///
/// @note       通过拉低 RST 引脚复位模块
void vEsp8266HardwareReset(emEsp8266DevNumTdf emDevNum)
{
    // 1. 拉低 RST 引脚，复位模块
    HAL_GPIO_WritePin(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstRstPort,
        astEsp8266DeviceParam[emDevNum].stStaticParam.usRstPin,
        GPIO_PIN_RESET
    );
    
    // 2. 延时至少 10ms
    HAL_Delay(20);
    
    // 3. 释放 RST 引脚
    HAL_GPIO_WritePin(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstRstPort,
        astEsp8266DeviceParam[emDevNum].stStaticParam.usRstPin,
        GPIO_PIN_SET
    );
    
    // 4. 等待模块启动
    HAL_Delay(500);
}

/// @brief      ESP8266 发送 AT 命令（通用）
///
/// @param      emDevNum    ：设备号
/// @param      pcCmd       ：AT 命令字符串
///
/// @note       发送命令后自动添加 \r\n
emEsp8266ResultTdf emEsp8266SendAtCmd(emEsp8266DevNumTdf emDevNum, const char *pcCmd)
{
    // 1. 构造完整命令（添加 \r\n）
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "%s\r\n", pcCmd);
    
    // 2. 发送命令
    HAL_UART_Transmit(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstUartHandle,
        (uint8_t *)s_acCmdBuf,
        strlen(s_acCmdBuf),
        astEsp8266DeviceParam[emDevNum].stStaticParam.ulTimeout
    );
    
    return emEsp8266Result_Ok;
}

/// @brief      ESP8266 发送 AT 命令并等待响应（通用）
///
/// @param      emDevNum    ：设备号
/// @param      pcCmd       ：AT 命令字符串（已包含 \r\n）
/// @param      pcAck       ：期望的响应字符串
///
/// @note       发送命令后自动添加 \r\n
emEsp8266ResultTdf emEsp8266SendAtCmdWithAck(emEsp8266DevNumTdf emDevNum, const char *pcCmd, const char *pcAck)
{
    uint32_t ulStartTick;
    uint32_t ulTimeout;
    uint8_t ucData;

    // 1. 清空接收行缓冲区
    stEsp8266RxData.usRxLen = 0;
    stEsp8266RxData.acRxBuf[0] = '\0';

    // 2. 发送命令（命令已包含 \r\n）
    HAL_UART_Transmit(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstUartHandle,
        (uint8_t *)pcCmd,
        strlen(pcCmd),
        astEsp8266DeviceParam[emDevNum].stStaticParam.ulTimeout
    );

    // 3. 等待响应
    ulTimeout = astEsp8266DeviceParam[emDevNum].stStaticParam.ulTimeout;
    ulStartTick = HAL_GetTick();

    while(HAL_GetTick() - ulStartTick < ulTimeout)
    {
        // 3.1 从环形缓冲区读取字节
        while(ucEsp8266RingBufferReadByte(emDevNum, &ucData))
        {
            // 存储到行缓冲区
            if(stEsp8266RxData.usRxLen < sizeof(stEsp8266RxData.acRxBuf) - 1)
            {
                stEsp8266RxData.acRxBuf[stEsp8266RxData.usRxLen++] = (char)ucData;
                stEsp8266RxData.acRxBuf[stEsp8266RxData.usRxLen] = '\0';
            }
        }

        // 3.2 检查是否接收到期望的响应
        if(strstr(stEsp8266RxData.acRxBuf, pcAck) != NULL)
        {
            return emEsp8266Result_Ok;
        }

        // 3.3 检查是否返回 ERROR
        if(strstr(stEsp8266RxData.acRxBuf, "ERROR") != NULL)
        {
            return emEsp8266Result_Error;
        }

        // 3.4 检查是否返回 FAIL
        if(strstr(stEsp8266RxData.acRxBuf, "FAIL") != NULL)
        {
            return emEsp8266Result_Fail;
        }

        // 3.5 延时，避免频繁查询
        HAL_Delay(5);
    }

    return emEsp8266Result_Timeout;
}

/// @brief      ESP8266 测试命令
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "AT" 测试模块是否正常
emEsp8266ResultTdf emEsp8266Test(emEsp8266DevNumTdf emDevNum)
{
    return emEsp8266SendAtCmdWithAck(emDevNum, "AT\r\n", "OK");
}

/// @brief      ESP8266 设置工作模式
///
/// @param      emDevNum    ：设备号
/// @param      emMode      ：工作模式
///
/// @note       1 = Station, 2 = SoftAP, 3 = SoftAP + Station
emEsp8266ResultTdf emEsp8266SetMode(emEsp8266DevNumTdf emDevNum, emEsp8266ModeTdf emMode)
{
    // 1. 构造 AT 命令
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "AT+CWMODE=%d\r\n", emMode);
    
    // 2. 发送并等待响应
    return emEsp8266SendAtCmdWithAck(emDevNum, s_acCmdBuf, "OK");
}

/// @brief      ESP8266 连接 WiFi
///
/// @param      emDevNum    ：设备号
/// @param      pcSsid      ：WiFi SSID
/// @param      pcPassword  ：WiFi 密码
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectWifi(emEsp8266DevNumTdf emDevNum, const char *pcSsid, const char *pcPassword)
{
    emEsp8266ResultTdf emResult;
    
    // 1. 保存 SSID 和密码
    strncpy(astEsp8266DeviceParam[emDevNum].stRunningParam.acSsid, pcSsid, sizeof(astEsp8266DeviceParam[emDevNum].stRunningParam.acSsid) - 1);
    strncpy(astEsp8266DeviceParam[emDevNum].stRunningParam.acPassword, pcPassword, sizeof(astEsp8266DeviceParam[emDevNum].stRunningParam.acPassword) - 1);
    
    // 2. 构造 AT 命令: AT+CWJAP="SSID","PASSWORD"
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "AT+CWJAP=\"%s\",\"%s\"\r\n", pcSsid, pcPassword);
    
    // 3. 延长超时时间（WiFi 连接可能需要较长时间）
    astEsp8266DeviceParam[emDevNum].stStaticParam.ulTimeout = 10000;
    
    // 4. 发送并等待响应
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, s_acCmdBuf, "OK");
    
    // 5. 恢复默认超时时间
    astEsp8266DeviceParam[emDevNum].stStaticParam.ulTimeout = 2000;
    
    // 6. 更新连接状态
    if(emResult == emEsp8266Result_Ok)
    {
        astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_Connected;
    }
    
    return emResult;
}

/// @brief      ESP8266 断开 WiFi 连接
///
/// @param      emDevNum    ：设备号
///
/// @note
emEsp8266ResultTdf emEsp8266DisconnectWifi(emEsp8266DevNumTdf emDevNum)
{
    emEsp8266ResultTdf emResult;
    
    // 1. 发送断开命令
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, "AT+CWQAP\r\n", "OK");
    
    // 2. 更新状态
    if(emResult == emEsp8266Result_Ok)
    {
        astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_Disconnected;
    }
    
    return emResult;
}

/// @brief      ESP8266 查询连接状态
///
/// @param      emDevNum    ：设备号
///
/// @note       返回连接状态
emEsp8266ConnStatusTdf emEsp8266QueryConnectionStatus(emEsp8266DevNumTdf emDevNum)
{
    char *pcStatus;
    uint8_t ucData;

    // 1. 清空接收行缓冲区
    stEsp8266RxData.usRxLen = 0;
    stEsp8266RxData.acRxBuf[0] = '\0';

    // 2. 发送查询命令
    emEsp8266SendAtCmd(emDevNum, "AT+CIPSTATUS");

    // 3. 等待响应并解析
    HAL_Delay(100);

    // 4. 从环形缓冲区读取所有可用数据
    while(ucEsp8266RingBufferReadByte(emDevNum, &ucData))
    {
        if(stEsp8266RxData.usRxLen < sizeof(stEsp8266RxData.acRxBuf) - 1)
        {
            stEsp8266RxData.acRxBuf[stEsp8266RxData.usRxLen++] = (char)ucData;
            stEsp8266RxData.acRxBuf[stEsp8266RxData.usRxLen] = '\0';
        }
    }

    // 5. 解析连接状态
    pcStatus = strstr(stEsp8266RxData.acRxBuf, "+CIPSTATUS:");
    if(pcStatus != NULL)
    {
        // 检查是否包含 IP 地址
        if(strstr(stEsp8266RxData.acRxBuf, "IP") != NULL)
        {
            astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_GotIP;
        }
        else
        {
            astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_Connected;
        }
    }
    else
    {
        astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus = emEsp8266ConnStatus_Disconnected;
    }

    return astEsp8266DeviceParam[emDevNum].stRunningParam.emConnStatus;
}

/// @brief      ESP8266 建立 TCP 连接
///
/// @param      emDevNum    ：设备号
/// @param      pcServerIP  ：服务器 IP 地址
/// @param      usPort      ：服务器端口
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectTcp(emEsp8266DevNumTdf emDevNum, const char *pcServerIP, uint16_t usPort)
{
    emEsp8266ResultTdf emResult;
    
    // 1. 保存服务器信息
    strncpy(astEsp8266DeviceParam[emDevNum].stRunningParam.acServerIP, pcServerIP, sizeof(astEsp8266DeviceParam[emDevNum].stRunningParam.acServerIP) - 1);
    astEsp8266DeviceParam[emDevNum].stRunningParam.usServerPort = usPort;
    
    // 2. 先关闭已有连接
    emEsp8266CloseConnection(emDevNum);
    HAL_Delay(100);
    
    // 3. 设置单连接模式
    emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPMUX=0\r\n", "OK");
    
    // 4. 构造 AT 命令: AT+CIPSTART="TCP","IP",PORT
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", pcServerIP, usPort);
    
    // 5. 发送并等待响应
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, s_acCmdBuf, "OK");
    
    return emResult;
}

/// @brief      ESP8266 建立 UDP 连接
///
/// @param      emDevNum    ：设备号
/// @param      pcServerIP  ：服务器 IP 地址
/// @param      usPort      ：服务器端口
///
/// @note
emEsp8266ResultTdf emEsp8266ConnectUdp(emEsp8266DevNumTdf emDevNum, const char *pcServerIP, uint16_t usPort)
{
    emEsp8266ResultTdf emResult;
    
    // 1. 保存服务器信息
    strncpy(astEsp8266DeviceParam[emDevNum].stRunningParam.acServerIP, pcServerIP, sizeof(astEsp8266DeviceParam[emDevNum].stRunningParam.acServerIP) - 1);
    astEsp8266DeviceParam[emDevNum].stRunningParam.usServerPort = usPort;
    
    // 2. 先关闭已有连接
    emEsp8266CloseConnection(emDevNum);
    HAL_Delay(100);
    
    // 3. 设置单连接模式
    emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPMUX=0\r\n", "OK");
    
    // 4. 构造 AT 命令: AT+CIPSTART="UDP","IP",PORT
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "AT+CIPSTART=\"UDP\",\"%s\",%d\r\n", pcServerIP, usPort);
    
    // 5. 发送并等待响应
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, s_acCmdBuf, "OK");
    
    return emResult;
}

/// @brief      ESP8266 关闭连接
///
/// @param      emDevNum    ：设备号
///
/// @note       关闭当前 TCP/UDP 连接
emEsp8266ResultTdf emEsp8266CloseConnection(emEsp8266DevNumTdf emDevNum)
{
    return emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPCLOSE\r\n", "OK");
}

/// @brief      ESP8266 进入透传模式
///
/// @param      emDevNum    ：设备号
///
/// @note
emEsp8266ResultTdf emEsp8266EnterTransMode(emEsp8266DevNumTdf emDevNum)
{
    emEsp8266ResultTdf emResult;
    
    // 1. 设置单连接模式
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPMUX=0\r\n", "OK");
    if(emResult != emEsp8266Result_Ok)
    {
        return emResult;
    }
    HAL_Delay(50);
    
    // 2. 设置透传模式
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPMODE=1\r\n", "OK");
    if(emResult != emEsp8266Result_Ok)
    {
        return emResult;
    }
    HAL_Delay(50);
    
    // 3. 开始透传
    emResult = emEsp8266SendAtCmdWithAck(emDevNum, "AT+CIPSEND\r\n", ">");
    if(emResult != emEsp8266Result_Ok)
    {
        return emResult;
    }
    
    // 4. 更新状态
    astEsp8266DeviceParam[emDevNum].stRunningParam.emTransMode = emEsp8266TransMode_Unvarnished;
    
    return emResult;
}

/// @brief      ESP8266 退出透传模式
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "+++" 退出透传（注意：不带 \r\n）
void vEsp8266ExitTransMode(emEsp8266DevNumTdf emDevNum)
{
    // 1. 更新状态
    astEsp8266DeviceParam[emDevNum].stRunningParam.emTransMode = emEsp8266TransMode_Normal;
    
    // 2. 延时确保之前的传输完成
    HAL_Delay(20);
    
    // 3. 发送 +++（不带 \r\n）
    HAL_UART_Transmit(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstUartHandle,
        (uint8_t *)"+++",
        3,
        100
    );
    
    // 4. 延时确保切换完成
    HAL_Delay(20);
}

/// @brief      ESP8266 发送数据
///
/// @param      emDevNum    ：设备号
/// @param      pcData      ：数据指针
/// @param      usLen       ：数据长度
///
/// @note
emEsp8266ResultTdf emEsp8266SendData(emEsp8266DevNumTdf emDevNum, const uint8_t *pcData, uint16_t usLen)
{
    char acHead[32];
    
    // 1. 透传模式下直接发送数据
    if(astEsp8266DeviceParam[emDevNum].stRunningParam.emTransMode == emEsp8266TransMode_Unvarnished)
    {
        HAL_UART_Transmit(
            astEsp8266DeviceParam[emDevNum].stStaticParam.pstUartHandle,
            pcData,
            usLen,
            usLen + 100
        );
        return emEsp8266Result_Ok;
    }
    
    // 2. 非透传模式下，需要先发送长度
    snprintf(acHead, sizeof(acHead), "AT+CIPSEND=%d\r\n", usLen);
    
    // 3. 发送 CIPSEND 命令
    if(emEsp8266SendAtCmdWithAck(emDevNum, acHead, ">") != emEsp8266Result_Ok)
    {
        return emEsp8266Result_Err;
    }
    
    // 4. 发送数据
    HAL_UART_Transmit(
        astEsp8266DeviceParam[emDevNum].stStaticParam.pstUartHandle,
        pcData,
        usLen,
        usLen + 100
    );
    
    // 5. 等待发送完成
    HAL_Delay(10);
    
    return emEsp8266Result_Ok;
}

/// @brief      ESP8266 发送字符串
///
/// @param      emDevNum    ：设备号
/// @param      pcStr       ：字符串指针
///
/// @note
emEsp8266ResultTdf emEsp8266SendString(emEsp8266DevNumTdf emDevNum, const char *pcStr)
{
    return emEsp8266SendData(emDevNum, (const uint8_t *)pcStr, strlen(pcStr));
}

/// @brief      ESP8266 环形缓冲区读取单个字节
///
/// @param      emDevNum    ：设备号
/// @param      pucData     ：读取到的数据存放地址
///
/// @return     成功返回 1，环形缓冲区空返回 0
///
/// @note       从关联的环形缓冲区读取一个字节
uint8_t ucEsp8266RingBufferReadByte(emEsp8266DevNumTdf emDevNum, uint8_t *pucData)
{
    emRingBufferErrorCodeTdf emResult;

    emResult = vRingBufferReadSingleElement(pucData,
        astEsp8266DeviceParam[emDevNum].stStaticParam.emRingBufferDevNum);

    return (emResult == emRingBufferError_None) ? 1 : 0;
}

/// @brief      ESP8266 从环形缓冲区读取一行（以 '\n' 结尾）
///
/// @param      emDevNum    ：设备号
/// @param      pcLine      ：存储读取行的缓冲区
/// @param      usMaxLen    ：缓冲区最大长度
///
/// @return     返回读取的字节数（不含换行符），无可用行返回 0
///
/// @note       读取到 '\n' 时停止，不含 '\r' 和 '\n'
uint16_t usEsp8266RingBufferReadLine(emEsp8266DevNumTdf emDevNum, char *pcLine, uint16_t usMaxLen)
{
    uint8_t ucData;
    uint16_t usCount = 0;

    while(ucEsp8266RingBufferReadByte(emDevNum, &ucData))
    {
        if(ucData == '\n')
        {
            break;
        }

        if(ucData != '\r')
        {
            if(usCount < usMaxLen - 1)
            {
                pcLine[usCount++] = (char)ucData;
            }
        }
    }

    pcLine[usCount] = '\0';
    return usCount;
}

/// @brief      ESP8266 非阻塞读取所有可用数据（不等待换行）
///
/// @param      emDevNum    ：设备号
/// @param      pcBuf       ：存储数据的缓冲区
/// @param      usMaxLen    ：缓冲区最大长度
///
/// @return     返回读取的字节数
///
/// @note       读取环形缓冲区中所有可用数据，用于解析无换行的JSON响应
uint16_t usEsp8266RingBufferReadAll(emEsp8266DevNumTdf emDevNum, char *pcBuf, uint16_t usMaxLen)
{
    uint8_t ucData;
    uint16_t usCount = 0;

    while(ucEsp8266RingBufferReadByte(emDevNum, &ucData))
    {
        if(usCount < usMaxLen - 1)
        {
            pcBuf[usCount++] = (char)ucData;
        }
    }

    pcBuf[usCount] = '\0';
    return usCount;
}

/// @brief      ESP8266 配置自动连接
///
/// @param      emDevNum    ：设备号
/// @param      ucEnable    ：使能标志 (0: 禁止, 1: 使能)
///
/// @note       上电自动连接上次保存的 WiFi
emEsp8266ResultTdf emEsp8266SetAutoConnect(emEsp8266DevNumTdf emDevNum, uint8_t ucEnable)
{
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE, "AT+CWAUTOCONN=%d\r\n", ucEnable);
    return emEsp8266SendAtCmdWithAck(emDevNum, s_acCmdBuf, "OK");
}

/// @brief      ESP8266 保存配置到 Flash
///
/// @param      emDevNum    ：设备号
///
/// @note       保存当前配置到 Flash
emEsp8266ResultTdf emEsp8266SaveConfig(emEsp8266DevNumTdf emDevNum)
{
    return emEsp8266SendAtCmdWithAck(emDevNum, "AT+SAVETRANSLINK=1\r\n", "OK");
}

/// @brief      ESP8266 恢复出厂设置
///
/// @param      emDevNum    ：设备号
///
/// @note       发送 "AT+RESTORE" 恢复出厂设置
emEsp8266ResultTdf emEsp8266Restore(emEsp8266DevNumTdf emDevNum)
{
    return emEsp8266SendAtCmdWithAck(emDevNum, "AT+RESTORE\r\n", "OK");
}

/// @brief      ESP8266 NleCloud 设备注册
///
/// @param      emDevNum        ：设备号
/// @param      pcDeviceId      ：设备ID
/// @param      pcApiKey        ：API密钥
///
/// @return     emEsp8266ResultTdf
///
/// @note       向 NleCloud 云平台发送设备注册信息
emEsp8266ResultTdf emEsp8266NleCloudRegister(emEsp8266DevNumTdf emDevNum, const char *pcDeviceId, const char *pcApiKey)
{
    snprintf(s_acCmdBuf, ESP8266_CMD_BUF_SIZE,
        "{\"t\":1,\"device\":\"%s\",\"key\":\"%s\",\"ver\":\"v1.1\"}\r\n",
        pcDeviceId, pcApiKey);

    return emEsp8266SendString(emDevNum, s_acCmdBuf);
}

/// @brief      ESP8266 周期执行（用于状态检测）
///
/// @param      emDevNum    ：设备号
///
/// @note       定期调用检测连接状态
void vEsp8266DevicePeriodExecute(emEsp8266DevNumTdf emDevNum)
{
    // 定期查询连接状态
    emEsp8266QueryConnectionStatus(emDevNum);
}


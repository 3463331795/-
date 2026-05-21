/**
  * @file       nlecloud_device.c
  * @author     Your Name
  * @version    V1.0.0
  * @date       20260518
  * @brief      新大陆云平台通信驱动实现（纯JSON解析/拼装）
  * 
  * @note       本驱动只负责JSON数据的构建和解析，
  *             与ESP8266解耦，数据发送由调用者决定
  */

#include "nlecloud_device.h"

// -----------------------------------------
//          全局变量定义
// -----------------------------------------
stNleCloudDeviceParamTdf astNleCloudDeviceParam[emNleCloudDevNum];      // 设备参数表




// -----------------------------------------
//          内部函数声明
// -----------------------------------------
static uint32_t ulNleCloudGenerateMsgId(emNleCloudDevNumTdf emDevNum);


// -----------------------------------------
//          函数实现
// -----------------------------------------

/// @brief      获取 NleCloud 设备参数
const stNleCloudDeviceParamTdf *c_pstGetNleCloudDeviceParam(emNleCloudDevNumTdf emDevNum)
{
    return &astNleCloudDeviceParam[emDevNum];
}


/// @brief      NleCloud 设备初始化
void vNleCloudDeviceInit(stNleCloudStaticParamTdf *pstInit, emNleCloudDevNumTdf emDevNum)
{
    // 保存静态参数
    memcpy(&astNleCloudDeviceParam[emDevNum].stStaticParam,
           pstInit,
           sizeof(stNleCloudStaticParamTdf));

    // 初始化运行参数
    astNleCloudDeviceParam[emDevNum].stRunningParam.ulMsgId = 0;
}


/// @brief      生成消息ID
static uint32_t ulNleCloudGenerateMsgId(emNleCloudDevNumTdf emDevNum)
{
    astNleCloudDeviceParam[emDevNum].stRunningParam.ulMsgId++;
    return astNleCloudDeviceParam[emDevNum].stRunningParam.ulMsgId;
}


/// @brief      NleCloud 构建单数据点上传统消息
void vNleCloudDeviceBuildUploadSingleJson(emNleCloudDevNumTdf emDevNum,
                                          const char *pcDataName,
                                          float fValue,
                                          char *pcBuf,
                                          uint16_t *pusLen)
{
    uint32_t ulMsgId = ulNleCloudGenerateMsgId(emDevNum);

    *pusLen = sprintf(pcBuf,
        "{\"t\":3,\"datatype\":1,\"datas\":{\"%s\":%.1f},\"msgid\":%lu}\r\n",
        pcDataName,
        fValue,
        ulMsgId);
}


/// @brief      NleCloud 构建多数据点上传统消息（可变参数版）
const char *pcNleCloudDeviceBuildUploadMultipleJson(emNleCloudDevNumTdf emDevNum, ...)
{
    static char sc_acJsonBuf[256];
    va_list stArgs;
    const char *pcDataName;
    float fValue;
    uint16_t usOffset = 0;
    uint8_t ucFirstFlag = 1;
    uint32_t ulMsgId = ulNleCloudGenerateMsgId(emDevNum);

    // 消息头
    usOffset = sprintf(&sc_acJsonBuf[usOffset],
        "{\"t\":3,\"datatype\":1,\"datas\":{");

    // 可变参数解析
    va_start(stArgs, emDevNum);

    while (1)
    {
        // 获取数据点名称
        pcDataName = va_arg(stArgs, const char *);

        // 如果名称为NULL，结束解析
        if (pcDataName == NULL)
        {
            break;
        }

        // 获取数据值
        fValue = (float)va_arg(stArgs, double);

        // 添加逗号分隔（第一个数据点前不加逗号）
        if (!ucFirstFlag)
        {
            usOffset += sprintf(&sc_acJsonBuf[usOffset], ",");
        }
        ucFirstFlag = 0;

        // 添加数据点
        usOffset += sprintf(&sc_acJsonBuf[usOffset], "\"%s\":%.1f", pcDataName, fValue);
    }

    va_end(stArgs);

    // 消息尾
    usOffset += sprintf(&sc_acJsonBuf[usOffset], "},\"msgid\":%lu}\r\n", ulMsgId);

    return sc_acJsonBuf;
}


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
emNleCloudResultTdf emNleCloudDeviceParseCmdData(const char *pcRxData, const char *pcApitag, char *pcDataBuf, uint16_t usBufLen)
{
    char *pcApitagStart;
    char *pcDataStart;
    char *pcDataEnd;
    int16_t sLen;

    // 构造 "apitag":"xxx" 的查找模式
    char acApitagPattern[64];
    sprintf(acApitagPattern, "\"apitag\":\"%s\"", pcApitag);

    // 查找匹配的 apitag
    pcApitagStart = strstr(pcRxData, acApitagPattern);
    if (pcApitagStart == NULL)
    {
        return emNleCloudResult_Err;
    }

    // 查找 "data": 后的值
    pcDataStart = strstr(pcApitagStart, "\"data\":");
    if (pcDataStart == NULL)
    {
        return emNleCloudResult_Err;
    }
    pcDataStart += 7; // 跳过 "\"data\":"

    // 跳过空格
    while (*pcDataStart == ' ')
    {
        pcDataStart++;
    }

    // 判断数据类型：字符串（以引号开头）还是数值
    if (*pcDataStart == '"')
    {
        // 字符串类型：提取引号内的内容
        pcDataStart++; // 跳过开始的引号
        pcDataEnd = strchr(pcDataStart, '"');
        if (pcDataEnd == NULL)
        {
            return emNleCloudResult_Err;
        }
    }
    else
    {
        // 数值类型：提取连续的数字和可能的小数点
        pcDataEnd = pcDataStart;
        while ((*pcDataEnd >= '0' && *pcDataEnd <= '9') || *pcDataEnd == '.' || *pcDataEnd == '-')
        {
            pcDataEnd++;
        }
    }

    sLen = pcDataEnd - pcDataStart;
    if (sLen <= 0 || sLen >= usBufLen)
    {
        return emNleCloudResult_Err;
    }

    strncpy(pcDataBuf, pcDataStart, sLen);
    pcDataBuf[sLen] = '\0';

    return emNleCloudResult_Ok;
}

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
const char *pcNleCloudDeviceBuildUploadActuatorJson(emNleCloudDevNumTdf emDevNum, ...)
{
    static char sc_acJsonBuf[256];
    va_list stArgs;
    const char *pcActuatorName;
    const char *pcValue;
    uint16_t usOffset = 0;
    uint8_t ucFirstFlag = 1;
    uint32_t ulMsgId = ulNleCloudGenerateMsgId(emDevNum);

    // 消息头 (t=3: 数据上报, datatype=1: JSON格式1字符串)
    usOffset = sprintf(&sc_acJsonBuf[usOffset],
        "{\"t\":3,\"datatype\":1,\"datas\":{");

    // 可变参数解析
    va_start(stArgs, emDevNum);

    while (1)
    {
        // 获取执行器名称
        pcActuatorName = va_arg(stArgs, const char *);

        // 如果名称为NULL，结束解析
        if (pcActuatorName == NULL)
        {
            break;
        }

        // 获取状态值字符串
        pcValue = va_arg(stArgs, const char *);

        // 添加逗号分隔（第一个执行器前不加逗号）
        if (!ucFirstFlag)
        {
            usOffset += sprintf(&sc_acJsonBuf[usOffset], ",");
        }
        ucFirstFlag = 0;

        // 添加执行器数据（值为字符串，用引号包裹）
        usOffset += sprintf(&sc_acJsonBuf[usOffset], "\"%s\":\"%s\"", pcActuatorName, pcValue);
    }

    va_end(stArgs);

    // 消息尾
    usOffset += sprintf(&sc_acJsonBuf[usOffset], "},\"msgid\":%lu}\r\n", ulMsgId);

    return sc_acJsonBuf;
}

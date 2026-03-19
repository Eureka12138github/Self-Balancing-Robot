/**
 ******************************************************************************
 * @file    usart.c
 * @author  Eureka
 * @brief   基于 STM32 标准外设库的多串口驱动（支持 USART1/2/3）
 *
 * @note    - 使用环形缓冲区实现非阻塞收发
 *          - 发送：调用 Serial_SendXXX() 入队，由 TXE 中断自动发送
 *          - 接收：RXNE 中断自动存入接收缓冲区
 *          - 支持 printf 重定向（通过 USART_DEBUG 宏指定）
 *
 * @usage   1. 在 bsp_usart.h 中配置 SERIAL_USE_USARTx 和 TX/RX_BUF_SIZE
 *          2. 调用 UsartX_Init(baud) 初始化
 *          3. 使用 Serial_SendString(USARTx, "...") 等 API
 *          4. 使用 Serial_Available() / Serial_ReadByte() 读取数据
 *
 ******************************************************************************
 */

#include "usart.h"
#include "bsp_config.h"

/* ======================== 类型定义 ======================== */

/**
 * @brief 串口句柄结构体，管理每个 USART 的收发状态
 */
typedef struct {
    cbuf_handle_t tx_cbuf;      /*!< 发送环形缓冲区句柄 */
    cbuf_handle_t rx_cbuf;      /*!< 接收环形缓冲区句柄 */
    volatile bool tx_sending;   /*!< 当前是否正在发送中 */
} serial_handle_t;

/* ======================== 静态缓冲区与句柄 ======================== */

#if SERIAL_USE_USART1
static uint8_t usart1_tx_buf[TX_BUF_SIZE];
static uint8_t usart1_rx_buf[RX_BUF_SIZE];
static serial_handle_t usart1_handle = {0};
#endif

#if SERIAL_USE_USART2
static uint8_t usart2_tx_buf[TX_BUF_SIZE];
static uint8_t usart2_rx_buf[RX_BUF_SIZE];
static serial_handle_t usart2_handle = {0};
#endif

#if SERIAL_USE_USART3
static uint8_t usart3_tx_buf[TX_BUF_SIZE];
static uint8_t usart3_rx_buf[RX_BUF_SIZE];
static serial_handle_t usart3_handle = {0};
#endif

/* ======================== 内部辅助函数 ======================== */

/**
 * @brief 根据 USART 外设基地址获取对应的串口句柄
 * @param USARTx USART 外设指针（如 USART1, USART2, USART3）
 * @return 对应的 serial_handle_t 指针，若未启用则返回 NULL
 */
static serial_handle_t* get_serial_handle(USART_TypeDef* USARTx)
{
#if SERIAL_USE_USART1
    if (USARTx == USART1) return &usart1_handle;
#endif
#if SERIAL_USE_USART2
    if (USARTx == USART2) return &usart2_handle;
#endif
#if SERIAL_USE_USART3
    if (USARTx == USART3) return &usart3_handle;
#endif
    return NULL;
}

/* ======================== 串口初始化函数 ======================== */

#if SERIAL_USE_USART1
/**
 * @brief 初始化 USART1（PA9-TX, PA10-RX）
 * @param baud 波特率（如 115200）
 */
void Usart1_Init(uint32_t baud)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 TX (PA9) 为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 RX (PA10) 为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 USART1 参数
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    // 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 配置 NVIC（抢占优先级 0，子优先级 0）
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART1_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART1, ENABLE);

    // 初始化环形缓冲区
    usart1_handle.tx_cbuf = circular_buf_init(usart1_tx_buf, TX_BUF_SIZE);
    usart1_handle.rx_cbuf = circular_buf_init(usart1_rx_buf, RX_BUF_SIZE);
    usart1_handle.tx_sending = false;

    CBUF_ASSERT(usart1_handle.tx_cbuf != NULL);
    CBUF_ASSERT(usart1_handle.rx_cbuf != NULL);
}
#endif

#if SERIAL_USE_USART2
/**
 * @brief 初始化 USART2（PA2-TX, PA3-RX）
 * @param baud 波特率（如 115200）
 */
void Usart2_Init(uint32_t baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//ESP8266的复位引脚初始化先放这里，后续应该专门写bsp_esp8266.c进行初始化
	//与EXP8266复位有关
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;				//设置为输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;						//将初始化的Pin脚
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;				//可承载的最大频率
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	GPIO_SetBits(GPIOA,GPIO_Pin_4);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART2_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART2, ENABLE);

    usart2_handle.tx_cbuf = circular_buf_init(usart2_tx_buf, TX_BUF_SIZE);
    usart2_handle.rx_cbuf = circular_buf_init(usart2_rx_buf, RX_BUF_SIZE);
    usart2_handle.tx_sending = false;

    CBUF_ASSERT(usart2_handle.tx_cbuf != NULL);
    CBUF_ASSERT(usart2_handle.rx_cbuf != NULL);
}
#endif

#if SERIAL_USE_USART3
/**
 * @brief 初始化 USART3（PB10-TX, PB11-RX）
 * @param baud 波特率（如 115200 ）
 */
void Usart3_Init(uint32_t baud)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = USART3_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);

    usart3_handle.tx_cbuf = circular_buf_init(usart3_tx_buf, TX_BUF_SIZE);
    usart3_handle.rx_cbuf = circular_buf_init(usart3_rx_buf, RX_BUF_SIZE);
    usart3_handle.tx_sending = false;

    CBUF_ASSERT(usart3_handle.tx_cbuf != NULL);
    CBUF_ASSERT(usart3_handle.rx_cbuf != NULL);
}
#endif



/* ======================== 发送 API ======================== */
/**
 * @brief 向指定串口发送一个字节（非阻塞）
 * @param USARTx 指向 USART 外设的基地址（如 USART1）
 * @param Byte   要发送的字节
 * @retval 0     成功入队
 * @retval -1    USART 未启用或发送缓冲区已满
 */
int Serial_SendByte(USART_TypeDef* USARTx, uint8_t Byte)
{
    serial_handle_t* handle = get_serial_handle(USARTx);
    if (handle == NULL) {
        return -1;
    }

    __disable_irq();
    int ret = circular_buf_put(handle->tx_cbuf, Byte);
    __enable_irq();

    if (ret == 0 && !handle->tx_sending) {
        handle->tx_sending = true;
        USART_ITConfig(USARTx, USART_IT_TXE, ENABLE);
    }
    return ret;
}

/**
 * @brief 发送字节数组（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @param Array  指向待发送数据的指针
 * @param Length 数据长度（字节数）
 * @return       实际成功入队的字节数（可能小于 Length，若缓冲区满）
 */
size_t Serial_SendArray(USART_TypeDef* USARTx, const uint8_t* Array, uint16_t Length)
{
    size_t sent = 0;
    for (size_t i = 0; i < Length; i++) {
        if (Serial_SendByte(USARTx, Array[i]) != 0) break;
        sent++;
    }
    return sent;
}

/**
 * @brief 发送以 '\0' 结尾的字符串（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @param String 指向字符串的指针
 * @return       实际成功发送的字符数
 */
size_t Serial_SendString(USART_TypeDef* USARTx, const char* String)
{
    size_t sent = 0;
    while (*String) {
        if (Serial_SendByte(USARTx, (uint8_t)*String) != 0) break;
        sent++;
        String++;
    }
    return sent;
}

/**
 * @brief 发送固定宽度的十进制数字（右对齐，补前导零）
 * @param USARTx 指向 USART 外设的基地址
 * @param Num    要发送的无符号整数
 * @param Length 总字符宽度（例如：Num=42, Length=4 → "0042"）
 */
void Serial_SendNum(USART_TypeDef* USARTx, uint32_t Num, uint8_t Length)
{
    char buf[10];
    for (int i = Length - 1; i >= 0; i--) {
        buf[i] = (Num % 10) + '0';
        Num /= 10;
    }
    Serial_SendArray(USARTx, (uint8_t*)buf, Length);
}

/**
 * @brief 重定向标准输出（printf）到调试串口
 * @param ch 要输出的字符（ASCII 值）
 * @param f  文件指针（忽略）
 * @return   返回输入字符 ch
 *
 * @note 需在 SerialV2.h 中定义 USART_DEBUG 为目标串口（如 USART1）
 */
int fputc(int ch, FILE* f)
{
    (void)f;
    Serial_SendByte(USART_DEBUG, (uint8_t)ch);
    return ch;
}

/**
 * @brief 格式化打印字符串到指定串口（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @param format 格式化字符串（支持 printf 风格）
 * @param ...    可变参数列表
 *
 * @note 内部缓冲区最大为 100 字节，超长内容将被截断
 */
void Serial_Printf(USART_TypeDef* USARTx, char* format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);
    va_end(arg);
    Serial_SendString(USARTx, String);
	
}


/* ======================== 接收 API ======================== */

/**
 * @brief 检查指定串口是否有可读数据（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @retval true  接收缓冲区非空
 * @retval false 接收缓冲区为空或 USART 未启用
 */
bool Serial_Available(USART_TypeDef* USARTx)
{
    serial_handle_t* handle = get_serial_handle(USARTx);
    CBUF_ASSERT(handle != NULL);

    if (handle == NULL) {
        return false;
    }

    return circular_buf_size(handle->rx_cbuf) > 0;
}

/**
 * @brief 从指定串口读取一个字节（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @retval >=0   成功读取的字节值（0～255）
 * @retval -1    缓冲区为空或 USART 未启用
 *
 * @warning 返回类型为 int，以便用 -1 表示错误
 */
int Serial_ReadByte(USART_TypeDef* USARTx)
{
    serial_handle_t* handle = get_serial_handle(USARTx);
    CBUF_ASSERT(handle != NULL);

    if (handle == NULL) {
        return -1;
    }

    uint8_t data;
    if (circular_buf_get(handle->rx_cbuf, &data) == 0) {
        return (int)data;
    }

    return -1;
}

/**
 * @brief 从指定串口批量读取多个字节（非阻塞）
 * @param USARTx 指向 USART 外设的基地址
 * @param buf    用户提供的接收缓冲区（不可为 NULL）
 * @param len    请求读取的最大字节数
 * @return       实际读取的字节数（0 ≤ return ≤ len）
 */
size_t Serial_ReadArray(USART_TypeDef* USARTx, uint8_t* buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return 0;
    }

    size_t count = 0;
    while (count < len && Serial_Available(USARTx)) {
        buf[count++] = (uint8_t)Serial_ReadByte(USARTx);
    }
    return count;
}

/**
 * @brief 获取指定串口接收缓冲区中的当前数据量
 * @param USARTx 指向 USART 外设的基地址
 * @return       当前接收缓冲区中的有效字节数
 */
size_t Serial_GetRxCount(USART_TypeDef* USARTx)
{
    serial_handle_t* handle = get_serial_handle(USARTx);
    CBUF_ASSERT(handle != NULL);

    if (handle == NULL) {
        return 0;
    }

    return circular_buf_size(handle->rx_cbuf);
}


//未归类函数


cbuf_handle_t BSP_USARTX_GetRxCbuf(USART_TypeDef* USARTx)
{
	serial_handle_t* handle = get_serial_handle(USARTx);
	CBUF_ASSERT(handle != NULL);

    if (handle == NULL) {
        return NULL;
    }	
    return handle->rx_cbuf;
}






/* ======================== 中断处理 ======================== */

/**
 * @brief 通用串口中断服务处理函数（内部使用）
 * @param USARTx 触发中断的 USART 外设基地址
 *
 * @note 该函数由各 USART 的 IRQHandler 调用，处理 RXNE 和 TXE 中断。
 * @warning 在中断上下文中执行，应保持高效、无阻塞。
 */
static void serial_irq_handler(USART_TypeDef* USARTx)
{
    serial_handle_t* handle = get_serial_handle(USARTx);
    if (handle == NULL) {
        return; // 安全防护：防止未启用的 USART 进入中断
    }

    // 接收中断（RXNE）
    if (USART_GetITStatus(USARTx, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USARTx); // 自动清除 RXNE 标志

        __disable_irq();
        circular_buf_put(handle->rx_cbuf, data); // 满则丢弃
        __enable_irq();

        USART_ClearITPendingBit(USARTx, USART_IT_RXNE);
    }

    // 发送中断（TXE）
    if (USART_GetITStatus(USARTx, USART_IT_TXE) != RESET) {
        uint8_t byte;
        if (circular_buf_get(handle->tx_cbuf, &byte) == 0) {
            USART_SendData(USARTx, byte); // 继续发送
        } else {
            USART_ITConfig(USARTx, USART_IT_TXE, DISABLE); // 关闭中断
            handle->tx_sending = false;
        }
        USART_ClearITPendingBit(USARTx, USART_IT_TXE);
    }
}

/* ==================== 中断向量入口 ==================== */

#if SERIAL_USE_USART1
/**
 * @brief USART1 中断服务函数
 */
void USART1_IRQHandler(void)
{
    serial_irq_handler(USART1);
}
#endif

#if SERIAL_USE_USART2
/**
 * @brief USART2 中断服务函数
 */
void USART2_IRQHandler(void)
{
    serial_irq_handler(USART2);
}
#endif

#if SERIAL_USE_USART3
/**
 * @brief USART3 中断服务函数
 */
void USART3_IRQHandler(void)
{
    serial_irq_handler(USART3);
}
#endif

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H
#include "stm32f10x.h"                  // Device header

//(已在 bsp_config.h 中统一管理)
///* ============================================================================ */
///*                             旋转编码器输入配置                               */
///* ============================================================================ */
///** @defgroup Encoder_Config 旋转编码器引脚与中断配置 */
///** @{ */

///* ---------------- 换引脚时需修改以下项(注意要确保引脚后续不会被其他初始覆盖占用) ---------------- */
//#define ENCODER_GPIO_PORT           GPIOA               /*!< 编码器端口（如 GPIOA） */
//#define ENCODER_GPIO_CLK            RCC_APB2Periph_GPIOA/*!< 对应的 GPIO 时钟 */
//#define ENCODER_PIN_A               GPIO_Pin_3          /*!< A 相连接的引脚 */
//#define ENCODER_PIN_B               GPIO_Pin_4          /*!< B 相连接的引脚 */
//#define ENCODER_PIN_SRC_A           GPIO_PinSource3     /*!< A 相 EXTI 映射源（必须与引脚号一致） */
//#define ENCODER_PIN_SRC_B           GPIO_PinSource4     /*!< B 相 EXTI 映射源（必须与引脚号一致） */
//#define ENCODER_EXTI_PORTSOURCE_A   GPIO_PortSourceGPIOA/*!< A 相端口 EXTI 映射源 */
//#define ENCODER_EXTI_PORTSOURCE_B   GPIO_PortSourceGPIOA/*!< B 相端口 EXTI 映射源 */
//#define ENCODER_IRQ_A               EXTI3_IRQn        /*!< A 相对应的中断通道 */
//#define ENCODER_IRQ_B               EXTI4_IRQn        /*!< B 相对应的中断通道 */
//// ⚠️ 同时需在 .c 文件中修改中断服务函数名称！

///* ---------------- 以下无需修改 ---------------- */
//#define ENCODER_EXTI_LINE           (ENCODER_PIN_A | ENCODER_PIN_B) /*!< 使用的 EXTI 线 */
//#define ENCODER_A_EXTI_LINE         ((uint32_t)(ENCODER_PIN_A))     /*!< A 相 EXTI 线（用于状态查询） */
//#define ENCODER_B_EXTI_LINE         ((uint32_t)(ENCODER_PIN_B))     /*!< B 相 EXTI 线（用于状态查询） */

//// #define ENCODER_EXTIA_PRIO       PRE_PRIO_5  /*!< 编码器通道 0 外部中断 */
//// #define ENCODER_EXTIB_PRIO       PRE_PRIO_6  /*!< 编码器通道 1 外部中断 */
//// （中断优先级已在系统配置中统一管理）

///* ⚠️ 注意：务必在 .c 文件中实现与所选 IRQ 匹配的中断服务函数，
// *         例如使用 EXTI9_5_IRQn 时，需实现 EXTI9_5_IRQHandler()
// */

///** @} */

void Encoder_Init(void);
int16_t Encoder_Get(void);
#endif

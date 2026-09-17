/*! ----------------------------------------------------------------------------
 * @file    port.c
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2016-2020 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include <port.h>
//#include <stm32f1xx_hal_conf.h>

/****************************************************************************//**
 *
 *                              APP global variables
 *
 *******************************************************************************/
extern SPI_HandleTypeDef hspi6;


/****************************************************************************//**
 *
 *                  Port private variables and function prototypes
 *
 *******************************************************************************/
static volatile uint32_t signalResetDone;

/* DW IC IRQ handler definition. */
static port_dwic_isr_t port_dwic_isr = NULL;

/****************************************************************************//**
 *
 *                              Time section
 *
 *******************************************************************************/

/* @fn    portGetTickCnt
 * @brief wrapper for to read a SysTickTimer, which is incremented with
 *        CLOCKS_PER_SEC frequency.
 *        The resolution of time32_incr is usually 1/1000 sec.
 * */
__INLINE uint32_t portGetTickCnt(void)
{
    return HAL_GetTick();
}

/* @fn    usleep
 * @brief precise usleep() delay
 * */
#pragma GCC optimize ("O0")
int usleep(uint32_t usec)
{
    unsigned int i;

    usec*=6;
    for(i=0;i<usec;i++)
    {
        __NOP();
    }
    return 0;
}

/* @fn    Sleep
 * @brief Sleep delay in ms using SysTick timer
 * */
__INLINE void Sleep(uint32_t x)
{
    HAL_Delay(x);
}

/****************************************************************************//**
 *
 *                              END OF Time section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                              Configuration section
 *
 *******************************************************************************/

/**
  * @brief  Checks whether the specified IRQn line is enabled or not.
  * @param  IRQn: specifies the IRQn line to check.
  * @return "0" when IRQn is "not enabled" and !0 otherwise
  */
ITStatus EXTI_GetITEnStatus(IRQn_Type IRQn)
{
    /*the former indicates the group number,  while the latter indicates the specific location within the group*/
    return ((NVIC->ISER[0U] &\
        (uint32_t)(1UL << (((uint32_t)(int32_t)IRQn) & 0x1FUL))) == (uint32_t)RESET)?(RESET):(SET);
}

/****************************************************************************//**
 *
 *                          End of configuration section
 *
 *******************************************************************************/

/****************************************************************************//**
 *
 *                          DW IC port section
 *
 *******************************************************************************/

/* @fn      reset_DW IC
 * @brief   DW_RESET pin on DW IC has 2 functions
 *          In general it is output, but it also can be used to reset the digital
 *          part of DW IC by driving this pin low.
 *          Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DWIC(void)
{
	HAL_GPIO_WritePin(DW_RST_GPIO_Port, DW_RST_Pin, 0);

    /* Diagnostic: the old usleep(1) gave only ~100ns of RSTn low at 240MHz,
     * which is below what the DW3000 needs for a reliable reset. Use a
     * millisecond-scale pulse (Qorvo examples use ~1-2ms). */
    Sleep(1);

    HAL_NVIC_DisableIRQ(EXTI0_IRQn);

    //put the pin back to output open-drain (not active)
	HAL_GPIO_WritePin(DW_RST_GPIO_Port, DW_RST_Pin, 1);
    Sleep(2);
}

/* @fn      port_set_dw_ic_spi_slowrate
 * @brief   set 4.06MHz
 *          note: SPI6 kernel clock is PLL2P = 130MHz (see HAL_SPI_MspInit)
 *          The DW3000 only tolerates a slow clock while it boots out of
 *          reset, so this is what must be active for reset_DWIC() and
 *          dwt_checkidlerc().
 * */
void port_set_dw_ic_spi_slowrate(void)
{
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    HAL_SPI_Init(&hspi6);
}

/* @fn      port_set_dw_ic_spi_fastrate
 * @brief   set 32.5MHz
 *          note: SPI6 kernel clock is PLL2P = 130MHz (see HAL_SPI_MspInit)
 *          130MHz/2 = 65MHz exceeds the DW3000 SPI limit of 38MHz, so the
 *          fastest safe divider is 4.
 * */
void port_set_dw_ic_spi_fastrate(void)
{
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    HAL_SPI_Init(&hspi6);
}

/****************************************************************************//**
 *
 *                          End APP port section
 *
 *******************************************************************************/



/****************************************************************************//**
 *
 *                              IRQ section
 *
 *******************************************************************************/

/* @fn      port_DisableEXT_IRQ
 * @brief   wrapper to disable DW_IRQ pin IRQ
 *          in current implementation it disables all IRQ from lines 5:9
 * */
__INLINE void port_DisableEXT_IRQ(void)
{
    NVIC_DisableIRQ(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_EnableEXT_IRQ
 * @brief   wrapper to enable DW_IRQ pin IRQ
 *          in current implementation it enables all IRQ from lines 5:9
 * */
__INLINE void port_EnableEXT_IRQ(void)
{
    NVIC_EnableIRQ(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_GetEXT_IRQStatus
 * @brief   wrapper to read a DW_IRQ pin IRQ status
 * */
__INLINE uint32_t port_GetEXT_IRQStatus(void)
{
    return EXTI_GetITEnStatus(DECAIRQ_EXTI_IRQn);
}

/* @fn      port_CheckEXT_IRQ
 * @brief   wrapper to read DW_IRQ input pin state
 * */
__INLINE uint32_t port_CheckEXT_IRQ(void)
{
    return HAL_GPIO_ReadPin(DW_IRQ_GPIO_Port, DW_IRQ_Pin);
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_dwic_isr()
 *
 * @brief This function is used to install the handling function for DW IC IRQ.
 *
 * NOTE:
 *   - The user application shall ensure that a proper handler is set by calling this function before any DW IC IRQ occurs.
 *   - This function deactivates the DW IC IRQ line while the handler is installed.
 *
 * @param deca_isr function pointer to DW IC interrupt handler to install
 *
 * @return none
 */
void port_set_dwic_isr(port_dwic_isr_t dwic_isr)
{
    /* Check DW IC IRQ activation status. */
    ITStatus en = port_GetEXT_IRQStatus();

    /* If needed, deactivate DW IC IRQ during the installation of the new handler. */
    port_DisableEXT_IRQ();

    port_dwic_isr = dwic_isr;

    if (!en)
    {
        port_EnableEXT_IRQ();
    }
}

/****************************************************************************//**
 *
 *                              END OF IRQ section
 *
 *******************************************************************************/

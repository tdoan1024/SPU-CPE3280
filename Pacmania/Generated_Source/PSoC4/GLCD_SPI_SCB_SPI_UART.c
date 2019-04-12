/***************************************************************************//**
* \file GLCD_SPI_SCB_SPI_UART.c
* \version 3.20
*
* \brief
*  This file provides the source code to the API for the SCB Component in
*  SPI and UART modes.
*
* Note:
*
*******************************************************************************
* \copyright
* Copyright 2013-2016, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "GLCD_SPI_SCB_PVT.h"
#include "GLCD_SPI_SCB_SPI_UART_PVT.h"

/***************************************
*        SPI/UART Private Vars
***************************************/

#if(GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST)
    /* Start index to put data into the software receive buffer.*/
    volatile uint32 GLCD_SPI_SCB_rxBufferHead;
    /* Start index to get data from the software receive buffer.*/
    volatile uint32 GLCD_SPI_SCB_rxBufferTail;
    /**
    * \addtogroup group_globals
    * \{
    */
    /** Sets when internal software receive buffer overflow
    *  was occurred.
    */
    volatile uint8  GLCD_SPI_SCB_rxBufferOverflow;
    /** \} globals */
#endif /* (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST) */

#if(GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST)
    /* Start index to put data into the software transmit buffer.*/
    volatile uint32 GLCD_SPI_SCB_txBufferHead;
    /* Start index to get data from the software transmit buffer.*/
    volatile uint32 GLCD_SPI_SCB_txBufferTail;
#endif /* (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST) */

#if(GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER)
    /* Add one element to the buffer to receive full packet. One byte in receive buffer is always empty */
    volatile uint8 GLCD_SPI_SCB_rxBufferInternal[GLCD_SPI_SCB_INTERNAL_RX_BUFFER_SIZE];
#endif /* (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER) */

#if(GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER)
    volatile uint8 GLCD_SPI_SCB_txBufferInternal[GLCD_SPI_SCB_TX_BUFFER_SIZE];
#endif /* (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER) */


#if(GLCD_SPI_SCB_RX_DIRECTION)
    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartReadRxData
    ****************************************************************************//**
    *
    *  Retrieves the next data element from the receive buffer.
    *   - RX software buffer is disabled: Returns data element retrieved from
    *     RX FIFO. Undefined data will be returned if the RX FIFO is empty.
    *   - RX software buffer is enabled: Returns data element from the software
    *     receive buffer. Zero value is returned if the software receive buffer
    *     is empty.
    *
    * \return
    *  Next data element from the receive buffer. 
    *  The amount of data bits to be received depends on RX data bits selection 
    *  (the data bit counting starts from LSB of return value).
    *
    * \globalvars
    *  GLCD_SPI_SCB_rxBufferHead - the start index to put data into the 
    *  software receive buffer.
    *  GLCD_SPI_SCB_rxBufferTail - the start index to get data from the 
    *  software receive buffer.
    *
    *******************************************************************************/
    uint32 GLCD_SPI_SCB_SpiUartReadRxData(void)
    {
        uint32 rxData = 0u;

    #if (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST)
        uint32 locTail;
    #endif /* (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST) */

        #if (GLCD_SPI_SCB_CHECK_RX_SW_BUFFER)
        {
            if (GLCD_SPI_SCB_rxBufferHead != GLCD_SPI_SCB_rxBufferTail)
            {
                /* There is data in RX software buffer */

                /* Calculate index to read from */
                locTail = (GLCD_SPI_SCB_rxBufferTail + 1u);

                if (GLCD_SPI_SCB_INTERNAL_RX_BUFFER_SIZE == locTail)
                {
                    locTail = 0u;
                }

                /* Get data from RX software buffer */
                rxData = GLCD_SPI_SCB_GetWordFromRxBuffer(locTail);

                /* Change index in the buffer */
                GLCD_SPI_SCB_rxBufferTail = locTail;

                #if (GLCD_SPI_SCB_CHECK_UART_RTS_CONTROL_FLOW)
                {
                    /* Check if RX Not Empty is disabled in the interrupt */
                    if (0u == (GLCD_SPI_SCB_INTR_RX_MASK_REG & GLCD_SPI_SCB_INTR_RX_NOT_EMPTY))
                    {
                        /* Enable RX Not Empty interrupt source to continue
                        * receiving data into software buffer.
                        */
                        GLCD_SPI_SCB_INTR_RX_MASK_REG |= GLCD_SPI_SCB_INTR_RX_NOT_EMPTY;
                    }
                }
                #endif

            }
        }
        #else
        {
            /* Read data from RX FIFO */
            rxData = GLCD_SPI_SCB_RX_FIFO_RD_REG;
        }
        #endif

        return (rxData);
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartGetRxBufferSize
    ****************************************************************************//**
    *
    *  Returns the number of received data elements in the receive buffer.
    *   - RX software buffer disabled: returns the number of used entries in
    *     RX FIFO.
    *   - RX software buffer enabled: returns the number of elements which were
    *     placed in the receive buffer. This does not include the hardware RX FIFO.
    *
    * \return
    *  Number of received data elements.
    *
    * \globalvars
    *  GLCD_SPI_SCB_rxBufferHead - the start index to put data into the 
    *  software receive buffer.
    *  GLCD_SPI_SCB_rxBufferTail - the start index to get data from the 
    *  software receive buffer.
    *
    *******************************************************************************/
    uint32 GLCD_SPI_SCB_SpiUartGetRxBufferSize(void)
    {
        uint32 size;
    #if (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST)
        uint32 locHead;
    #endif /* (GLCD_SPI_SCB_INTERNAL_RX_SW_BUFFER_CONST) */

        #if (GLCD_SPI_SCB_CHECK_RX_SW_BUFFER)
        {
            locHead = GLCD_SPI_SCB_rxBufferHead;

            if(locHead >= GLCD_SPI_SCB_rxBufferTail)
            {
                size = (locHead - GLCD_SPI_SCB_rxBufferTail);
            }
            else
            {
                size = (locHead + (GLCD_SPI_SCB_INTERNAL_RX_BUFFER_SIZE - GLCD_SPI_SCB_rxBufferTail));
            }
        }
        #else
        {
            size = GLCD_SPI_SCB_GET_RX_FIFO_ENTRIES;
        }
        #endif

        return (size);
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartClearRxBuffer
    ****************************************************************************//**
    *
    *  Clears the receive buffer and RX FIFO.
    *
    * \globalvars
    *  GLCD_SPI_SCB_rxBufferHead - the start index to put data into the 
    *  software receive buffer.
    *  GLCD_SPI_SCB_rxBufferTail - the start index to get data from the 
    *  software receive buffer.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_SpiUartClearRxBuffer(void)
    {
        #if (GLCD_SPI_SCB_CHECK_RX_SW_BUFFER)
        {
            /* Lock from component interruption */
            GLCD_SPI_SCB_DisableInt();

            /* Flush RX software buffer */
            GLCD_SPI_SCB_rxBufferHead = GLCD_SPI_SCB_rxBufferTail;
            GLCD_SPI_SCB_rxBufferOverflow = 0u;

            GLCD_SPI_SCB_CLEAR_RX_FIFO;
            GLCD_SPI_SCB_ClearRxInterruptSource(GLCD_SPI_SCB_INTR_RX_ALL);

            #if (GLCD_SPI_SCB_CHECK_UART_RTS_CONTROL_FLOW)
            {
                /* Enable RX Not Empty interrupt source to continue receiving
                * data into software buffer.
                */
                GLCD_SPI_SCB_INTR_RX_MASK_REG |= GLCD_SPI_SCB_INTR_RX_NOT_EMPTY;
            }
            #endif
            
            /* Release lock */
            GLCD_SPI_SCB_EnableInt();
        }
        #else
        {
            GLCD_SPI_SCB_CLEAR_RX_FIFO;
        }
        #endif
    }

#endif /* (GLCD_SPI_SCB_RX_DIRECTION) */


#if(GLCD_SPI_SCB_TX_DIRECTION)
    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartWriteTxData
    ****************************************************************************//**
    *
    *  Places a data entry into the transmit buffer to be sent at the next available
    *  bus time.
    *  This function is blocking and waits until there is space available to put the
    *  requested data in the transmit buffer.
    *
    *  \param txDataByte: the data to be transmitted.
    *   The amount of data bits to be transmitted depends on TX data bits selection 
    *   (the data bit counting starts from LSB of txDataByte).
    *
    * \globalvars
    *  GLCD_SPI_SCB_txBufferHead - the start index to put data into the 
    *  software transmit buffer.
    *  GLCD_SPI_SCB_txBufferTail - start index to get data from the software
    *  transmit buffer.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_SpiUartWriteTxData(uint32 txData)
    {
    #if (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST)
        uint32 locHead;
    #endif /* (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST) */

        #if (GLCD_SPI_SCB_CHECK_TX_SW_BUFFER)
        {
            /* Put data directly into the TX FIFO */
            if ((GLCD_SPI_SCB_txBufferHead == GLCD_SPI_SCB_txBufferTail) &&
                (GLCD_SPI_SCB_SPI_UART_FIFO_SIZE != GLCD_SPI_SCB_GET_TX_FIFO_ENTRIES))
            {
                /* TX software buffer is empty: put data directly in TX FIFO */
                GLCD_SPI_SCB_TX_FIFO_WR_REG = txData;
            }
            /* Put data into TX software buffer */
            else
            {
                /* Head index to put data */
                locHead = (GLCD_SPI_SCB_txBufferHead + 1u);

                /* Adjust TX software buffer index */
                if (GLCD_SPI_SCB_TX_BUFFER_SIZE == locHead)
                {
                    locHead = 0u;
                }

                /* Wait for space in TX software buffer */
                while (locHead == GLCD_SPI_SCB_txBufferTail)
                {
                }

                /* TX software buffer has at least one room */

                /* Clear old status of INTR_TX_NOT_FULL. It sets at the end of transfer when TX FIFO is empty. */
                GLCD_SPI_SCB_ClearTxInterruptSource(GLCD_SPI_SCB_INTR_TX_NOT_FULL);

                GLCD_SPI_SCB_PutWordInTxBuffer(locHead, txData);

                GLCD_SPI_SCB_txBufferHead = locHead;

                /* Check if TX Not Full is disabled in interrupt */
                if (0u == (GLCD_SPI_SCB_INTR_TX_MASK_REG & GLCD_SPI_SCB_INTR_TX_NOT_FULL))
                {
                    /* Enable TX Not Full interrupt source to transmit from software buffer */
                    GLCD_SPI_SCB_INTR_TX_MASK_REG |= (uint32) GLCD_SPI_SCB_INTR_TX_NOT_FULL;
                }
            }
        }
        #else
        {
            /* Wait until TX FIFO has space to put data element */
            while (GLCD_SPI_SCB_SPI_UART_FIFO_SIZE == GLCD_SPI_SCB_GET_TX_FIFO_ENTRIES)
            {
            }

            GLCD_SPI_SCB_TX_FIFO_WR_REG = txData;
        }
        #endif
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartPutArray
    ****************************************************************************//**
    *
    *  Places an array of data into the transmit buffer to be sent.
    *  This function is blocking and waits until there is a space available to put
    *  all the requested data in the transmit buffer. The array size can be greater
    *  than transmit buffer size.
    *
    * \param wrBuf: pointer to an array of data to be placed in transmit buffer. 
    *  The width of the data to be transmitted depends on TX data width selection 
    *  (the data bit counting starts from LSB for each array element).
    * \param count: number of data elements to be placed in the transmit buffer.
    *
    * \globalvars
    *  GLCD_SPI_SCB_txBufferHead - the start index to put data into the 
    *  software transmit buffer.
    *  GLCD_SPI_SCB_txBufferTail - start index to get data from the software
    *  transmit buffer.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_SpiUartPutArray(const uint8 wrBuf[], uint32 count)
    {
        uint32 i;

        for (i=0u; i < count; i++)
        {
            GLCD_SPI_SCB_SpiUartWriteTxData((uint32) wrBuf[i]);
        }
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartGetTxBufferSize
    ****************************************************************************//**
    *
    *  Returns the number of elements currently in the transmit buffer.
    *   - TX software buffer is disabled: returns the number of used entries in
    *     TX FIFO.
    *   - TX software buffer is enabled: returns the number of elements currently
    *     used in the transmit buffer. This number does not include used entries in
    *     the TX FIFO. The transmit buffer size is zero until the TX FIFO is
    *     not full.
    *
    * \return
    *  Number of data elements ready to transmit.
    *
    * \globalvars
    *  GLCD_SPI_SCB_txBufferHead - the start index to put data into the 
    *  software transmit buffer.
    *  GLCD_SPI_SCB_txBufferTail - start index to get data from the software
    *  transmit buffer.
    *
    *******************************************************************************/
    uint32 GLCD_SPI_SCB_SpiUartGetTxBufferSize(void)
    {
        uint32 size;
    #if (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST)
        uint32 locTail;
    #endif /* (GLCD_SPI_SCB_INTERNAL_TX_SW_BUFFER_CONST) */

        #if (GLCD_SPI_SCB_CHECK_TX_SW_BUFFER)
        {
            /* Get current Tail index */
            locTail = GLCD_SPI_SCB_txBufferTail;

            if (GLCD_SPI_SCB_txBufferHead >= locTail)
            {
                size = (GLCD_SPI_SCB_txBufferHead - locTail);
            }
            else
            {
                size = (GLCD_SPI_SCB_txBufferHead + (GLCD_SPI_SCB_TX_BUFFER_SIZE - locTail));
            }
        }
        #else
        {
            size = GLCD_SPI_SCB_GET_TX_FIFO_ENTRIES;
        }
        #endif

        return (size);
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_SpiUartClearTxBuffer
    ****************************************************************************//**
    *
    *  Clears the transmit buffer and TX FIFO.
    *
    * \globalvars
    *  GLCD_SPI_SCB_txBufferHead - the start index to put data into the 
    *  software transmit buffer.
    *  GLCD_SPI_SCB_txBufferTail - start index to get data from the software
    *  transmit buffer.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_SpiUartClearTxBuffer(void)
    {
        #if (GLCD_SPI_SCB_CHECK_TX_SW_BUFFER)
        {
            /* Lock from component interruption */
            GLCD_SPI_SCB_DisableInt();

            /* Flush TX software buffer */
            GLCD_SPI_SCB_txBufferHead = GLCD_SPI_SCB_txBufferTail;

            GLCD_SPI_SCB_INTR_TX_MASK_REG &= (uint32) ~GLCD_SPI_SCB_INTR_TX_NOT_FULL;
            GLCD_SPI_SCB_CLEAR_TX_FIFO;
            GLCD_SPI_SCB_ClearTxInterruptSource(GLCD_SPI_SCB_INTR_TX_ALL);

            /* Release lock */
            GLCD_SPI_SCB_EnableInt();
        }
        #else
        {
            GLCD_SPI_SCB_CLEAR_TX_FIFO;
        }
        #endif
    }

#endif /* (GLCD_SPI_SCB_TX_DIRECTION) */


/*******************************************************************************
* Function Name: GLCD_SPI_SCB_SpiUartDisableIntRx
****************************************************************************//**
*
*  Disables the RX interrupt sources.
*
*  \return
*   Returns the RX interrupt sources enabled before the function call.
*
*******************************************************************************/
uint32 GLCD_SPI_SCB_SpiUartDisableIntRx(void)
{
    uint32 intSource;

    intSource = GLCD_SPI_SCB_GetRxInterruptMode();

    GLCD_SPI_SCB_SetRxInterruptMode(GLCD_SPI_SCB_NO_INTR_SOURCES);

    return (intSource);
}


/*******************************************************************************
* Function Name: GLCD_SPI_SCB_SpiUartDisableIntTx
****************************************************************************//**
*
*  Disables TX interrupt sources.
*
*  \return
*   Returns TX interrupt sources enabled before function call.
*
*******************************************************************************/
uint32 GLCD_SPI_SCB_SpiUartDisableIntTx(void)
{
    uint32 intSourceMask;

    intSourceMask = GLCD_SPI_SCB_GetTxInterruptMode();

    GLCD_SPI_SCB_SetTxInterruptMode(GLCD_SPI_SCB_NO_INTR_SOURCES);

    return (intSourceMask);
}


#if(GLCD_SPI_SCB_SCB_MODE_UNCONFIG_CONST_CFG)
    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_PutWordInRxBuffer
    ****************************************************************************//**
    *
    *  Stores a byte/word into the RX buffer.
    *  Only available in the Unconfigured operation mode.
    *
    *  \param index:      index to store data byte/word in the RX buffer.
    *  \param rxDataByte: byte/word to store.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_PutWordInRxBuffer(uint32 idx, uint32 rxDataByte)
    {
        /* Put data in buffer */
        if (GLCD_SPI_SCB_ONE_BYTE_WIDTH == GLCD_SPI_SCB_rxDataBits)
        {
            GLCD_SPI_SCB_rxBuffer[idx] = ((uint8) rxDataByte);
        }
        else
        {
            GLCD_SPI_SCB_rxBuffer[(uint32)(idx << 1u)]      = LO8(LO16(rxDataByte));
            GLCD_SPI_SCB_rxBuffer[(uint32)(idx << 1u) + 1u] = HI8(LO16(rxDataByte));
        }
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_GetWordFromRxBuffer
    ****************************************************************************//**
    *
    *  Reads byte/word from RX buffer.
    *  Only available in the Unconfigured operation mode.
    *
    *  \return
    *   Returns byte/word read from RX buffer.
    *
    *******************************************************************************/
    uint32 GLCD_SPI_SCB_GetWordFromRxBuffer(uint32 idx)
    {
        uint32 value;

        if (GLCD_SPI_SCB_ONE_BYTE_WIDTH == GLCD_SPI_SCB_rxDataBits)
        {
            value = GLCD_SPI_SCB_rxBuffer[idx];
        }
        else
        {
            value  = (uint32) GLCD_SPI_SCB_rxBuffer[(uint32)(idx << 1u)];
            value |= (uint32) ((uint32)GLCD_SPI_SCB_rxBuffer[(uint32)(idx << 1u) + 1u] << 8u);
        }

        return (value);
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_PutWordInTxBuffer
    ****************************************************************************//**
    *
    *  Stores byte/word into the TX buffer.
    *  Only available in the Unconfigured operation mode.
    *
    *  \param idx:        index to store data byte/word in the TX buffer.
    *  \param txDataByte: byte/word to store.
    *
    *******************************************************************************/
    void GLCD_SPI_SCB_PutWordInTxBuffer(uint32 idx, uint32 txDataByte)
    {
        /* Put data in buffer */
        if (GLCD_SPI_SCB_ONE_BYTE_WIDTH == GLCD_SPI_SCB_txDataBits)
        {
            GLCD_SPI_SCB_txBuffer[idx] = ((uint8) txDataByte);
        }
        else
        {
            GLCD_SPI_SCB_txBuffer[(uint32)(idx << 1u)]      = LO8(LO16(txDataByte));
            GLCD_SPI_SCB_txBuffer[(uint32)(idx << 1u) + 1u] = HI8(LO16(txDataByte));
        }
    }


    /*******************************************************************************
    * Function Name: GLCD_SPI_SCB_GetWordFromTxBuffer
    ****************************************************************************//**
    *
    *  Reads byte/word from the TX buffer.
    *  Only available in the Unconfigured operation mode.
    *
    *  \param idx: index to get data byte/word from the TX buffer.
    *
    *  \return
    *   Returns byte/word read from the TX buffer.
    *
    *******************************************************************************/
    uint32 GLCD_SPI_SCB_GetWordFromTxBuffer(uint32 idx)
    {
        uint32 value;

        if (GLCD_SPI_SCB_ONE_BYTE_WIDTH == GLCD_SPI_SCB_txDataBits)
        {
            value = (uint32) GLCD_SPI_SCB_txBuffer[idx];
        }
        else
        {
            value  = (uint32) GLCD_SPI_SCB_txBuffer[(uint32)(idx << 1u)];
            value |= (uint32) ((uint32) GLCD_SPI_SCB_txBuffer[(uint32)(idx << 1u) + 1u] << 8u);
        }

        return (value);
    }

#endif /* (GLCD_SPI_SCB_SCB_MODE_UNCONFIG_CONST_CFG) */


/* [] END OF FILE */

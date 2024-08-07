/*
        UARTE Library
        Author : Seonguk Jeong
        Created at : 2023-10-12
        Updated at : 2023-10-12
        Target : nRF52 Series
*/
#pragma once
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

#define UARTE_BAUDRATE_9600	        0x00275000
#define UARTE_BAUDRATE_14400	    0x003AF000
#define UARTE_BAUDRATE_19200	    0x004EA000
#define UARTE_BAUDRATE_28800	    0x0075C000
#define UARTE_BAUDRATE_38400	    0x009D0000
#define UARTE_BAUDRATE_57600	    0x00EB0000
#define UARTE_BAUDRATE_76800	    0x013A9000
#define UARTE_BAUDRATE_115200	    0x01D60000
#define UARTE_BAUDRATE_230400	    0x03B00000
#define UARTE_NO_INTERRUPT			0x00
#define UARTE_INTERRUPT_CTS			0x01
#define UARTE_INTERRUPT_NCTS		(0x01 << 1)
#define UARTE_INTERRUPT_RXDRDY		(0x01 << 2)
#define UARTE_INTERRUPT_ENDRX		(0x01 << 4)
#define UARTE_INTERRUPT_TXDRDY		(0x01 << 7)
#define UARTE_INTERRUPT_ENDTX		(0x01 << 8)
#define UARTE_INTERRUPT_ERROR		(0x01 << 9)
#define UARTE_INTERRUPT_RXTO		(0x01 << 17)
#define UARTE_INTERRUPT_RXSTARTED	(0x01 << 19)
#define UARTE_INTERRUPT_TXSTARTED	(0x01 << 20)
#define UARTE_INTERRUPT_TXSTOPPED	(0x01 << 22)
#define UARTE_SHORTS_ENDRX_STARTRX	(1 << 5)
#define UARTE_SHORTS_ENDRX_STOPRX	(1 << 6)

struct UARTE_CONFIG{
    uint32_t PIN_RX;
    uint32_t PIN_TX;
    uint32_t BAUD;
};

class UARTE{
    private:
        static uint8_t UARTE_TX_BUFF[255], UARTE_RX_BUFF[255];
        UARTE_CONFIG CONF;
        void clearEvents();
    public:
        void Init(UARTE_CONFIG CONFIG);
        void Clear();
        void Write(uint8_t *data, uint8_t length);
        void startRx(uint8_t buf_length);
        void setInterrupt(uint32_t interrupt);
        void clearInterrupt(uint32_t interrupt);
        void setShorts(uint32_t shorts);
        void clearShorts(uint32_t shorts);
        void clearRxBuffers();
};
#include "UARTE.h"

// Static variables
uint8_t UARTE::UARTE_TX_BUFF[255] = {0};
uint8_t UARTE::UARTE_RX_BUFF[255] = {0};

void UARTE::Init(UARTE_CONFIG CONFIG){
	// Initialize UARTE
    NRF_UARTE0->ENABLE = 0;
    NRF_UARTE0->PSEL.TXD = CONFIG.PIN_TX;
	NRF_UARTE0->PSEL.RXD = CONFIG.PIN_RX;
	NRF_UARTE0->BAUDRATE = CONFIG.BAUD;
	NRF_UARTE0->CONFIG = 0x00;

	// Set buffer
	NRF_UARTE0->TXD.PTR = (uint32_t)&UARTE_TX_BUFF;
	NRF_UARTE0->RXD.PTR = (uint32_t)&UARTE_RX_BUFF;
	NRF_UARTE0->ENABLE = 8;

	// Copy configuration structure
	memcpy(&CONF, &CONFIG, sizeof(CONFIG));
}

void UARTE::Clear(){
	NRF_UARTE0->ENABLE = 0;
}

void UARTE::Write(uint8_t *data, uint8_t length){
    // for data send
	memset(UARTE_TX_BUFF, 0, 255);
	memcpy(UARTE_TX_BUFF, data, length);
	NRF_UARTE0->TXD.MAXCNT = length;
	NRF_UARTE0->TASKS_STARTTX = 1;
	while(!NRF_UARTE0->EVENTS_ENDTX){}
	NRF_UARTE0->EVENTS_ENDTX = 0;
	NRF_UARTE0->TASKS_STOPTX = 1;
}

void UARTE::startRx(uint8_t buf_length){
	NRF_UARTE0->RXD.MAXCNT = buf_length;
	NRF_UARTE0->TASKS_STARTRX = 1;
}

void UARTE::setInterrupt(uint32_t interrupt){
	NRF_UARTE0->INTENSET = interrupt;
}

void UARTE::clearInterrupt(uint32_t interrupt){
	NRF_UARTE0->INTENCLR = interrupt;
}

void UARTE::setShorts(uint32_t shorts){
	NRF_UARTE0->SHORTS = shorts;
}

void UARTE::clearShorts(uint32_t shorts){
	NRF_UARTE0->SHORTS = 0x00;
}

void UARTE::clearEvents(){
	NRF_UARTE0->EVENTS_CTS = 0;
	NRF_UARTE0->EVENTS_NCTS = 0;
	NRF_UARTE0->EVENTS_RXDRDY = 0;
	NRF_UARTE0->EVENTS_ENDRX = 0;
	NRF_UARTE0->EVENTS_TXDRDY = 0;
	NRF_UARTE0->EVENTS_ENDTX = 0;
	NRF_UARTE0->EVENTS_ERROR = 0;
	NRF_UARTE0->EVENTS_RXTO = 0;
	NRF_UARTE0->EVENTS_RXSTARTED = 0;
	NRF_UARTE0->EVENTS_TXSTARTED = 0;
	NRF_UARTE0->EVENTS_TXSTOPPED = 0;
}

void UARTE::clearRxBuffers(){
	memset(UARTE_RX_BUFF, 0x00, sizeof(UARTE_RX_BUFF));
}
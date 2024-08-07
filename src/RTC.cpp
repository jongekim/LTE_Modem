#include "RTC.h"

void RTC::Init(RTC_CONFIG CONFIG){
    // Low frequency clock 활성화 체크
	if(!(NRF_CLOCK->LFCLKSTAT >> 16) & 0x01){
		NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal;
		NRF_CLOCK->TASKS_LFCLKSTART = 0x01;
		while(!NRF_CLOCK->EVENTS_LFCLKSTARTED){}
	}
    switch(CONFIG.channel){
        case RTC0:
            NRF_RTC0->TASKS_CLEAR = 1;
		    NRF_RTC0->TASKS_STOP = 1;
		    NRF_RTC0->PRESCALER = CONFIG.prescaler;
            break;
        case RTC1:
            NRF_RTC1->TASKS_CLEAR = 1;
		    NRF_RTC1->TASKS_STOP = 1;
		    NRF_RTC1->PRESCALER = CONFIG.prescaler;
            break;
        case RTC2:
            NRF_RTC2->TASKS_CLEAR = 1;
		    NRF_RTC2->TASKS_STOP = 1;
		    NRF_RTC2->PRESCALER = CONFIG.prescaler;
            break;
        default:
            break;
    }
    // 설정 구조체 저장
    memcpy(&CONF, &CONFIG, sizeof(CONFIG));
}

void RTC::Start(){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->TASKS_START = 1;
            break;
        case RTC1:
            NRF_RTC1->TASKS_START = 1;
            break;
        case RTC2:
            NRF_RTC2->TASKS_START = 1;
            break;
        default:
            break;
    }
}
void RTC::Stop(){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->TASKS_STOP = 1;
            NRF_RTC0->TASKS_CLEAR = 1;
            break;
        case RTC1:
            NRF_RTC1->TASKS_STOP = 1;
            NRF_RTC1->TASKS_CLEAR = 1;
            break;
        case RTC2:
            NRF_RTC2->TASKS_STOP = 1;
            NRF_RTC2->TASKS_CLEAR = 1;
            break;
        default:
            break;
    }
}
void RTC::setInterrupt(uint32_t interrupt){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->INTENSET = interrupt;
            NVIC_SetPriority(RTC0_IRQn, 5);
	        NVIC_EnableIRQ(RTC0_IRQn);
            break;
        case RTC1:
            NRF_RTC1->INTENSET = interrupt;
            NVIC_SetPriority(RTC1_IRQn, 5);
	        NVIC_EnableIRQ(RTC1_IRQn);
            break;
        case RTC2:
            NRF_RTC2->INTENSET = interrupt;
            NVIC_SetPriority(RTC2_IRQn, 5);
	        NVIC_EnableIRQ(RTC2_IRQn);
            break;
        default:
            break;
    }
}
void RTC::clearInterrupt(uint32_t interrupt){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->INTENCLR = interrupt;
            break;
        case RTC1:
            NRF_RTC1->INTENCLR = interrupt;
            break;
        case RTC2:
            NRF_RTC2->INTENCLR = interrupt;
            break;
        default:
            break;
    }
}
void RTC::setEvent(uint32_t event){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->EVTENSET = event;
            break;
        case RTC1:
            NRF_RTC1->EVTENSET = event; 
            break;
        case RTC2:
            NRF_RTC2->EVTENSET = event;
            break;
        default:
            break;
    }
}
void RTC::clearEvent(uint32_t event){
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->EVTENCLR = event;
            break;
        case RTC1:
            NRF_RTC1->EVTENCLR = event;
            break;
        case RTC2:
            NRF_RTC2->EVTENCLR = event;
            break;
        default:
            break;
    }
}
void RTC::setCompare(uint8_t comp_no, uint32_t cc){
    if(comp_no > 4){
        return;
    }
    switch(CONF.channel){
        case RTC0:
            NRF_RTC0->CC[comp_no] = cc;
            break;
        case RTC1:
            NRF_RTC1->CC[comp_no] = cc;
            break;
        case RTC2:
            NRF_RTC2->CC[comp_no] = cc;
            break;
        default:
            break;
    }
}
uint32_t RTC::getCounter(){
    static uint32_t output=0;
    switch(CONF.channel){
        case RTC0:
            output = NRF_RTC0->COUNTER;
            break;
        case RTC1:
            output = NRF_RTC1->COUNTER;
            break;
        case RTC2:
            output = NRF_RTC2->COUNTER;
            break;
        default:
            output = NRF_RTC0->COUNTER;
            break;
    }
    return output;
}

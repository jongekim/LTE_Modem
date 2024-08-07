/*
        Real Time Clock(RTC) Library
        Author : Seonguk Jeong
        Created at : 2023-10-12
        Updated at : 2023-10-12
        Target : nRF52 Series
*/
#pragma once
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

#define RTC0 0x00
#define RTC1 0x01
#define RTC2 0x02
#define RTC_COMPARE0 0x00
#define RTC_COMPARE1 0x01
#define RTC_COMPARE2 0x02
#define RTC_COMPARE3 0x03
#define RTC_NO_INTERRUPT 0x00
#define RTC_INTERRUPT_TICK 0x01
#define RTC_INTERRUPT_OVRFLW (0x01 << 1)
#define RTC_INTERRUPT_COMPARE0 (0x01 << 16)
#define RTC_INTERRUPT_COMPARE1 (0x01 << 17)
#define RTC_INTERRUPT_COMPARE2 (0x01 << 18)
#define RTC_INTERRUPT_COMPARE3 (0x01 << 19)
#define RTC_COMPARE0    0x00
#define RTC_COMPARE1    0x01
#define RTC_COMPARE2    0x02
#define RTC_COMPARE3    0x03

struct RTC_CONFIG{
    uint8_t channel;
    uint16_t prescaler;
};


class RTC{
    private:
        void setPrescaler();
        RTC_CONFIG CONF;
    public:
        void Init(RTC_CONFIG CONFIG);
        void Start();
        void Stop();
        void setInterrupt(uint32_t interrupt);
        void clearInterrupt( uint32_t interrupt);
        void setEvent(uint32_t event);
        void clearEvent(uint32_t event);
        void setCompare(uint8_t comp_no, uint32_t cc);
        uint32_t getCounter();
};
#include <zephyr/kernel.h>
#include <EC21V2.h>
#include <UARTE.h>
#include <RTC.h>

/* GPIO Pin 설정 */
#define GPIO_LED_RED        22
#define GPIO_LED_GREEN      23
#define GPIO_LED_BLUE       24
#define GPIO_EXT_PWR        2

/* LTE 모뎀 핀 설정 */
#define GPIO_LTE_PIN_DISABLE    7
#define GPIO_LTE_PIN_WAKEUP     8
#define GPIO_LTE_PIN_PWRKEY     25
#define GPIO_LTE_PIN_RESET      26
#define GPIO_LTE_UART_RX        10
#define GPIO_LTE_UART_TX        9

// External Module Library
UARTE uarte;
EC21 lte;
RTC rtc;
EC21_DEVICE_INFO ec21_device_info;
extern bool getmes;

// External Handler
void EC21_EventHandler(uint16_t EC21_EVENT_TYPE);
void EC21_ErrorHandler(uint16_t EC21_ERROR_CODE);

// 인터럽트 핸들러
void RTC2_IRQHandler();
void GPIOTE_IRQHandler();

int main(void)
{
        // GNSS 초기화
        NRF_GPIO->DIRSET = (1 << GPIO_LED_RED) | (1 << GPIO_LED_GREEN) | (1 << GPIO_LED_BLUE);
        NRF_GPIO->OUTSET = (1 << GPIO_LED_RED) | (1 << GPIO_LED_GREEN) | (1 << GPIO_LED_BLUE);

        // 초기화 함수
        RTC_CONFIG rtc_config = {
             .channel = RTC2,
             .prescaler = 32            // 99.9Hz   
        };
        rtc.Init(rtc_config);
        rtc.setEvent(RTC_INTERRUPT_COMPARE0);
        rtc.setInterrupt(RTC_INTERRUPT_COMPARE0);
        IRQ_DIRECT_CONNECT(RTC2_IRQn, 0, RTC2_IRQHandler, 0);
        irq_enable(RTC2_IRQn);
        rtc.Start();
        
        // NFC핀을 GPIO로 변환
        NRF_UICR->NFCPINS = 0x00;

        // LTE 모뎀 초기화
        UARTE_CONFIG config_uarte = {
                .PIN_RX = GPIO_LTE_UART_RX,
                .PIN_TX = GPIO_LTE_UART_TX,
                .BAUD = UARTE_BAUDRATE_115200,
        };
        uarte.Init(config_uarte);
        EC21_CONFIG config_lte = {
                .uarte = uarte,
                .rtc = rtc,
                .pin_pwr = GPIO_LTE_PIN_PWRKEY,
                .pin_reset = GPIO_LTE_PIN_RESET,
                .pin_wakeup = GPIO_LTE_PIN_WAKEUP,
                .pin_disable = GPIO_LTE_PIN_DISABLE,
        };
        lte.Init(config_lte);
        printk("LTE modem initialized.\n");     

        // RESET핀을 GPIO로 설정
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos; // Write Enable
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        NRF_UICR->PSELRESET[0] = 1 << 31;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        NRF_UICR->PSELRESET[1] = 1 << 31;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos; // Read-only Enable
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        
        // LTE 모뎀 Power On
        lte.Reset();
        k_msleep(200);
        lte.powerOn();

        // GPIOTE Event 및 Interrupt 설정
        // 풀업 설정
        NRF_GPIO->PIN_CNF[21] = 0x0C;
        NRF_GPIOTE->CONFIG[0] = 0x01 | (21 << 8) | (0x02 << 16);
        NRF_GPIOTE->INTENSET = 0x01;
        IRQ_DIRECT_CONNECT(GPIOTE_IRQn, 0, GPIOTE_IRQHandler, 0);
        irq_enable(GPIOTE_IRQn);

        // NVIC 활성화
        NVIC_SetPriority(GPIOTE_IRQn, 5);
        NVIC_EnableIRQ(GPIOTE_IRQn);

        printk("GPIOTE Set.\n");
        while(true){
                k_usleep(10);
        }
        return 0;
}

void EC21_EventHandler(uint16_t EC21_EVENT_TYPE){
        // EC21 이벤트 핸들러
        switch(EC21_EVENT_TYPE){
                case EC21_EVENT_POWERON:
                        // 모뎀의 전원이 켜졌을 때 동작
                        // LED 주황색으로 수정
                        printk("LTE modem power activated.\n");
                        NRF_GPIO->OUTCLR = (1 << GPIO_LED_GREEN);

                        // Echo Off
                        lte.setEcho(false);
                break;
                case EC21_EVENT_ECHO_OFF:
                        // Echo Off 이벤트 동작
                        printk("Echo Off events\n");

                        // 기본 정보 체크
                        ec21_device_info = lte.getDeviceInfo();
                        if(strlen(ec21_device_info.imei) < 1){
                                // IMEI 정보가 없으므로 읽어옴
                                printk("No IMEI information. Query IMEI.\r\n");
                                lte.getImei();
                        }
                break;
                
                case EC21_EVENT_CAPTURE_IMEI:
                        // IMEI값 취득함
                        lte.clearLastQueary();
                        ec21_device_info = lte.getDeviceInfo();
                        if(strlen(ec21_device_info.imei) < 15){
                                break;
                        }
                        printk("IMEI Captured : %s\n", ec21_device_info.imei);
                        
                        // ICCID값 존재 여부 체크
                        if(strlen(ec21_device_info.iccid) < 1){
                                // ICCID 값이 없으므로 읽어옴
                                printk("No ICCID information. Query ICCID.\r\n");
                                lte.getIccid();
                        }
                break;
                case EC21_EVENT_CAPTURE_ICCID:
                        // ICCID값 취득 이벤트
                        lte.clearLastQueary();
                        ec21_device_info = lte.getDeviceInfo();
                        if(strlen(ec21_device_info.iccid) < 20){
                                break;
                        }
                        printk("ICCID Captured : %s\n", ec21_device_info.iccid);

                        // IMSI값 존재 여부 체크
                        if(strlen(ec21_device_info.imsi) < 1){
                                // ICCID 값이 없으므로 읽어옴
                                printk("No IMSI information. Query ICCID.\r\n");
                                lte.getImsi();
                        }
                break;
                case EC21_EVENT_CAPTURE_IMSI:
                        // IMSI값 취득 이벤트
                        lte.clearLastQueary();
                        ec21_device_info = lte.getDeviceInfo();
                        if(strlen(ec21_device_info.iccid) < 20){
                                break;
                        }
                        printk("IMSI Captured : %s\n", ec21_device_info.imsi);
                break;
        }
}
void EC21_ErrorHandler(uint16_t EC21_ERROR_CODE){

}

void RTC2_IRQHandler(){
        // RTC2 IRQHandler
        if(NRF_RTC2->EVENTS_COMPARE[0] == 1){
                NRF_RTC2->EVENTS_COMPARE[0] = 0;
                NRF_UARTE0->TASKS_STOPRX = 1;
        }
}

void GPIOTE_IRQHandler(){
        // GPIOTE 핸들러 

        int cnt = 0;

        if(NRF_GPIOTE->EVENTS_IN[0] == 1){
                NRF_GPIOTE->EVENTS_IN[0] = 0;

                        printk("AT+CFUN=1\n");
                        lte.Query("AT+CFUN=1\r\n");
                        k_busy_wait(2000000); 

                        printk("AT+QGPS=1"\n);
                        lte.Query("AT+QGPS=1\r\n");
                        k_busy_wait(2000000);

                        printk("AT+QCFG=\"band\",F,80A,80A"\n);
                        lte.Query("AT+QCFG=\"band\",F,80A,80A\r\n");
                        k_busy_wait(2000000); 

                        printk("AT+COPS=4,2,\"310410\""\n);
                        lte.Query("AT+COPS=4,2,\"310410\"\r\n");
                        k_busy_wait(2000000);   

                        printk("AT+CREG=2"\n);
                        lte.Query("AT+CREG=2\r\n");
                        k_busy_wait(2000000); 

                        printk("AT+CSCLK=0\n");
                        lte.Query("AT+CSCLK=0\r\n");
                        k_busy_wait(2000000);

                        printk("AT+QCFG=\"powerclass\",1,1");
                        lte.Query("AT+QCFG=\"powerclass\",1,1\r\n");
                        k_busy_wait(2000000);


                        //AWS연결
                        printk("AT+CEREG?\n");
                        lte.Query("AT+CEREG?\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QENG=\"servingcell\"\n");
                        lte.Query("AT+QENG=\"servingcell\"\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QIACT=1\n");
                        lte.Query("AT+QIACT=1\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+CGPADDR=1\n");
                        lte.Query("AT+CGPADDR=1\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QPING=1,\"a3ovp7i619ec6q-ats.iot.us-east-1.amazonaws.com\"\n");
                        lte.Query("AT+QPING=1,\"a3ovp7i619ec6q-ats.iot.us-east-1.amazonaws.com\"\r\n");
                        k_busy_wait(4000000);
                        
                        printk("AT+QFLST=\"*\"\n");
                        lte.Query("AT+QFLST=\"*\"\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QMTCFG=\"recv/mode\",0,0,1\n");
                        lte.Query("AT+QMTCFG=\"recv/mode\",0,0,1\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QMTCFG=\"ssl\",0,1,2\n");
                        lte.Query("AT+QMTCFG=\"ssl\",0,1,2\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"cacert\",2,\"cacert.pem\"\n");
                        lte.Query("AT+QSSLCFG=\"cacert\",2,\"cacert.pem\"\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"clientcert\",2,\"client.pem\"\n");
                        lte.Query("AT+QSSLCFG=\"clientcert\",2,\"client.pem\"\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"clientkey\",2,\"user_key.pem\"\n");
                        lte.Query("AT+QSSLCFG=\"clientkey\",2,\"user_key.pem\"\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"seclevel\",2,2\n");
                        lte.Query("AT+QSSLCFG=\"seclevel\",2,2\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"sslversion\",2,4\n");
                        lte.Query("AT+QSSLCFG=\"sslversion\",2,4\r\n");
                        k_busy_wait(2000000);

                        printk("AT+QSSLCFG=\"ciphersuite\",2,0xFFFF\n");
                        lte.Query("AT+QSSLCFG=\"ciphersuite\",2,0xFFFF\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QSSLCFG=\"ignorelocaltime\",2,1\n");
                        lte.Query("AT+QSSLCFG=\"ignorelocaltime\",2,1\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QMTOPEN=0,\"a3ovp7i619ec6q-ats.iot.us-east-1.amazonaws.com\",8883\n");
                        lte.Query("AT+QMTOPEN=0,\"a3ovp7i619ec6q-ats.iot.us-east-1.amazonaws.com\",8883\r\n");
                        k_busy_wait(2000000);
                        
                        printk("AT+QMTCONN=0,\"smartKit001\"\n");
                        lte.Query("AT+QMTCONN=0,\"smartKit001\"\r\n");
                        k_busy_wait(2000000);

                        //연결 완료
                        printk("AWS Connected\n");

                        printk("AT+QMTPUBEX=0,1,1,0,\"aws/smartKit001/data/report/message\",23\n");
                        lte.Query("AT+QMTPUBEX=0,1,1,0,\"aws/smartKit001/data/report/message\",23\r\n");
                        k_busy_wait(2000000);
                        
                        printk("{\"message\": \"TEST\"}\n");
                        lte.Query("{\"message\": \"TEST\"}\r\n");
                        k_busy_wait(2000000);
                        
                        //송신 테스트 완료
                        printk("AWS message sended\n");

                        // 저전력 모드로 진입
                        //NRF_UART0->TASKS_STOPTX = 1; // UART 송신 중지
                        //NRF_UART0->TASKS_STOPRX = 1; // UART 수신 중지
                        //NRF_UART0->ENABLE = 0; // UART 비활성화

                        printk("Low power mode starting...\n");

                        printk("AT+QCFG=\"powerclass\",1,0\n");
                        lte.Query("AT+QCFG=\"powerclass\",1,0\r\n");
                        k_busy_wait(2000000);

                        NRF_SPI0->ENABLE = 0; // SPI 비활성화
                        printk("SPI unactivated\n");

                        NRF_TWI0->ENABLE = 0; // TWI(I2C) 비활성화
                        printk("TWI unactivated\n");

                        NRF_SAADC->ENABLE = 0; // ADC 비활성화
                        printk("ADC unactivated\n");

                        NRF_TIMER0->TASKS_STOP = 1; // 타이머0 중지
                        printk("Timer0 unactivated\n");

                        NRF_TIMER1->TASKS_STOP = 1; // 타이머1 중지
                        printk("Timer1 unactivated\n");

                        NRF_TIMER2->TASKS_STOP = 1; // 타이머2 중지
                        printk("Timer2 unactivated\n");

                        // 저전력 모드 활성화
                        NRF_POWER->TASKS_LOWPWR = 1;
                        printk("TASK_LOWPWR\n");

                        printk("AT+QGPSEND\n");
                        lte.Query("AT+QGPSEND\r\n");
                        k_busy_wait(2000000);

                        printk("AT+CREG=0\n");
                        lte.Query("AT+CREG=0\r\n");
                        k_busy_wait(2000000);

                        printk("AT+COPS=0\n");
                        lte.Query("AT+COPS=0\r\n");
                        k_busy_wait(2000000);

                        printk("AT+QCFG=\"band\",F,1,1\n");
                        lte.Query("AT+QCFG=\"band\",F,1,1\r\n");
                        k_busy_wait(2000000); 


                         // LTE 및 GNSS 저전력 모드 활성화
                        printk("AT+QSCLK=1\n");
                        lte.Query("AT+QSCLK=1\r\n"); // 저전력 모드 활성화 명령어
                        k_busy_wait(2000000);

                        //lte.Query("AT+QGPSCFG=\"gnssconfig\",0\r\n"); // GNSS 저전력 모드 활성화
                        //k_busy_wait(2000000);
                        //printk("21\n");
                        printk("AT+QGPS=1\n");
                        printk("GNSS init\n");
                        lte.Query("AT+QGPS=1\r\n");//GNSS 초기화
                        k_busy_wait(2000000); 

                        printk("AT+QGPSEND\n");
                        printk("GNSS unactivated\n");
                        lte.Query("AT+QGPSEND\r\n"); // GNSS 비활성화
                        k_busy_wait(2000000);

                        printk("Low power mode on\n");

                        while (cnt<10)
                        {
                                printk("Enter Loop %d\n",cnt);
                                lte.Query("AT+QMTSUB=0,1,\"aws/smartKit001/data/report/message\",1\r\n");
                                k_sleep(K_MSEC(10000));
                                cnt++;
                                if(getmes == true){
                                        printk("Message interrupted!\n");
                                        break;
                                }
                        }
                        getmes = false;
                        
                        cnt = 0;

                        printk("Low Power mode exiting...\n");
                
                     // 외부 신호에 의해 정상 모드로 복귀
                        //NRF_UART0->TASKS_STOPTX = 1; // UART 송신 중지
                        //NRF_UART0->TASKS_STOPRX = 1; // UART 수신 중지
                        //NRF_UART0->ENABLE = 1; // UART 활성화
                        //NRF_UART0->TASKS_STARTRX = 1; // UART 수신 시작
                        //NRF_UART0->TASKS_STARTTX = 1; // UART 송신 시작
                        NRF_SPI0->ENABLE = 1; // SPI 활성화
                        printk("SPI activated\n");
                        
                        NRF_TWI0->ENABLE = 1; // TWI(I2C) 활성화
                        printk("TWI activated\n");

                        NRF_SAADC->ENABLE = 1; // ADC 활성화
                        printk("ADC activated\n");

                        NRF_TIMER0->TASKS_START = 1; // 타이머0 시작
                        printk("Timer0 activated\n");

                        NRF_TIMER1->TASKS_START = 1; // 타이머1 시작
                        printk("Timer1 activated\n");

                        NRF_TIMER2->TASKS_START = 1; // 타이머2 시작
                        printk("Timer2 activated\n");


                        printk("AT+QCFG=\"powerclass\",1,0\n");
                        lte.Query("AT+QCFG=\"powerclass\",1,0\r\n");

                        // LTE 및 GNSS 정상 모드 복귀
                        printk("AT+QSCLK=0\n");
                        lte.Query("AT+QSCLK=0\r\n"); // 저전력 모드 해제 명령어
                        k_busy_wait(2000000);
                        
                        printk("AT+QGPS=1\n");
                        printk("GNSS activated\n");
                        lte.Query("AT+QGPS=1\r\n"); // GNSS 활성화
                        k_busy_wait(2000000);

                        printk("Low power mode off\n");
        }
}
/*
        EC21 Series Module Library V2
        Author : Seonguk Jeong
        Created at : 2024-05-02
        Updated at : 2024-05-02
        Target : Quectel EC21
*/
#include <UARTE.h>
#include <RTC.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <zephyr/sys/timeutil.h>

/* EC21 Modem Event */
#define EC21_EVENT_POWERON                          0x0001
#define EC21_EVENT_POWEROFF                         0x0002
#define EC21_EVENT_SIM_NORMAL                       0x0003
#define EC21_EVENT_SIM_ERROR                        0x0004
#define EC21_EVENT_ECHO_ON                          0x0005
#define EC21_EVENT_ECHO_OFF                         0x0006
#define EC21_EVENT_CAPTURE_ICCID                    0x0007
#define EC21_EVENT_CAPTURE_IMEI                     0x0008
#define EC21_EVENT_CAPTURE_IMSI                     0x0009
#define EC21_EVENT_CAPTURE_SYSTIME                  0x000A
#define EC21_EVENT_QI_ACTIVATED                     0x000B
#define EC21_EVENT_QI_DEACTIVATED                   0x000C
#define EC21_EVENT_QHTTPCFG_SET                     0x000D
#define EC21_EVENT_QHTTPURL_SET                     0x000E
#define EC21_EVENT_QHTTPPOST_SET                    0x000F
#define EC21_EVENT_QHTTPPOST                        0x0010
#define EC21_EVENT_NETWORK_NOT_REGISTERED           0x0011
#define EC21_EVENT_NETWORK_REGISTERED               0x0012
#define EC21_EVENT_NETWORK_REGISTRATION_DENIED      0x0013
#define EC21_EVENT_NETWORK_REGISTRATION_UNKNOWN     0x0014
#define EC21_EVENT_NETWORK_REGISTERED_ROAMING       0x0015
#define EC21_EVENT_FOTA_START                       0x0016
#define EC21_EVENT_FOTA_PROGRESS                    0x0017
#define EC21_EVENT_FOTA_END                         0x0018
#define EC21_EVENT_FOTA_KT                          0x0019
#define EC21_EVENT_APN_CHANGED                      0x0020
#define EC21_EVENT_REMOTE_REBOOT                    0x0FFF

/* EC21 CME Error Code */
#define EC21_CME_ERROR_SIM_NOT_INSERTED             10
#define EC21_CME_ERROR_SIM_PIN_REQUIRED             11
#define EC21_CME_ERROR_SIM_PUK_REQUIRED             13
#define EC21_CME_ERROR_SIM_FAILURE                  14
#define EC21_CME_ERROR_SIM_BUSY                     15
#define EC21_CME_ERROR_SIM_WRONG                    16
#define EC21_HTTP_RESPONSE_403                      403
#define EC21_HTTP_ERROR_GENERAL                     700
#define EC21_CME_ERROR_HTTP_UNKNOWN                 701
#define EC21_CME_ERROR_HTTP_BUSY                    703
#define EC21_CME_ERROR_HTTP_DNS_ERROR               714
#define EC21_CME_ERROR_HTTP_CONNECT_ERROR           716
#define EC21_HTTP_ERROR_TIMEOUT                     702
#define EC21_HTTP_ERROR_BUSY                        703
#define EC21_GENERAL_ERROR                          900

/* EC21 기본 설정 */
#define UARTE_TIMEOUT 50
#define EC21_TIMEOUT 5

struct EC21_CONFIG{
    UARTE uarte;
    RTC rtc;
    uint8_t pin_pwr;
    uint8_t pin_reset;
    uint8_t pin_wakeup;
    uint8_t pin_disable;
};
typedef struct _EC21_DEVICE_INFO{
    char imei[16] = {0};
    char imsi[16] = {0};
    char iccid[21] = {0};
    uint64_t epoch = 0;
    char urls[255] = {0};
}EC21_DEVICE_INFO;
class EC21{
    private:
        static void UARTE_IRQHandler();
        static EC21_CONFIG CONFIG;
        static uint16_t error_code;
        static char lastQuery[255];
        static void parseCommand(const char *cmd);
    public:
        void Init(EC21_CONFIG ec21_config);
        void powerOn();
        void Reset();
        void retryQuery();
        void getSimStatus();
        uint16_t getErrorCode();
        EC21_DEVICE_INFO getDeviceInfo();
        tm getDatetime(uint64_t time_elapsed);
        void getIccid();
        void getImei();
        void getImsi();
        void getNetworkDatetime();
        void setEcho(bool enable);
        void setHttpBody();
        void setHttpUrls(const char *urls, ...);
        void setHttpPost(uint16_t data_length);
        void enablePDP();
        void disablePDP();
        void checkNetworkRegistration();
        void clearDeviceInfo();
        void clearLastQueary();
        static bool Query(const char *cmd, ...);
        
};
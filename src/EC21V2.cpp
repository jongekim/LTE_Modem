#include <EC21V2.h>

/* Global Variables */
static uint8_t rxBuffer[255] = {0};
char EC21::lastQuery[255] = {0};
uint16_t EC21::error_code = 0;

/* Global Functions */
EC21_CONFIG EC21::CONFIG;
EC21_DEVICE_INFO device_info;
extern void EC21_EventHandler(uint16_t EC21_EVENT_TYPE);
extern void EC21_ErrorHandler(uint16_t EC21_ERROR_CODE);

/* EC21 초기화 함수 */
void EC21::Init(EC21_CONFIG ec21_config){
    // 설정값 초기화 및 복사
    memset(&CONFIG, 0x00, sizeof(CONFIG));
    memcpy(&CONFIG, &ec21_config, sizeof(ec21_config));
    
    // GPIO 활성화
    NRF_GPIO->DIRSET = (1 << CONFIG.pin_pwr) | (1 << CONFIG.pin_reset) | (1 << CONFIG.pin_disable) | (1 << CONFIG.pin_wakeup);

    // UARTE 초기화
    CONFIG.uarte.setInterrupt(UARTE_INTERRUPT_RXDRDY | UARTE_INTERRUPT_ENDRX | UARTE_INTERRUPT_RXTO);
    CONFIG.uarte.setShorts(UARTE_SHORTS_ENDRX_STARTRX);
    IRQ_DIRECT_CONNECT(UARTE0_UART0_IRQn, 0, UARTE_IRQHandler, 0);
    irq_enable(UARTE0_UART0_IRQn);
    NRF_UARTE0->RXD.PTR = (uint32_t)rxBuffer;
    CONFIG.uarte.startRx(255);
}

bool EC21::Query(const char *cmd, ...){
    char temp[255] = {0};
    va_list ap;
    va_start(ap, cmd);
    vsprintf(temp, cmd, ap);
    va_end(ap);
    
    // 현재 명령어 저장
    if(strncmp(temp, "A/", 2) != 0){
        memset(lastQuery, 0x00, 255);
        strcpy(lastQuery, temp);
    }
    CONFIG.uarte.Write((uint8_t *)temp, strlen(temp));
    return true;
}

void EC21::powerOn(){
    // 모뎀 파워 온
    NRF_GPIO->OUTSET = (1 << CONFIG.pin_pwr);
    k_busy_wait(200000);
    NRF_GPIO->OUTCLR = (1 << CONFIG.pin_pwr);
}

void EC21::Reset(){
    // 모뎀 리셋
    NRF_GPIO->OUTSET = (1 << CONFIG.pin_reset);
    k_busy_wait(200000);
    NRF_GPIO->OUTCLR = (1 << CONFIG.pin_reset);
}

void EC21::retryQuery(){
    // 직전 명령어 재시도
    Query("A/\r\n");
}

void EC21::getSimStatus(){
    // SIM 상태 쿼리
    Query("AT+CPIN?\r\n");
}

void EC21::UARTE_IRQHandler(){
    if(NRF_UARTE0->EVENTS_RXDRDY == 1){
        NRF_UARTE0->EVENTS_RXDRDY = 0;
        CONFIG.rtc.setCompare(RTC_COMPARE0, CONFIG.rtc.getCounter() + UARTE_TIMEOUT);
    }
    if(NRF_UARTE0->EVENTS_ENDRX == 1){
        NRF_UARTE0->EVENTS_ENDRX = 0;
        //printk("%s\n", rxBuffer); //주석 해제 처리
        parseCommand((const char *)rxBuffer);
        memset(rxBuffer, 0x00, 255);
    }
    if(NRF_UARTE0->EVENTS_RXTO == 1){
        
        NRF_UARTE0->EVENTS_RXTO = 0;
        NRF_UARTE0->TASKS_FLUSHRX = 1;
        printk("EVENTS_RXTO\n");
    }
}

void EC21::parseCommand(const char *cmd){
    // 파싱 함수
    char *temp_ptr, *temp;

    temp = strtok_r((char *)cmd, "\r\n", &temp_ptr);
    while(temp != NULL){
        printk("Temp : %s\r\n", temp);
        
        if(strncmp(temp, "+QIND: \"POWER\",1", 15) == 0){
            // Power On 이벤트
            EC21_EventHandler(EC21_EVENT_POWERON);
        }else if(strncmp(temp, "RDY", 3) == 0){
            // Power On 이벤트
            EC21_EventHandler(EC21_EVENT_POWERON);
        }else if(strncmp(temp, "ATE1", 4) == 0){
            // Echo on 이벤트
            EC21_EventHandler(EC21_EVENT_ECHO_ON);
        }else if(strncmp(temp, "+QIND: PB DONE", 14) == 0){
            // Strange
            //EC21_EventHandler(EC21_EVENT_SIM_NORMAL);
        }else if(strncmp(temp, "ATE0", 4) == 0){
            // Echo on 이벤트
            EC21_EventHandler(EC21_EVENT_ECHO_OFF);
        }else if(strncmp(temp, "+CPIN: READY", 12) == 0){
            // USIM 인식 확인
            EC21_EventHandler(EC21_EVENT_SIM_NORMAL);
        }else if(strncmp(temp, "+CPIN: NOT INSERTED", 19) == 0){
            // USIM 인식 확인
            EC21_EventHandler(EC21_EVENT_POWERON);
        }else if(strncmp(temp, "+CME", 4) == 0){
            // CME 에러 체크
            temp = strtok_r(temp, " ", &temp_ptr);
            temp = strtok_r(NULL, " ", &temp_ptr);
            temp = strtok_r(NULL, "\r\n", &temp_ptr);
            error_code = atoi(temp);
            EC21_ErrorHandler(error_code);
        }else if(strncmp(temp, "+QCCID", 6) == 0){
            // ICCID 값 캡쳐 및 저장
            temp = strtok_r(temp, " ", &temp_ptr);
            temp = strtok_r(NULL, "\r\n", &temp_ptr);
            strcpy(device_info.iccid, temp);
            EC21_EventHandler(EC21_EVENT_CAPTURE_ICCID);
        }else if((strncmp(temp, "+CREG", 5) == 0) && (strncmp(lastQuery, "AT+CREG?", 8) == 0)){
            // 네트워크 등록 체크
            temp = strtok_r(temp, ",", &temp_ptr);
            temp = strtok_r(NULL, "\r\n", &temp_ptr);
            uint8_t netStatus = atoi(temp);
            switch(netStatus){
                case 1:
                    EC21_EventHandler(EC21_EVENT_NETWORK_REGISTERED);
                break;
                case 2:
                    EC21_EventHandler(EC21_EVENT_NETWORK_NOT_REGISTERED);
                break;
                case 3:
                    EC21_EventHandler(EC21_EVENT_NETWORK_REGISTRATION_DENIED);
                break;
                case 4:
                    EC21_EventHandler(EC21_EVENT_NETWORK_REGISTRATION_UNKNOWN);
                break;
                case 5:
                    EC21_EventHandler(EC21_EVENT_NETWORK_REGISTERED_ROAMING);
                break;
                default:
                    EC21_EventHandler(EC21_EVENT_NETWORK_NOT_REGISTERED);
                break;
            }
        }else if(strncmp(temp, "+QLTS", 5) == 0){
            // 시스템 날짜 및 시간 캡쳐
            // 읽은 날짜 및 시간을 Unix epoch 값으로 변환
            struct tm t;
            temp = strtok_r(temp, "\"", &temp_ptr);
            temp = strtok_r(NULL, "/", &temp_ptr);
            t.tm_year = atoi(temp)-1900;
            temp = strtok_r(NULL, "/", &temp_ptr);
            t.tm_mon = atoi(temp)-1;
            temp = strtok_r(NULL, ",", &temp_ptr);
            t.tm_mday = atoi(temp);
            temp = strtok_r(NULL, ":", &temp_ptr);
            t.tm_hour = atoi(temp);
            temp = strtok_r(NULL, ":", &temp_ptr);
            t.tm_min = atoi(temp);
            temp = strtok_r(NULL, "+", &temp_ptr);
            t.tm_sec = atoi(temp);
            device_info.epoch = timeutil_timegm64(&t);
            EC21_EventHandler(EC21_EVENT_CAPTURE_SYSTIME);
        }else if(strncmp(temp, "+QHTTPPOST", 10) == 0){
            // QHTTPPOST Check
            temp = strtok_r(temp, ":", &temp_ptr);
            temp = strtok_r(NULL, ",", &temp_ptr);
            int16_t returnCode = atoi(temp);
            temp = strtok_r(NULL, ",", &temp_ptr);
            int16_t responseCode = atoi(temp);
            temp = strtok_r(NULL, ",", &temp_ptr);
            int16_t contentsLength = atoi(temp);
            if(returnCode == 702){
                // HTTP Timeout
                EC21_ErrorHandler(EC21_HTTP_ERROR_GENERAL);
            }else if(returnCode == 703){
                // HTTP Busy
                EC21_ErrorHandler(EC21_CME_ERROR_HTTP_BUSY);
            }else if(returnCode == 704){
                // HTTP UART Busy
                EC21_ErrorHandler(EC21_HTTP_ERROR_GENERAL);
            }else if(responseCode == 403){
                // 서버 주소 에러
                EC21_ErrorHandler(EC21_HTTP_RESPONSE_403);
            }else if(contentsLength == 8){
                // 재부팅용
                EC21_EventHandler(EC21_EVENT_REMOTE_REBOOT);
            }else{
                EC21_EventHandler(EC21_EVENT_QHTTPPOST);
            }
        }else if(strncmp(temp, "OK", 2) == 0){
            // OK 명령 파싱
            if(strcmp(lastQuery, "AT+QIACT=1\r\n") == 0){
                // Activate PDP Context 명령
                EC21_EventHandler(EC21_EVENT_QI_ACTIVATED);
            }else if(strncmp(lastQuery, "AT+QHTTPCFG", 11) == 0){
                // QHTTPCFG 설정 OK
                EC21_EventHandler(EC21_EVENT_QHTTPCFG_SET);
            }else if(strncmp(lastQuery, "http", 4) == 0){
                // http url 설정
                EC21_EventHandler(EC21_EVENT_QHTTPURL_SET);
            }else if(strncmp(lastQuery, "AT+QIDEACT", 10) == 0){
                Query("AT+QIACT=1\r\n");
            }else{
                //temp = strtok_r(NULL, ":", &temp_ptr);
                //printk("Query OK : %s\r\n", temp);
            }
        }else if(strncmp(temp, "CONNECT", 7) == 0){
            // CONNECT 명령 파싱
            if(strncmp(lastQuery, "AT+QHTTPURL", 11) == 0){
                Query("%s\r\n", device_info.urls);
            }else if(strncmp(lastQuery, "AT+QHTTPPOST", 12) == 0){
                EC21_EventHandler(EC21_EVENT_QHTTPPOST_SET);
            }else{
                //printk("Query Connect : %s", temp);
            }
        }else if(strncmp(temp, "+QIND: \"FOTA\",\"START\"", 21) == 0){
            // FOTA START 관련 처리
            EC21_EventHandler(EC21_EVENT_FOTA_START);
        }else if(strncmp(temp, "+QIND: \"FOTA\",\"END\"", 17) == 0){
            // FOTA END 관련 처리
            EC21_EventHandler(EC21_EVENT_FOTA_END);
        }else if(strncmp(temp, "+QIND: \"FOTA\"", 13) == 0){
            // FOTA PROGRESS 관련 처리
            EC21_EventHandler(EC21_EVENT_FOTA_PROGRESS);
        }else if(strncmp(temp, "+QKTFOTA: 200", 13) == 0){
            // KT FOTA 처리
            EC21_EventHandler(EC21_EVENT_FOTA_KT);
        }else if(strncmp(temp, "+QURC: \"DMC-APN\"", 16) == 0){
            // APN 변경시 동작
            EC21_EventHandler(EC21_EVENT_APN_CHANGED);
        }else if(strncmp(temp, "ERROR", 5) == 0){
            // 에러 동작
            EC21_ErrorHandler(EC21_GENERAL_ERROR);
        }else{
            // 별도의 AT 커맨드 표기 없는 리턴 값 분류
            if((strcmp(lastQuery, "AT+GSN\r\n") == 0) && (strncmp(temp, "86", 2) == 0)){
                // IMEI 쿼리
                temp = strtok_r(temp, "\r\n", &temp_ptr);
                strcpy(device_info.imei, temp);
                EC21_EventHandler(EC21_EVENT_CAPTURE_IMEI);
            }else if((strcmp(lastQuery, "AT+CIMI\r\n") == 0) && (temp[0] >= 0x30 && temp[0] <= 0x39)){
                // IMSI 쿼리
                temp = strtok_r(temp, "\r\n", &temp_ptr);
                strcpy(device_info.imsi, temp);
                EC21_EventHandler(EC21_EVENT_CAPTURE_IMSI);
            }else{
            //printk("TEST : %s", temp);
            }
        }
        temp = strtok_r(NULL, "\r\n", &temp_ptr);
    };
}

uint16_t EC21::getErrorCode(){
    return error_code;
}

EC21_DEVICE_INFO EC21::getDeviceInfo(){
    // ICCID, IMEI, IMSI 값 읽어오는 함수
    return device_info;
}

tm EC21::getDatetime(uint64_t time_elapsed){
    // 시간 계산
    time_t temp = device_info.epoch + time_elapsed;
    struct tm output = {0};
    gmtime_r(&temp, &output);
    return output;
}

void EC21::getIccid(){
    // ICCID 값 쿼리 함수
    Query("AT+QCCID\r\n");
}
void EC21::getImei(){
    // IMEI 값 쿼리 함수
    Query("AT+GSN\r\n");
}
void EC21::getImsi(){
    // IMSI 값 쿼리 함수
    Query("AT+CIMI\r\n");
}

void EC21::getNetworkDatetime(){
    // 시스템 날짜 및 시간 쿼리 함수
    Query("AT+QLTS=2\r\n");
}

void EC21::setEcho(bool enable){
    if(enable){
        Query("ATE1\r\n");
    }else{
        Query("ATE0\r\n");
    }
}

void EC21::setHttpBody(){
    // HTTP Body 형식 지정
    Query("AT+QHTTPCFG=\"contenttype\",4\r\n");
}

void EC21::setHttpUrls(const char *urls, ...){
    // HTTP Endpoint 설정
    memset(device_info.urls, 0x00, 255);
    va_list ap;
    va_start(ap, urls);
    vsprintf(device_info.urls, urls, ap);
    va_end(ap);

    if(strncmp(device_info.urls, "http://", 7) == 0){
        // URL 설정 쿼리
        Query("AT+QHTTPURL=%d\r\n", strlen(device_info.urls));
    }else if(strncmp(device_info.urls, "https://", 8) == 0){
        // URL 설정 쿼리
        Query("AT+QHTTPURL=%d\r\n", strlen(device_info.urls));
    }else{
        printk("Wrong URL set.\r\n");
    }
}

void EC21::setHttpPost(uint16_t data_length){
    // HTTP Post 보내는 함수
    Query("AT+QHTTPPOST=%d,%d,%d\r\n", data_length, EC21_TIMEOUT, EC21_TIMEOUT);
}

void EC21::enablePDP(){
    // Activate PDP Context
    Query("AT+QIACT=1\r\n");
}
void EC21::disablePDP(){
    // Deactivate PDP Context
    Query("AT+QIDEACT=1\r\n");
}

void EC21::checkNetworkRegistration(){
    // 네트워크 등록 체크
    Query("AT+CREG?\r\n");
}

void EC21::clearDeviceInfo(){
    // 장치 정보 초기화
    memset(device_info.iccid, 0x00, sizeof(char *));
    memset(device_info.imei, 0x00, sizeof(char *));
    memset(device_info.imsi, 0x00, sizeof(char *));
    memset(lastQuery, 0x00, 255);
}

void EC21::clearLastQueary(){
    memset(lastQuery, 0x00, 255);
}
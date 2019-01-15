#include "BlRobotics.h"
#include <string.h>

#define BEARER_PROFILE_GPRS "AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r\n"
#define BEARER_PROFILE_APN "AT+SAPBR=3,1,\"APN\",\"%s\"\r\n"
#define QUERY_BEARER "AT+SAPBR=2,1\r\n"
#define OPEN_GPRS_CONTEXT "AT+SAPBR=1,1\r\n"
#define CLOSE_GPRS_CONTEXT "AT+SAPBR=0,1\r\n"
#define HTTP_INIT "AT+HTTPINIT\r\n"
#define HTTP_CID "AT+HTTPPARA=\"CID\",1\r\n"
#define HTTP_PARA "AT+HTTPPARA=\"URL\",\"%s\"\r\n"
#define HTTP_GET "AT+HTTPACTION=0\r\n"
#define HTTP_POST "AT+HTTPACTION=1\n"
#define HTTP_DATA "AT+HTTPDATA=%d,%d\r\n"
#define HTTP_READ "AT+HTTPREAD\r\n"
#define HTTP_CLOSE "AT+HTTPTERM\r\n"
#define HTTP_CONTENT "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r\n"
#define HTTPS_ENABLE "AT+HTTPSSL=1\r\n"
#define HTTPS_DISABLE "AT+HTTPSSL=0\r\n"
#define NORMAL_MODE "AT+CFUN=1,1\r\n"
#define REGISTRATION_STATUS "AT+CREG?\r\n"
#define SIGNAL_QUALITY "AT+CSQ\r\n"
#define READ_VOLTAGE "AT+CBC\r\n"
#define SLEEP_MODE "AT+CSCLK=1\r\n"

#define OK "OK\r\n"
#define DOWNLOAD "DOWNLOAD"
#define HTTP_2XX ",2XX,"
#define HTTPS_PREFIX "https://"
#define CONNECTED "+CREG: 0,5"
#define BEARER_OPEN "+SAPBR: 1,1"
#define NETWORK_LOCATION "AT+CIPGSMLOC=1,1\r\n"
#define INFORM_MODEM "AT\r\n"

int GSMSIM::preInit(void){
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(200);
    digitalWrite(resetPin, LOW);
    delay(2000);
    digitalWrite(resetPin, HIGH);
    delay(3000);
    return 0;
}
void GSMSIM::init(String url)
{
    String response;
    connect();
    post(url,"",response);
}

int GSMSIM::sendCmdAndWaitForResp(String cmd, String resp, unsigned timeout)
{
    listen();
    flush();
    delay(500);
    println(cmd);
    flush();
    int len = resp.length();
    int sum = 0;
    unsigned long timerStart, timerEnd;
    timerStart = millis();
    while (1)
    {
        if (available()>0)
        {
            char c = read();
            // if (c == '#')
            //     write((char)26);
            if (debugMode)
                Serial.print(c);

            sum = (c == resp[sum] || resp[sum] == 'X') ? sum + 1 : 0;
            if (sum == len)
                break;
        }
        timerEnd = millis();
        if (timerEnd - timerStart > timeout)
        {
            return FALSE;
        }
    }

    while (available())
    {
        read();
    }

    return TRUE;
}

Result GSMSIM::setHTTPSession(String url){
    Serial.println(url);
    Result result;
    if (sendCmdAndWaitForResp(HTTP_CID, OK, 2000) == FALSE)
        result = ERROR_HTTP_CID;

    String httpPara = "AT+HTTPPARA=\"URL\",\"" + url + "\"\r\n";
    if (sendCmdAndWaitForResp(httpPara, OK, 2000) == FALSE)
        result = ERROR_HTTP_PARA;

    bool https = url.indexOf(HTTPS_PREFIX) !=-1;
    if (sendCmdAndWaitForResp(https ? HTTPS_ENABLE : HTTPS_DISABLE, OK, 2000) == FALSE) {
        result = https ? ERROR_HTTPS_ENABLE : ERROR_HTTPS_DISABLE;
    }

    if (sendCmdAndWaitForResp(HTTP_CONTENT, OK, 2000) == FALSE)
        result = ERROR_HTTP_CONTENT;

    return result;
}

int GSMSIM::sendATTest(void)
{
    int ret = sendCmdAndWaitForResp(INFORM_MODEM, OK, DEFAULT_TIMEOUT);
    return ret;
}

void GSMSIM::sendEndMark(void)
{
    println((char)26);
}

Result GSMSIM::configureBearer(String apn)
{

    Result result = SUCCESS;

    unsigned int attempts = 0;
    unsigned int MAX_ATTEMPTS = 5;

    sendATTest();

    while (sendCmdAndWaitForResp(REGISTRATION_STATUS, CONNECTED, 2000) != TRUE && attempts <=MAX_ATTEMPTS)
    {
        sendCmdAndWaitForResp(READ_VOLTAGE, OK, 1000);
        sendCmdAndWaitForResp(SIGNAL_QUALITY, OK, 1000);
        attempts++;
        delay(1000 * attempts);
        if (attempts == MAX_ATTEMPTS)
        {
            attempts = 0;
            preInit();
        }
    }

    if (sendCmdAndWaitForResp(BEARER_PROFILE_GPRS, OK, 2000) == FALSE)
        result = ERROR_BEARER_PROFILE_GPRS;

    String httpApn = "AT+SAPBR=3,1,\"APN\",\"" + apn + "\"\r\n";
    if (sendCmdAndWaitForResp(httpApn, OK, 2000) == FALSE)
        result = ERROR_BEARER_PROFILE_APN;

    return result;
}

Result GSMSIM::connect()
{
    Result result = SUCCESS;
    unsigned int attempts = 0;
    unsigned int MAX_ATTEMPTS = 10;
    while (sendCmdAndWaitForResp(QUERY_BEARER, BEARER_OPEN, 2000) == FALSE && attempts < MAX_ATTEMPTS)
    {
        attempts++;
        
        if (sendCmdAndWaitForResp(OPEN_GPRS_CONTEXT, OK, 2000) == FALSE)
        {            
            result = ERROR_OPEN_GPRS_CONTEXT;
        }
        else
        {
            result = SUCCESS;
        }
    }
    if (sendCmdAndWaitForResp(HTTP_INIT, OK, 2000) == FALSE)
        result = ERROR_HTTP_INIT;
    return result;
}

Result GSMSIM::post(String url, String body, String response)
{
    Result result = setHTTPSession(url);
    String httpData = "AT+HTTPDATA=" + (String)body.length() + ",10000\r\n";
    unsigned int delayToDownload = 20000;
    if (sendCmdAndWaitForResp(httpData, DOWNLOAD, 2000) == FALSE)
    {
        result = ERROR_HTTP_DATA;
    }
    println(body);
    if (sendCmdAndWaitForResp(HTTP_POST, HTTP_2XX, delayToDownload) == TRUE)
    {
        // read response
        // println(HTTP_READ);
        // while (available())
        // {
        //     Serial.println(readString());
        // }
        response="Success";
        result = SUCCESS;
        if(debugMode) Serial.println(response);
    }
    else
    {
        result = ERROR_HTTP_POST;
    }

    return result;
}
void GSMSIM::getLocation(String *coor)
{
    listen();
    flush();
    delay(500);
    println(NETWORK_LOCATION);
    flush();
    // println(NETWORK_LOCATION);
    const char *resp = "+CIPGSMLOC: 0,OK";
    int len = strlen(resp);
    int sum = 0;
    int count = 0;
    String str_split[4];
    // String data;
    unsigned long timerStart, timerEnd;
    timerStart = millis();
    while (1)
    {
        if (available() > 0){
            char c = read();
            if (c == resp[sum])
            {
                sum += 1;
            }
            else
            {
                if (sum >13)
                {
                    // data += (String)c;
                    if (c == ','){
                        //str_split[count].trim();
                        count++;
                    }
                    else{
                        str_split[count] += c;
                    }
                }
            }
            if (sum == len)
                break;
        }
        timerEnd = millis();
        if (timerEnd - timerStart > DEFAULT_TIMEOUT)
        {
            break;
        }
    }
    while (available())
    {
        read();
    }
    // uint8_t indexOne;
    // uint8_t indexTwo;
    // indexOne = 0;
    // indexTwo = data.indexOf(",", indexOne);
    // String longi = data.substring(indexOne, indexTwo);
    // indexOne = data.indexOf(",", indexTwo) + 1;
    // indexTwo = data.indexOf(",", indexOne);
    // String lati = data.substring(indexOne, indexTwo);
    // if (coor) *coor = lati + "," + longi;
    if (coor) *coor = str_split[1]+","+str_split[0];
}

Result GSMSIM::disconnect(void) {
    Result result = SUCCESS;
    if (sendCmdAndWaitForResp(CLOSE_GPRS_CONTEXT, OK, 2000) == FALSE)
        result = ERROR_CLOSE_GPRS_CONTEXT;
    if (sendCmdAndWaitForResp(HTTP_CLOSE, OK, 2000) == FALSE)
        result = ERROR_HTTP_CLOSE;
    return result;
}



int GPS::preInit(void){
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, HIGH);
    delay(200);
    digitalWrite(resetPin, LOW);
    delay(2000);
    digitalWrite(resetPin, HIGH);
    delay(3000);
    return TRUE;
}

bool GPS::connect(void)
{
    unsigned long timerStart, timerEnd;
    listen();
    timerStart = millis();
    while(1){
        if (available())
        {
            if (encode(read()))
            {
                return TRUE;
            }
        }
        timerEnd = millis();
        if (timerEnd - timerStart > DEFAULT_TIMEOUT)
        {
            break;
        }
    }
    return FALSE;
}
void GPS::getCoordinate(String * coordinate)
{
    float latitude, longitude;
    f_get_position(&latitude, &longitude);
    if (coordinate)
        *coordinate = String(latitude, 6) + "," + String(longitude, 6);
    if (debugMode)
        Serial.println(*coordinate);
}

void GYRO::start(int addres)
{
    begin();
    beginTransmission(address = addres);
    write(0x6B);
    write(0);
    endTransmission(true);
}
void GYRO::getAcceleration(int16_t * AccX, int16_t * AccY, int16_t * AccZ, int16_t * Temp, int16_t * GyroX, int16_t * GyroY, int16_t * GyroZ)
{
    beginTransmission(address);
    write(0x3B);
    endTransmission(false);
    requestFrom(address, 14, true);
    if (AccX)
        *AccX = AcceX = Wire.read() << 8 | Wire.read();
    if (AccY)
        *AccY = AcceY = Wire.read() << 8 | Wire.read();
    if (AccZ)
        *AccZ = AcceZ = Wire.read() << 8 | Wire.read();
    if (Temp)
        *Temp = Wire.read() << 8 | Wire.read();
    if (GyroX)
        *GyroX = Wire.read() << 8 | Wire.read();
    if (GyroY)
        *GyroY = Wire.read() << 8 | Wire.read();
    if (GyroZ)
        *GyroZ = Wire.read() << 8 | Wire.read();
    if (debugMode)
    {
        Serial.println(*AccX);
        Serial.println(*AccY);
        Serial.println(*AccZ);
        Serial.println(*Temp / 340.00 + 36.53);
        Serial.println(*GyroX);
        Serial.println(*GyroY);
        Serial.println(*GyroZ);
    }
}
void GYRO::getAngle(double *AngleX, double *AngleY, double *AngleZ)
{
    int xAng = map(AcceX, minValue, maxValue, -90, 90);
    int yAng = map(AcceY, minValue, maxValue, -90, 90);
    int zAng = map(AcceZ, minValue, maxValue, -90, 90);
    if (AngleX)
        *AngleX = RAD_TO_DEG * (atan2(-yAng, -zAng) + PI);
    if (AngleY)
        *AngleY = RAD_TO_DEG * (atan2(-xAng, -zAng) + PI);
    if (AngleZ)
        *AngleZ = RAD_TO_DEG * (atan2(-yAng, -xAng) + PI);
}
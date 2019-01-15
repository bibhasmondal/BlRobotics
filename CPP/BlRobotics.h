#ifndef __BlRobotics_H__
#define __BlRobotics_H__
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include "TinyGPS.h"

#define TRUE                    1
#define FALSE                   0
#define DEFAULT_TIMEOUT         5000

enum Result
{
    SUCCESS = 0,
    ERROR_INITIALIZATION = 1,
    ERROR_BEARER_PROFILE_GPRS = 2,
    ERROR_BEARER_PROFILE_APN = 3,
    ERROR_OPEN_GPRS_CONTEXT = 4,
    ERROR_QUERY_GPRS_CONTEXT = 5,
    ERROR_CLOSE_GPRS_CONTEXT = 6,
    ERROR_HTTP_INIT = 7,
    ERROR_HTTP_CID = 8,
    ERROR_HTTP_PARA = 9,
    ERROR_HTTP_GET = 10,
    ERROR_HTTP_READ = 11,
    ERROR_HTTP_CLOSE = 12,
    ERROR_HTTP_POST = 13,
    ERROR_HTTP_DATA = 14,
    ERROR_HTTP_CONTENT = 15,
    ERROR_NORMAL_MODE = 16,
    ERROR_LOW_CONSUMPTION_MODE = 17,
    ERROR_HTTPS_ENABLE = 18,
    ERROR_HTTPS_DISABLE = 19,
    ERROR_INFORM_MODEM = 20,
    ERROR_GETTING_LOC = 21
};

class GSMSIM : public SoftwareSerial{
    public:
        GSMSIM(unsigned int rxPin, unsigned int txPin, unsigned int rstPin, bool debug = TRUE) : SoftwareSerial(txPin, rxPin) {
            resetPin = rstPin;
            debugMode = debug;
        }
        
        int preInit(void);
        void init(String url);
        int sendATTest(void);
        void sendEndMark(void);
        Result configureBearer(String apn);
        Result connect();
        Result disconnect(void);
        Result post(String url, String body, String response);
        void getLocation(String *coor);
    private:
        // String str_split[4];
        bool debugMode;
        unsigned int resetPin;
        int sendCmdAndWaitForResp(String cmd, String resp, unsigned timeout);
        Result setHTTPSession(String url);  
};

class GPS : public SoftwareSerial,public TinyGPS {
    public:
        GPS(unsigned int rxPin, unsigned int txPin, unsigned int rstPin, bool debug = TRUE) : SoftwareSerial(txPin, rxPin) {
            resetPin = rstPin;
            debugMode = debug;
        }
        int preInit(void);
        bool connect();
        void getCoordinate(String *coordinate);
    private:
        bool debugMode;
        unsigned int resetPin;
};
class GYRO : public TwoWire {
    public:
        GYRO(int minVal = 265, int maxVal=402, bool debug = TRUE):TwoWire(){
            minValue=minVal;
            maxValue=maxVal;
            debugMode=debug;
        }
        void start(int addres);
        void getAcceleration(int16_t *AccX,int16_t *AccY,int16_t *AccZ,int16_t *Temp,int16_t *GyroX,int16_t *GyroY,int16_t *GyroZ);
        void getAngle(double *AngleX,double *AngleY,double *AngleZ);
    private:
        bool debugMode;
        int minValue;int maxValue;
        int address;
        int16_t AcceX,AcceY,AcceZ;
};
#endif
#include <BlRobotics.h>
#define url "https://api.blrobotics.in/BRIOT0010/post.php"
#define SN "1000000000"

int condition=1;
int attempt = 0;
unsigned int arduinoResetPin = 2;

unsigned int SIM800l_RX_PIN = 8;
unsigned int SIM800l_TX_PIN = 7;
unsigned int SIM800l_RST_PIN = 10;
GSMSIM gsm(SIM800l_RX_PIN, SIM800l_TX_PIN, SIM800l_RST_PIN);

String coor;
float spd;
//unsigned long _date,_time;
unsigned int NEO_6M_RX_PIN = 3;
unsigned int NEO_6M_TX_PIN = 4;
unsigned int NEO_6M_RST_PIN = 11;
GPS gps(NEO_6M_RX_PIN,NEO_6M_TX_PIN,NEO_6M_RST_PIN,FALSE);

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
double x,y,z;
GYRO gyro(minVal,maxVal,FALSE);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial);
  gsm.begin(9600);
  gps.begin(9600);
  gyro.start(MPU_addr);
  gsm.init(url);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(condition==1)MPU_6050();
  if(condition==2)NEO_6M();
  if(condition==3)SIM800L();
}

void MPU_6050(){
  gyro.getAcceleration(&AcX,&AcY,&AcZ,&Tmp,&GyX,&GyY,&GyZ);
  gyro.getAngle(&x,&y,&z);
  condition=2;
}
void NEO_6M(){
  if(gps.connect()){
    gps.getCoordinate(&coor);//28.5458,77.1703
    spd = gps.f_speed_mps();
    condition=3;
  }
  else{
    coor="";
    condition=3;
  }
}

void SIM800L(){
  gsm.connect();
  int max_count = 0;
  while(coor.length()<=1 && max_count++<=5)gsm.getLocation(&coor);
  String response;
  String rawData;
  if (coor.length()>1){
    rawData="{\"dsn\": \""+String(SN)+"\",\"coor\":\""+coor+"\",\"spd\":\""+String(spd)+"\",\"tmp\":\""+String(Tmp/340.00+36.53)+"\",\"acc\":{\"x\":\""+String(AcX)+"\",\"y\":\""+String(AcY)+"\",\"z\":\""+String(AcZ)+"\"},\"ang\":{\"x\":\""+String(x)+"\",\"y\":\""+String(y)+"\",\"z\":\""+String(z)+"\"}}\r\n";
  }
  else{
    rawData = "";
  }
  if (gsm.post(url,rawData,response) == SUCCESS){
    condition=1;
  }
  else{
    if (attempt++ == 2){
      attempt = 0;
      pinMode(arduinoResetPin, OUTPUT);
      digitalWrite(arduinoResetPin, HIGH);
      delay(300);
      digitalWrite(arduinoResetPin, LOW);
    }
    gsm.preInit();
    gsm.init(url);
  }
}

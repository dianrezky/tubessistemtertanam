#include <WiFi.h>
#include <Arduino.h> 
#include <EEPROM.h>


//INISIALISASI PASSWORD WIFI, HOST DAN PORT

const char* ssid = "Lazer";
const char* password =  "AzzahraC@25092000.";
 
const uint16_t port = 8090;
const char * host = "192.168.1.13";

//INISIALISASI PIN

#define PhSensorPIN 35        //define pin ph
#define TdsSensorPin 34       //define pin tds
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

#define SOUND_SPEED 0.034     //define sound speed in cm/uS
#define CM_TO_INCH 0.393701   //convert cm to inch/uS
 

//INISIALISASI VARIABLE BUAT SENSOR PH

int analogBuffer_ph[SCOUNT];      //membaca nilai analog buffer dan menyimpan
int analogBufferTemp_ph[SCOUNT];  //menyimpan nilai buffer sementara

const int ph_Pin = 35;  //inisialisasi untuk pin ph

float Po = 0;  //inisialisasi untuk hasil baca ph
float PH_step;    
float averageVoltage_ph = 0;  //inisialisasi untuk voltage
int analogBufferIndex_ph = 0; //inisialisasi index buffer pada ph
int copyIndex_ph = 0;         //mengcopy nilai index ph

int nilai_analog_PH;    //inisialisasi untuk baca nilai analog ph
double TeganganPh;      //inisialisasi tegangan pada ph


float PH4 = 3.1;      // Kalibrasi ph
float PH7 = 2.6;      // Kalibrasi ph


//INISIALISASI SENSOR HC

const int trigPin = 5;    //inisialisasi pin untuk triger di HC
const int echoPin = 18;   //inisialisasi pin untuk echo di HC

long duration;            //inisialisasi untuk membaca echo dan mengembalikan waktu tempuh gelombang suara dalam ms
float distanceCm;         //inisialisasi jarak dalam cm
float distanceInch;       //inisialisasi jarak dalam inchi


//INISIALISASI SENSOR TDS

int analogBuffer_tds[SCOUNT];       //menyimpan nilai analog kedalam array dan membacanya dari ADC
int analogBufferTemp_tds[SCOUNT];   //tempat untuk pertukaran atau penyimpanan sementara
int analogBufferIndex_tds = 0;  //inisialisasi index buffer pada ph
int copyIndex_tds = 0;          //mengcopy nilai dari index buffer

int nilai_analog_tds;           //baca nilai analog sensor tds
const int tds_Pin = 34;         //inisialisasi pin untuk tds

float averageVoltage_tds = 0;   //inisialisasi voltage pada tds
float tdsValue = 0;             //inisialisasi hasil tds
float temperature = 25;         // inisialisasi suhu dalam c untuk kompensasi


//fungsi algoritma median filtering sehingga hasil pemvacaan sensor lebih akurat

int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}


void setup(){
 
  Serial.begin(115200);

  //SET PIN

  pinMode(TdsSensorPin,INPUT); //set pin tds

  pinMode(PhSensorPIN,INPUT); //set pin ph

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  
  //check koneksi wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
 
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP()); //menampilkan ip dari esp yg digunakan

 
}

float tds_func(){

  static unsigned long analogSampleTimepoint_tds = millis();

  //fungsi buat baca sensor setiap 20 detik dan membaca nilai analog dari ADC
  if(millis()-analogSampleTimepoint_tds > 10U){     //every 10 milliseconds,read the analog value from the ADC
    analogSampleTimepoint_tds = millis();
    analogBuffer_tds[analogBufferIndex_tds] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex_tds++;
    if(analogBufferIndex_tds == SCOUNT){ 
      analogBufferIndex_tds = 0;
    }
  }   
  
  static unsigned long printTimepoint_tds = millis();


  if(millis()-printTimepoint_tds > 10U){
    printTimepoint_tds = millis();
    for(copyIndex_tds=0; copyIndex_tds<SCOUNT; copyIndex_tds++){
      analogBufferTemp_tds[copyIndex_tds] = analogBuffer_tds[copyIndex_tds];
      
      //baca nilai analog agar lebih stabil dengan menggunakan fungsi getmedian num
      //dan convert ke nilai voltage

      averageVoltage = getMedianNum(analogBufferTemp_tds,SCOUNT) * (float)VREF / 4096.0;
      
      //rumus temperature compensation: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient_tds = 1.0+0.02*(temperature-25.0);    //hitung nilai koefisien kompensasi untuk tds
    
      float compensationVoltage_tds=averageVoltage/compensationCoefficient_tds;  //temperature compensation
      
      //convert nilai voltage ke nilai tds
      tdsValue=((133.42*compensationVoltage*compensationVoltage*compensationVoltage) - (255.86*compensationVoltage*compensationVoltage) + (857.39*compensationVoltage)) *0.5;
      
    }
  }

  return tdsValue;
  
}

float ph_func(){
  
  static unsigned long analogSampleTimepoint_ph = millis();
  if(millis()-analogSampleTimepoint_ph > 15U){     //every 15 milliseconds,read the analog value from the ADC
    analogSampleTimepoint_ph = millis();
    analogBuffer_ph[analogBufferIndex_ph] = analogRead(PhSensorPIN);    //read the analog value and store into the buffer
    analogBufferIndex_ph++;
    if(analogBufferIndex_ph == SCOUNT){ 
      analogBufferIndex_ph = 0;
    }
  }   
  
  static unsigned long printTimepoint_ph = millis();
  if(millis()-printTimepoint_ph > 15U){
    printTimepoint_ph = millis();
    for(copyIndex_ph=0; copyIndex_ph<SCOUNT; copyIndex_ph++){
      analogBufferTemp_ph[copyIndex_ph] = analogBuffer_ph[copyIndex_ph];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage_ph = getMedianNum(analogBufferTemp_ph,SCOUNT) * (float)VREF / 4095.0;
      
      // Perhitungan Ph Sensor
      PH_step = (PH4 - PH7) / 3;
      Po = 7.00 + ( (PH7 - averageVoltage_ph) / PH_step);

    }
  }

  return Po;
  

}

float hc_func(){

  
  //UNTUK MEMBACA NILAI SENSOR HC / INFRARED  

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;


  //menyimpan kedalam satu variabel untuk dikirim data ke python

  result_of_all_sensor = String(tds_func()) + " " + String(ph_func()) + " " + String(distanceCm);

  //menampilkan hasil sensor ke serial monitor
  Serial.print("No: ");
  Serial.println(no);
  Serial.print("Nilai Solid Air:");
  Serial.println(tdsValue);
  Serial.print("Nilai PH dalam Air (Sensor PH):");
  Serial.println(Po, 2);
  Serial.print("Nilai Tinggi Air: ");
  Serial.println(distanceCm);
  Serial.println("");

}

int no=0;

void loop(){

  WiFiClient sensor_value;
    

  if (!sensor_value.connect(host, port)) {
 
      Serial.println("Connection to host failed");
 
      delay(1000);
      return;
  }

  hc_func();

  // value= tds_func();
  // values=ph_func();
  // valuess=hc_func();
 
  sensor_value.print(result_of_all_sensor);
  Serial.println("Disconnecting...");
  sensor_value.stop();

  no++;
    
  delay(3000);
    
}
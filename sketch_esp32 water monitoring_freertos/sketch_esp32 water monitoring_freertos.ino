#include <WiFi.h>
#include <Arduino.h> 
#include <EEPROM.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Some string to print
// const char msg[];

// Task handles
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;
static TaskHandle_t task_3 = NULL;

//Set Queue
static const uint8_t msg_queue_len = 5;
static QueueHandle_t msg_queue;

static const uint8_t msg_variable_len = 5;
static QueueHandle_t msg_variable;

typedef struct Message {
  char body[100];
  float value_save;
} Message;




// char hc_title[100] = "Nilai Ketinggian Air (Sensor HC): ";
// char tds_title[100] = "Nilai Solid Air (Sensor TDS): ";
// char ph_title[100] = "Nilai PH Air (Sensor PH): ";
//*****************************************************************************
// INISIALISASI

//INISIALISASI PASSWORD WIFI, HOST DAN PORT

 
// const uint16_t port = 8090;
// const char * host = "192.168.1.13";

const char* ssid = "studiokost";
const char* password =  "6A95IB76";

// const char* ssid = "Rez";
// const char* password =  "dianrezky";
 
const uint16_t port = 8090;
const char * host = "10.10.103.241";

//INISIALISASI PIN

#define PhSensorPIN 35        //define pin ph
#define TdsSensorPin 34       //define pin tds
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

#define SOUND_SPEED 0.034     //define sound speed in cm/uS
#define CM_TO_INCH 0.393701   //convert cm to inch/uS
 
String result_of_all_sensor;
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



//*****************************************************************************
// Tasks

// Task: print to Serial Terminal with lower priority
void startTask1(void *parameter) { //TDS TASK

  // Print string to Terminal
  while (1) {

    Message msg;

    float get_message_que;
    // Serial.println();
    // for (int i = 0; i < msg_len; i++) {
    //   Serial.print(msg[i]);
    // }
    // Serial.println();

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

        averageVoltage_tds = getMedianNum(analogBufferTemp_tds,SCOUNT) * (float)VREF / 4096.0;
        
        //rumus temperature compensation: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
        float compensationCoefficient_tds = 1.0+0.02*(temperature-25.0);    //hitung nilai koefisien kompensasi untuk tds
      
        float compensationVoltage_tds=averageVoltage_tds/compensationCoefficient_tds;  //temperature compensation
        
        //convert nilai voltage ke nilai tds
        tdsValue=((133.42*compensationVoltage_tds*compensationVoltage_tds*compensationVoltage_tds) - (255.86*compensationVoltage_tds*compensationVoltage_tds) 
                + (857.39*compensationVoltage_tds)) *0.5;
        
      }
    }
    
    msg.value_save = tdsValue;
    strcpy(msg.body, "Nilai Partikel Air (Sensor TDS): ");

    if (xQueueSend(msg_queue, (void *)&msg, 10) != pdTRUE) {
      // Serial.print("Nilai Partikel Air (Sensor TDS): ");
      // Serial.println(get_message_que);
      // Serial.println();
    }


    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

}

// Task: print to Serial Terminal with lower priority
void startTask2(void *parameter) {

  // Print string to Terminal
  while (1) {

    Message msg;

    float get_message_que;
    Serial.println();
    // for (int i = 0; i < msg_len; i++) {
    //   Serial.print(msg[i]);
    // }

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
    msg.value_save = Po;
    strcpy(msg.body, "Nilai PH Air (Sensor PH): ");
    // Try to add item to queue for 10 ticks, fail if queue is full
    if (xQueueSend(msg_queue, (void *)&msg, 10) != pdTRUE) {
      // Serial.print("Nilai PH Air (Sensor PH): ");
      // Serial.println(get_message_que);
      // Serial.println();

      
    }

    Serial.println();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task: print to Serial Terminal with higher priority
void startTask3(void *parameter) {  //SENSOR HC TASK

  Message rcv_msg;

  while (1) {

    // Wait before trying again
    vTaskDelay(500 / portTICK_PERIOD_MS);
  
    // Count number of characters in string
    // int msg_len = strlen(msg);

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

    result_of_all_sensor = String(tdsValue) + " " + String(Po) + " " + String(distanceCm);

    // msg.value_save = distanceCm;
    // strcpy(msg.body, "Nilai Ketinggian Air (Sensor HC): ");

    // if (xQueueSend(msg_queue, (void *)&msg, 10) != pdTRUE) {

    //   Serial.print("Nilai Ketinggian Air (Sensor HC): ");
    //   Serial.println(distanceCm);
    //   Serial.println();
    // }

    if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE) {
      Serial.print(rcv_msg.body);
      Serial.println(rcv_msg.value_save);
      Serial.println();
      Serial.print("Nilai Ketinggian Air (Sensor HC): ");
      Serial.println(distanceCm);
      Serial.println();
      
    }
    else{
      Serial.print("Nilai Ketinggian Air (Sensor HC): ");
      Serial.println(distanceCm);
      Serial.println();
    }




    // Serial.print('*');
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //MENAMPILKAN HASIL PEMBACAAN SEMUA SENSOR KE SERIAL MONITOR

    // Serial.print("No: ");
    // Serial.println(no);
    // Serial.print("Nilai Solid Air:");
    // Serial.println(tdsValue);
    // Serial.print("Nilai PH dalam Air (Sensor PH):");
    // Serial.println(Po, 2);
    // Serial.print("Nilai Tinggi Air: ");
    // Serial.println(distanceCm);
    // Serial.println("");

    // See if there's a message in the queue (do not block)
    
    //Serial.println(result_of_all_sensor); 


  }
}

//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)
 
void setup() {

  // Configure Serial (go slow so we can watch the preemption)
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  //check koneksi wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Not Connected. Please Check SSID and Password...");
  }
  
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP()); //menampilkan ip dari esp yg digunakan

  Serial.println();
  Serial.println("--- Hi Welcome Back ---");
 
  
  //SET PIN

  pinMode(TdsSensorPin,INPUT); //set pin tds

  pinMode(PhSensorPIN,INPUT); //set pin ph

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input


  //SET TASK

  // Print self priority
  Serial.print("Setup and loop task running on core ");
  Serial.print(xPortGetCoreID());
  Serial.print(" with priority ");
  Serial.println(uxTaskPriorityGet(NULL));

  // Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

 // Task to run forever
  xTaskCreatePinnedToCore(startTask1,
                          "Task 1",
                          1024,
                          NULL,
                          1,
                          &task_1,
                          app_cpu);

  // Task to run once with higher priority
  xTaskCreatePinnedToCore(startTask2,
                          "Task 2",
                          1024,
                          NULL,
                          2,
                          &task_2,
                          app_cpu);

  // Task to run once with higher priority
  xTaskCreatePinnedToCore(startTask3,
                          "Task 3",
                          1024,
                          NULL,
                          3,
                          &task_3,
                          app_cpu);
}

    
void loop() {

  WiFiClient sensor_value; //send data to python



  for (int i = 0; i < 2; i++) {
      vTaskSuspend(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);

    // if (xQueueReceive(msg_queue, (void *)&get_message_que, 0) == pdTRUE) {
    //   //Serial.println(get_message_que);
    // }

    // Serial.print(get_message_value_que);
    // Serial.print(": ");
    // Serial.println(get_message_que);

      vTaskResume(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);
  }

  
  if (!sensor_value.connect(host, port)) {
 
    Serial.println("Connection to host failed");
  
    delay(1000);
    return;
  }
  else{


    //Suspend the higher priority task for some intervals
    
    
    
    for (int i = 0; i < 2; i++) {
      vTaskSuspend(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);

    // if (xQueueReceive(msg_queue, (void *)&get_message_que, 0) == pdTRUE) {
    //   //Serial.println(get_message_que);
    // }

    // Serial.print(get_message_value_que);
    // Serial.print(": ");
    // Serial.println(get_message_que);

      vTaskResume(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    sensor_value.print(result_of_all_sensor);
    Serial.println("Disconnecting...");

    sensor_value.stop();


    // Delete the lower priority task
    if (task_1 != NULL) {
      vTaskDelete(task_1);
      task_1 = NULL;
    }

    // Suspend the higher priority task for some intervals
    for (int i = 0; i < 1; i++) {
      vTaskSuspend(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);
      vTaskResume(task_3);
      vTaskDelay(1500 / portTICK_PERIOD_MS);

    }

    if (task_2 != NULL) {
      vTaskDelete(task_2);
      task_2 = NULL;
    }

    

  }
  
  vTaskDelay(1500 / portTICK_PERIOD_MS);
 
}
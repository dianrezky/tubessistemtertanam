#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino.h> 
#include <EEPROM.h>
#include <WebServer.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Some string to print

WebServer server(80);

String page = "";
String data_json = "";

// Task handles
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;
static TaskHandle_t task_3 = NULL;

//Set Queue
static const uint8_t msg_queue_len = 12;
static QueueHandle_t msg_queue;

typedef struct Message {
  char body[100];
  float value_save;
} Message;


//*****************************************************************************
// INISIALISASI

//INISIALISASI PASSWORD WIFI

const char* ssid = "TelkomUniversity";
// const char* password =  "";
 

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

int counter_hc=60;
int i=1;
float average_hc=0;

int saveData_hc[SCOUNT];
int copyData_hc[SCOUNT];
int total_data_hc=0;

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



//*****************************************************************************
// Tasks

// Task: print to Serial Terminal with lower priority
void startTask1(void *parameter) { //TDS TASK

  // Print string to Terminal
  while (1) {

    Message msg;

    float analog_tds = analogRead(TdsSensorPin);    //read the analog value 

    averageVoltage_tds = analog_tds * (float)VREF / 4096.0;
        
    //rumus temperature compensation: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
    float compensationCoefficient_tds = 1.0+0.02*(temperature-25.0);    //hitung nilai koefisien kompensasi untuk tds
      
    float compensationVoltage_tds=averageVoltage_tds/compensationCoefficient_tds;  //temperature compensation
        
    //convert nilai voltage ke nilai tds
    tdsValue=((133.42*compensationVoltage_tds*compensationVoltage_tds*compensationVoltage_tds) - 
              (255.86*compensationVoltage_tds*compensationVoltage_tds) 
              + (857.39*compensationVoltage_tds)) *0.5;    
    
    //Serial.print(tdsValue);
    msg.value_save = tdsValue;
    strcpy(msg.body, "Nilai Partikel Air (Sensor TDS): ");

    if (xQueueSend(msg_queue, (void *)&msg, 10) != pdTRUE) {

    }

    vTaskDelay(15000 / portTICK_PERIOD_MS);
  }

}

// Task: print to Serial Terminal with lower priority
void startTask2(void *parameter) {

  // Print string to Terminal
  while (1) {

    Message msg;

    float analog_ph = analogRead(PhSensorPIN);    //read the analog value and store into the buffer

    averageVoltage_ph = analog_ph * (float)VREF / 4095.0;
        
    // Perhitungan Ph Sensor
    PH_step = (PH4 - PH7) / 3;
    Po = 7.00 + ( (PH7 - averageVoltage_ph) / PH_step);
  

    msg.value_save = Po;
    strcpy(msg.body, "Nilai PH Air (Sensor PH): ");
    // Try to add item to queue for 10 ticks, fail if queue is full
    if (xQueueSend(msg_queue, (void *)&msg, 10) != pdTRUE) {}
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task: print to Serial Terminal with higher priority
void startTask3(void *parameter) {  //SENSOR HC TASK

  Message rcv_msg;

  while (1) {


    //UNTUK MEMBACA NILAI SENSOR HC 

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



     if(i<=60){
      //saveData_hc[i] = distanceCm;

      total_data_hc=total_data_hc+distanceCm;
      Serial.print("Data HC sendiri ");
      Serial.print(": ");
      Serial.println(distanceCm);
      Serial.print("Data ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(total_data_hc);

      i++;


    }
    else if(i>=counter_hc){
      average_hc = total_data_hc/60;

      if(average_hc<=1000){
        Serial.println("HATI - HATI BANJIR");
        i=0;
        average_hc=0;
      }
      else if(average_hc>10){
        Serial.println("MASIH AMAN");
        i=0;
        average_hc=0;
      }
    }

    if (xQueueReceive(msg_queue, (void *)&rcv_msg, 0) == pdTRUE) {
      Serial.print(rcv_msg.body);
      Serial.println(rcv_msg.value_save);
      Serial.println();
    }

    // Serial.print('*');
    vTaskDelay(1000 / portTICK_PERIOD_MS);



  }
}


//*****************************************************************************
// Main (runs as its own task with priority 1 on core 1)
 
void setup() {

  // Configure Serial (go slow so we can watch the preemption)
  Serial.begin(115200);
  
  //SET PIN

  pinMode(TdsSensorPin,INPUT); //set pin tds

  pinMode(PhSensorPIN,INPUT); //set pin ph

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  // tunggu biar keliatan ke serial monitor
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  //check koneksi wifi
  WiFi.begin(ssid);
  // WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Not Connected. Please Check SSID and Password...");
  }
  
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //SET TASK

  // Print self priority
  Serial.print("Setup and loop task running on core ");
  Serial.print(xPortGetCoreID());
  Serial.print(" with priority ");
  Serial.println(uxTaskPriorityGet(NULL));

  // Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(Message));

  Serial.println();
  Serial.println("--- Hi Welcome Back ---");

  server.on("/", []() {
  page = "<head><meta http-equiv=\"refresh\" content=\"3\"></head><center><h1>"+ String(tdsValue) +"</h1> <h3>"+ String(Po) +"</h3> <h4>" + String(distanceCm) + "</h4></center>";
  server.send(200, "text/html", page);
    
  });
  server.begin();
  Serial.println("Web server started!");

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
float save_tds = 0;
void loop() {

  // Suspend the higher priority task for some intervals
  for (int i = 0; i < 5; i++) {
    vTaskSuspend(task_3);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskSuspend(task_2);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskResume(task_3);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    vTaskResume(task_2);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Delete the lower priority task
  if (task_1 != NULL){
    save_tds = tdsValue;
    vTaskDelete(task_1);
    task_1 = NULL;
    if (task_2 != NULL){
      vTaskDelete(task_2);
      task_2 = NULL;
    }
  } 

  server.handleClient();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
 
}
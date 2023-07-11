//library
#include <ESP32Servo.h> //servo
#include "HX711.h"
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "time.h"

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid       = "TGH27";
const char* password   = "isiajasendiri";

long lastTime = 0;
long timerDelay = 500;

//Sensor Berat
const int DOUT = 4;
const int CLK = 2;

//Sensor IR depan
int isObstaclePin = 18; // Pin input
int isObstacle = HIGH;

//Sensor IR Dalam
int ifredPin = 19; // Pin input
int ifred = HIGH;

//Servo
int pin_servo = 13;
int count = 0;
float calibration_factor = 1000; //Hasil Kalibrasi
float units;

Servo servo;
HX711 scale;

//Firebase Database
#define API_KEY "AIzaSyBv8J0KOYXLBxdDJpLjKIJQGkYhopXbGRU"
#define DATABASE_URL "https://nomnomcatv2-default-rtdb.asia-southeast1.firebasedatabase.app"

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
String strValue;
float floatValue;
bool signupOK = false;

//Mengambil waktu
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  servo.attach(pin_servo);
  servo.write(90);
  pinMode(isObstaclePin, INPUT);
  pinMode(ifredPin, INPUT);
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare();  //Reset scale ke 0

  long zero_factor = scale.read_average(10); //Get a baseline reading
    
}

void loop()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  char dayOfWeek[50];
  strftime(dayOfWeek, sizeof(dayOfWeek), "%A", &timeinfo);

  char HourOfDay[50];
  strftime(HourOfDay, sizeof(HourOfDay), "%H", &timeinfo);
  
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  units = scale.get_units();
  isObstacle = digitalRead(isObstaclePin);
  ifred = digitalRead(ifredPin);

  if (units < 0)
    {
      units = 0.00;
    }

  if (units <= 25){
     if (isObstacle == LOW) {
      Serial.println("ada Hewan didepan!!");
      servo.write(0); // posisi servo pada 180 derajat
      Serial.print("Berat: ");
      count = count + 1;
      if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
        sendDataPrevMillis = millis();
        // Write an Int number on the database path test/int
        if (Firebase.RTDB.setInt(&fbdo, "Urxw9gjZjGYojR5bwcAqH0q79G82/" + String(dayOfWeek) + "/" + String(HourOfDay), count)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }
      } 
     } else {
      Serial.println("didepan kosong");
      servo.write(90); // posisi servo pada 0 derajat
     }
  Serial.print("Berat: ");
  Serial.print(units);
  Serial.print(" Gram");
  Serial.println();
  }
  else{
    if (isObstacle == LOW) {
      Serial.println("ada Hewan didepan!!");
     } else {
      Serial.println("didepan kosong");
     }
    Serial.println("Makanan Melebihi Kapasitas!");
    servo.write(0);
  }
  
  if (ifred == LOW) {
    Serial.println("Makanan masih cukup!!");
    delay(500); // jeda 2 detik
  } else {
    Serial.println("Makanan menipis");
    delay(500); // jeda 1 detik
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.getString(&fbdo, "/Urxw9gjZjGYojR5bwcAqH0q79G82/servoStatus")) {
      if (fbdo.dataType() == "String") {
        strValue = fbdo.stringData();
        Serial.print("Read Data : ");
        Serial.println(strValue);
      }
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  }
  
  lastTime = millis();  
  delay(1000);
  Serial.print("Berat: ");
  Serial.print(units);
  Serial.print(" Gram");
  Serial.println();
  Serial.print("Hari : ");
  Serial.println(dayOfWeek);
  Serial.print("Jam : ");
  Serial.println(HourOfDay);
  Serial.println("");
}

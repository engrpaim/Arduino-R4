#include "WiFiS3.h"
#include "WiFiClient.h"
#include "arduino_secrets.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <ArduinoJson.h>
#include <cstring>

#include <SoftwareSerial.h>

SoftwareSerial sim800(8, 7);
//CLASS FOR MATRIX DISPLAY
ArduinoLEDMatrix matrix;

class MessageDisplay {
private:
  ArduinoLEDMatrix& matrix;
  String message;

public:

  MessageDisplay(ArduinoLEDMatrix& m)
    : matrix(m) {}


  void setMessage(String msg) {
    message = msg;
  }

  void displayStatic(const char* text) {
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_4x6);
    matrix.beginText(0, 1, 0x00FF00);
    matrix.println(text);
    matrix.endText();
    matrix.endDraw();
  }
};

MessageDisplay display(matrix);

// COMPONENTS VARIABLES

const int modeSwitch = 3;
const int doortrigger = 4;
const int closeTrigger = 5;
const int temperatureAlarm = 9;



//WIFI CREDENTIALS & METHODS

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int checkOnline = 0;
int status = WL_IDLE_STATUS;
char server[] = "172.17.8.60";

WiFiClient client;

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//HTTP REQUEST RESPONSE

void read_response() {
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}
//------------------------------------------------------------------CONTROL METHODS----------------------------------------------------------------------//
void triggerDoor() {
  delay(1000);
  digitalWrite(doortrigger, LOW);
  Serial.println("Door triggered");
}
void alarmDoor() {
  delay(1000);
  digitalWrite(doortrigger, HIGH);
  Serial.println("Door triggered");
}
void openDoor() {
  digitalWrite(doortrigger, HIGH);
  if (digitalRead(closeTrigger) == LOW) {
    for (int i = 0; i <= 5; i++) {
      Serial.println("Door triggered for " + String(i) + " sec");
      String countDown = "O0" + String(i);
      display.displayStatic(countDown.c_str());
      delay(1000);
      if (digitalRead(closeTrigger) == HIGH) {
        Serial.println("Door Opened");
        break;
      }
      if (i == 5) {
        isDataSaved();
      }
    }
  }
  delay(1000);
}
//send message
void sendSMS(String number, String message) {
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(100);

  sim800.println(message);
  delay(100);

  sim800.write(26);  // ASCII code of CTRL+Z to send the message
  delay(100);

  // Check for response
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}
//RELAY TO SAVED CLOSED

void isDataSaved() {

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  unsigned long startTime = millis();
  while (status != WL_CONNECTED) {
    if (millis() - startTime > 10000) {  // 10 seconds timeout
      Serial.println("Connection timed out!");
      return;
    }
    display.displayStatic("::con");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  printWifiStatus();
  Serial.println("\nStarting connection to server...");
  if (client.connect(server, 80)) {
    Serial.println("Connected to server");
    String postData = "status=CLOSED";
    client.println("POST /refx/controllers/data_results.php HTTP/1.1");
    client.println("Host: 172.17.8.60");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println("Connection: close");
    client.println();
    client.print(postData);

    read_response();


  } else {
    display.displayStatic("Err1");
    Serial.println("Connection to server failed!");
  }
}



// CHECK STATUS EVERY 5 SECONDS

void checkStatus() {
  checkOnline++;
  Serial.println(checkOnline);
  if (checkOnline == 10000) {
    groupMessage("+639217293658", "REF MONITORING IS ONLINE");
    checkOnline = 0;
  }
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }




  if (client.connect(server, 80)) {
    client.println("GET /refx/fetch_status.php HTTP/1.1");
    client.println("Host: 172.17.8.60");
    client.println("Connection: close");
    client.println();

    while (client.connected() || client.available()) {
      if (client.available()) {
        String json = client.readStringUntil('\n');
        StaticJsonDocument<512> doc;
        deserializeJson(doc, json);

        const char* idNumber = doc[0]["Id_Number"];
        const char* status = doc[0]["status"];

        Serial.println(status);
        Serial.println(idNumber);
        if (strcmp(status, "OPEN") == 0) {

          Serial.println("OPEN HELLO WORLD");
          openDoor();
        }

        if (strcmp(status, "NEXT") == 0) {

          Serial.println("NEXT HELLO WORLD");
          if (digitalRead(closeTrigger) == LOW) {
            for (int i = 0; i <= 3; i++) {
              Serial.println("Door triggered for " + String(i) + " sec");
              String countDown = "N0" + String(i);
              display.displayStatic(countDown.c_str());
              delay(1000);

              if (i == 3) {
                triggerDoor();
              }
            }
          }
        }
      }
    }
    client.stop();
  } else {
    Serial.println("No Network");
    for (int j = 0; j < 5; j++) {
      display.displayStatic("WFX");
      delay(1000);
    }
  }

  delay(1000);
}

// send message
void groupMessage(String number, String message) {
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(100);

  sim800.println(message);
  delay(100);

  sim800.write(26);  // ASCII code of CTRL+Z to send the message
  delay(100);

  // Check for response
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}

void getSMSManager(int messageNumber) {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }




  if (client.connect(server, 80)) {
    client.println("GET /refx/fetch_sms.php HTTP/1.1");
    client.println("Host: 172.17.8.60");
    client.println("Connection: close");
    client.println();

    while (client.connected() || client.available()) {
      if (client.available()) {
        String json = client.readStringUntil('\n');
        StaticJsonDocument<1024> doc;
        deserializeJson(doc, json);
        JsonArray array = doc.as<JsonArray>();

        // Loop through the array
        for (int i = 0; i < array.size(); i++) {
          JsonObject obj = array[i];
          String Recipient = obj["person_incharge"].as<String>();
          String number = obj["sim_number"].as<String>();
          String sentNumber = "+63" + number;

          Serial.print("SIM Number: ");
          Serial.println(sentNumber);
          Serial.print("Person in Charge: ");
          Serial.println(Recipient);

          // Check if the current index is less than the array length
          if (i < array.size()) {
            String message;
            switch (messageNumber) {
              case 1:
                message = "---- P5 REFMONITORING ---- \n  \n[{ SYSTEM IS ONLINE }]\n\n SENT TO " + Recipient + " \n\n This SMS is automatically generated by the system \n\n---- AUTOMATION 2024 ----";
                groupMessage(sentNumber, message);
                delay(6000);
                break;
              case 2:
                message = "---- P5 REFMONITORING ---- \n  \n[{ WARNING: TEMPERATURE IS ABOVE TARGET!! }]\n\n SENT TO " + Recipient + " \n\n This SMS is automatically generated by the system \n\n---- AUTOMATION 2024 ----";
                groupMessage(sentNumber, message);
                delay(6000);
                break;
              case 3:

              default:
                // If mode is not recognized, do nothing
                break;
            }
          } else {
            break;  // Break the loop if the index exceeds the size of the array
          }
        }
      }
    }
    client.stop();
  } else {
    Serial.println("No Network");
    display.displayStatic("WFX");
  }
}
void setup() {

  pinMode(doortrigger, OUTPUT);
  pinMode(closeTrigger, INPUT_PULLUP);
  pinMode(modeSwitch, INPUT_PULLUP);
  pinMode(temperatureAlarm, INPUT_PULLUP);
  digitalWrite(doortrigger, HIGH);


  Serial.begin(115200);
  sim800.begin(115200);
  matrix.begin();

  display.displayStatic("REFX ");
  delay(2000);
  display.displayStatic("%AE");
  delay(2000);
  display.displayStatic("%24");
  delay(2000);
  while (!Serial) { ; }
  unsigned long startTime = millis();
  while (status != WL_CONNECTED) {
    if (millis() - startTime > 10000) {
      Serial.println("Connection timed out!");

      return;
    }
    display.displayStatic("EI1");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(4000);

    sim800.println("AT");
    delay(100);
    while (sim800.available()) {
      Serial.write(sim800.read());
    }


    sim800.println("AT+CMGF=1");
    delay(100);

    Serial.println("Sending SMS...");
    getSMSManager(1);
    // sendSMS("+639217293658", "---- P5 REFMONITORING ---- \n  \n[{ SYSTEM IS ONLINE }]\n\n\n This SMS is auto generated by the system \n\n---- AUTOMATION 2024 ----");
    delay(1000);
  }
}

void loop() {
  if (digitalRead(temperatureAlarm) == HIGH) {
    if (status == WL_CONNECTED) {

      if (digitalRead(modeSwitch) == HIGH) {
        checkStatus();
        display.displayStatic("P01");
        Serial.println("P01");
        delay(1000);
      } else {

        display.displayStatic("P02");
        Serial.println("P02 No program.");
        delay(5000);
      }
    } else {
      display.displayStatic("E01");
    }
  } else {
    Serial.println("HIGH TEMPERATURE!!");
    getSMSManager(2);
    delay(1000);
  }
}

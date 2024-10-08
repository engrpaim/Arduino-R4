#include "WiFiS3.h"
#include "WiFiClient.h"
#include "arduino_secrets.h"
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

class MessageDisplay {
  private:
    ArduinoLEDMatrix &matrix; // Reference to the LED matrix
    String message;

  public:
    // Constructor
    MessageDisplay(ArduinoLEDMatrix &m) : matrix(m) {}

    // Method to set the message
    void setMessage(String msg) {
      message = msg;
    }

    // Method to display static text
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
const int modeSwitch = 3; 
const int relayPin = 4; // Pin connected to the relay

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
char server[] = "172.17.8.60";

WiFiClient client;

void isDataSaved() {
  // Ensure WiFi module is available
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  unsigned long startTime = millis();
  while (status != WL_CONNECTED) {
    if (millis() - startTime > 10000) { // 10 seconds timeout
      Serial.println("Connection timed out!");
      return;
    }
    display.displayStatic("er1");
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

    // Read response from server
    read_response();
    
    // Activate the relay after successful data save
    activateRelay();
  } else {
    display.displayStatic("Err1");
    Serial.println("Connection to server failed!");
  }
}

void activateRelay() {
  digitalWrite(relayPin, HIGH); // Activate relay
  delay(1000); // Keep it activated for 1 second
  digitalWrite(relayPin, LOW); // Deactivate relay
  Serial.println("Relay activated.");
}

void read_response() {
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}

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

void setup() {
  // INITIAL DISPLAY
  pinMode(relayPin, OUTPUT); // Set relay pin as OUTPUT
  pinMode(modeSwitch, INPUT_PULLUP);
  
  // Initialize the Serial Monitor
  Serial.begin(115200);
  while (!Serial) { ; }

  matrix.begin();  
  display.displayStatic("REFX");
  delay(2000);
}

void loop() {
  if (digitalRead(modeSwitch) == HIGH) {
    display.displayStatic("S01");
    Serial.println("S01");
    isDataSaved();
    
    if (!client.connected()) {
      Serial.println();
      Serial.println("Disconnecting from server.");
      client.stop();
      while (true);
    }
    delay(1000);
  } else {
    display.displayStatic("S02");
    Serial.println("S02");
    delay(1000);
  }
}

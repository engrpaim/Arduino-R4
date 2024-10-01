#include "WiFiS3.h"
#include "WiFiClient.h"
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
char server[] = "172.17.8.60";

WiFiClient client;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  printWifiStatus();
  Serial.println("\nStarting connection to server...");

  if (client.connect(server, 80)) {
    Serial.println("Connected to server");
    String postData = "param0=OperatorName&param1=12345&time=1";
    client.println("POST /refx/server.php HTTP/1.1");
    client.println("Host: 172.17.8.60");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println("Connection: close");
    client.println();
    client.print(postData);
  } else {
    Serial.println("Connection to server failed!");
  }
}

void read_response() {
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
}

void loop() {
  read_response();
  if (!client.connected()) {
    Serial.println();
    Serial.println("Disconnecting from server.");
    client.stop();
    while (true);
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
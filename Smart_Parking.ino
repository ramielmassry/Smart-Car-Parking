#include <ESP32Servo.h>

#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <WiFi.h>

const char * ssid = "semo";
const char * password = "gogo2345";

#define LINE_FLR_ENTRANCE 25
#define LINE_FLR_EXIT 26
#define LINE_FLR_P_1 33
#define LINE_FLR_P_2 32
#define LINE_FLR_P_3 35
#define LINE_FLR_P_4 34
#define MAIN_GATE 27
#define LINE_FLR_MID 14
#define GARAGE_LIGHT 23
#define FIRE_ALARM 12
#define FLAME_SENSOR 13

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo GATE;
WiFiServer server(80);
String header;
String GARAGE_LIGHTState = "OFF";
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  pinMode(GARAGE_LIGHT, OUTPUT);

  digitalWrite(GARAGE_LIGHT, LOW);

  pinMode(LINE_FLR_ENTRANCE, INPUT);
  pinMode(LINE_FLR_MID, INPUT);
  pinMode(LINE_FLR_EXIT, INPUT);
  pinMode(LINE_FLR_P_1, INPUT);
  pinMode(LINE_FLR_P_2, INPUT);
  pinMode(LINE_FLR_P_3, INPUT);
  pinMode(LINE_FLR_P_4, INPUT);

  pinMode(FLAME_SENSOR, INPUT);
  pinMode(FIRE_ALARM, OUTPUT);

  lcd.begin();
  lcd.backlight();

  GATE.attach(MAIN_GATE);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void fireSystem() {
  if (digitalRead(FLAME_SENSOR) == 0) {
    Serial.println("Flame detected...!");
    digitalWrite(FIRE_ALARM, HIGH);
    delay(1000);
    digitalWrite(FIRE_ALARM, LOW);
    delay(50);
    digitalWrite(GARAGE_LIGHT, LOW);

  } else
    Serial.println("No flame detected. Stay cool");

  delay(1000);
}

void loop() {
  int ENT = !digitalRead(LINE_FLR_ENTRANCE);
  int MID = !digitalRead(LINE_FLR_MID);
  int EXT = !digitalRead(LINE_FLR_EXIT);

  int P1 = !digitalRead(LINE_FLR_P_1); // Parking slots status -- 0=EMPTY , 1=FULL
  int P2 = !digitalRead(LINE_FLR_P_2);
  int P3 = !digitalRead(LINE_FLR_P_3);
  int P4 = !digitalRead(LINE_FLR_P_4);

  int PARKING_STATES[4] = {
    P1,
    P2,
    P3,
    P4
  };

  int SUM_OF_SLOTS = P1 + P2 + P3 + P4;
  int EMPTY_SLOTS = 4 - SUM_OF_SLOTS;

  lcd.setCursor(3, 0);
  lcd.print("WELCOME :)");
  if (EMPTY_SLOTS != 0) {
    lcd.setCursor(0, 1);
    lcd.print(" EMPTY PLACES: ");
    lcd.setCursor(14, 1);
    lcd.print(EMPTY_SLOTS);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("   NO PLACES    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  if ((ENT == 1 || EXT == 1 || MID == 1) && SUM_OF_SLOTS < 4)
    GATE.write(90);
  else if (ENT == 0 || EXT == 0 || MID == 0 || EMPTY_SLOTS == 0)
    GATE.write(0);

  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = ""; // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) { // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c); // print it out the serial monitor
        header += c;
        if (c == '\n') { // if the byte is a newline character
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /23/on") >= 0) {
              Serial.println("GPIO 23 on");
              GARAGE_LIGHTState = "ON";
              digitalWrite(GARAGE_LIGHT, HIGH);
            } else if (header.indexOf("GET /23/off") >= 0) {
              Serial.println("GPIO 23 off");
              GARAGE_LIGHTState = "OFF";
              digitalWrite(GARAGE_LIGHT, LOW);
            }

            String STATES[4];
            String COLORS[4];

            for (int i = 0; i < 4; i++) {
              if (PARKING_STATES[i] == 1) {
                STATES[i] = "FULL";
                COLORS[i] = "RED";
              } else {
                STATES[i] = "EMPTY";
                COLORS[i] = "GREEN";
              }
            }

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            client.println("<body><h1>Smart Car Parking</h1>");

            client.println("<p>GARAGE LIGHTS STATE: " + GARAGE_LIGHTState + "</p>");

            if (GARAGE_LIGHTState == "OFF") {
              client.println("<p><a href=\"/23/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/23/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            client.println("<p style=\"font-size: 1.5em;\">&nbsp;</p>");
            client.println("<table class=\"editorDemoTable\" style=\"width: 264px; margin-left: auto; margin-right: auto;\">");
            client.println("<tbody>");
            client.println("<tr style=\"height: 82.8px;\">");

            client.println("<td style=\"height: 82.8px; width: 107px; background-color:" + COLORS[1] + ";\">");
            client.println("<h3>P2 = " + STATES[1] + "</h3>");

            client.println("</td>");
            client.println("<td style=\"width: 18px; height: 82.8px;\">");
            client.println("<h3>&nbsp;</h3>");
            client.println("</td>");

            client.println("<td style=\"width: 110px; height: 82.8px; background-color:" + COLORS[2] + ";\">");
            client.println("<h3>P3 =" + STATES[2] + "</h3>");

            client.println("</td>");
            client.println("</tr>");
            client.println("<tr style=\"height: 101px;\">");

            client.println("<td style=\"width: 107px; height: 101px; background-color:" + COLORS[0] + ";\">");
            client.println("<h3>P1 = " + STATES[0] + "</h3>");

            client.println("</td>");
            client.println("<td style=\"width: 18px; height: 101px;\">");
            client.println("<h3>&nbsp;</h3>");
            client.println("</td>");

            client.println("<td style=\"width: 110px; height: 101px; background-color:" + COLORS[3] + ";\">");
            client.println("<h3>P4 = " + STATES[3] + "</h3>");
            client.println("</td>");
            client.println("</tr>");
            client.println("</tbody>");
            client.println("</table>");

            client.println(" <meta http-equiv=\"refresh\" content=\"1\">");
            client.println("</body></html>");

            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

  fireSystem();
}

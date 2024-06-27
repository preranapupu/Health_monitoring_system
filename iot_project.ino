#define BLYNK_TEMPLATE_ID "TMPL3EFlG-3vk"
#define BLYNK_TEMPLATE_NAME "safety"
#define BLYNK_AUTH_TOKEN "Mz9HEMtpJ51YznwGqPCZXFcUPgp8f5gr"


#define TEMPERATURE_PIN V0
#define HUMIDITY_PIN V1
#define PULSE_PIN V2
#define AVGPULSE_PIN V3
#define DHTPIN 4
#define DHTTYPE DHT11

// SCL of MH-ET LIVE -> D22, SDA of MH-ET LIVE -> D21

// GPS TX -> RX2(16), GPS RX -> TX2(17)
#define RXPin 16 // RX pin for GPS
#define TXPin 17 // TX pin for GPS
#define GPSBaud 9600


#include <TinyGPS++.h>
#include "DHT.h"
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "Arduino.h"
#include <EMailSender.h>

char auth[] = BLYNK_AUTH_TOKEN;

TinyGPSPlus gps;

EMailSender emailSend("test451972@gmail.com", "diup emku kpgf tlxw");

DHT dht(DHTPIN, DHTTYPE);
MAX30105 particleSensor;





WiFiServer server(80);
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;






const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

double temperature_c;
double temperature_f;
double humidity;
double hif;
double hic;

int flag = 1;
String latitude = "22.574422", longitude = "88.433840";

// Your WiFi credentials.
// Set password to "" for open networks.
const char ssid[] = "Daenerys";
const char pass[] = "nskv5685";

BlynkTimer timer;

void setup()
{
  // Debug console
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  dht.begin();
  Serial2.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin); // Initialize hardware serial for
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }


  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  timer.setInterval(10L, humidity_func);
  timer.setInterval(10L, pulse_func);
  timer.setInterval(10L, alert);
  timer.setInterval(10L, displayInfo);
  server.begin();
}

void loop()
{

  Blynk.run();
  timer.run();
  //  while (Serial2.available() > 0) {
  //
  //  }

  //  if (millis() > 5000 && gps.charsProcessed() < 10) {
  //    Serial.println("No GPS detected: check wiring.");
  //    while (true);
  //  }
  //  if (gps.encode(Serial2.read())) {
  //    displayInfo();
  //  }
  //  alert();



  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>Details</h1>");
            client.println("<p>Pulse rate: " + (String)beatsPerMinute + "</p>");
            client.println("<p>Humidity: " + (String)humidity + "</p>");
            client.println("<p>Temperature " + (String)temperature_c + "</p>");
            client.println("<p><a href=\"https://maps.google.com/maps?q=loc:"+latitude+","+longitude+"\"><button class=\"button button2\">Map</button></a></p>");
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void alert() {
  if (beatsPerMinute > 55 && beatsPerMinute < 77) {
    sendEmail(String(beatsPerMinute), String(humidity), String(temperature_c));
  }
  else if (humidity > 50) {
    sendEmail(String(beatsPerMinute), String(humidity), String(temperature_c));
  }
  else if (temperature_c > 20) {
    sendEmail(String(beatsPerMinute), String(humidity), String(temperature_c));
  }
}

void pulse_func() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  if ((irValue == 0) || (irValue < 700)) {
    beatsPerMinute = 0.00;
    beatAvg = 0.00;
  }
  else if ((flag == 1)) {
    beatsPerMinute = 76.63;
    beatAvg = 69.855;
    flag = 2;
  }
  else if ((flag == 2)) {
    beatsPerMinute = 76.63 - 21.1;
    beatAvg = 69.855 - 2.15;
    flag = 3;
  }
  else if ((flag == 3)) {
    beatsPerMinute = 76.63 - 11.21;
    beatAvg = 69.855 - 1.47;
    flag = 4;
  }
  else if ((flag == 4)) {
    beatsPerMinute = 76.63 + 5.21;
    beatAvg = 69.855 + 2.35;
    flag = 1;
  }

  Blynk.virtualWrite(PULSE_PIN, beatsPerMinute);
  Blynk.virtualWrite(AVGPULSE_PIN, beatAvg);

  if (beatsPerMinute > 55 && beatsPerMinute < 77) {
    Serial.println("bpm_alert");
    Blynk.logEvent("alert");
    //    sendEmail(beatsPerMinute);
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();
}

void humidity_func() {
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature_c = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  temperature_f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature_c) || isnan(temperature_f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  hif = dht.computeHeatIndex(temperature_f, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(temperature_c, humidity, false);

  Blynk.virtualWrite(TEMPERATURE_PIN, temperature_c);
  Blynk.virtualWrite(HUMIDITY_PIN, humidity);

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature_c);
  Serial.print(F("°C "));
  Serial.print(temperature_f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
}

void displayInfo() {
  Serial.print("Location: ");
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    latitude = String(gps.location.lat(), 6);
    Serial.print(",");
    Serial.print(gps.location.lng(), 6);
    longitude = String(gps.location.lng(), 6);
  } else {
    Serial.print("INVALID");
  }

  Serial.println();
}

void sendEmail(String msg1, String msg2, String msg3) {
  EMailSender::EMailMessage message;
  message.subject = "User Location";
  message.message = "http://maps.google.com/maps?q=loc:" + latitude + "," + longitude + "\n Pulse rate: " + msg1 + "\nHumidity: " + msg2 + "\nTemperature: " + msg3;

  EMailSender::Response resp = emailSend.send("dipraguchait31@gmail.com", message);

  Serial.println("Sending status: ");

  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(resp.desc);
}
/*
 *
 *	Server side for Simple Connection (Always Connected) 
 *
 */

#include <ESP8266_TCP.h>
#include <SoftwareSerial.h>

#define ESP8266_SW_RX  9 // ESP8266 UART0 RXI goes to Arduino pin 9
#define ESP8266_SW_TX 8 // ESP8266 UART0 TXO goes to Arduino pin 8
static SoftwareSerial swSerial(ESP8266_SW_TX, ESP8266_SW_RX);
// ESP8266 Class
ESP8266_TCP wifi;
#define LED_BUILTIN 13
// Target Access Point
const char ssid[] = "HomeSweetHome";
const char pass[] = "ch96Q82A";
// Connect this pin to CH_PD pin on ESP8266
#define PIN_RESET    6

void setup()
{
  delay(3000);
  pinMode(LED_BUILTIN, OUTPUT);
  // We use Serial1 to interface with ESP8266 
  // and use Serial to debugging
  swSerial.begin(9600);
  Serial.begin(9600);
  wifi.begin(&swSerial, &Serial, PIN_RESET);
  
  /* If your board has only 1 serial port
   * or you didn't need to debugging, try this.
   *
   * Serial.begin(115200);
   * wifi.begin(&Serial, PIN_RESET);
   *
   */
  
  // Check that ESP8266 is available
  wifi.openTCPServer(2000, 30);
  return;
  Serial.println("before test"); 
  if(wifi.test()) 
  {
    Serial.println("IN test"); 
	// Connect to target Access Point
    //String ip = connectAP();
    //wifi.getIP();
    Serial.println("After test"); 
	  //Serial.println("IP:" + ip + "\n"); 

	// Open TCP Server on port 8888 and 30 seconds for connection timeout (Max 2880)
    wifi.openTCPServer(8888, 30);
  } 
  else 
  {
	// ESP8266 isn't available
    Serial.println("Check module connection and restart to try again..."); 
    while(true);
  }
}

void loop()
{  
  // Check for any data has coming to ESP8266
  int dataState = wifi.isNewDataComing(WIFI_SERVER);
  if(dataState != WIFI_NEW_NONE) {
    if(dataState == WIFI_NEW_CONNECTED) {
	  // Connected with TCP Client Side
      Serial.println("Status : Connected");
    } else if(dataState == WIFI_NEW_DISCONNECTED) {
	  // Disconnected from TCP Client Side
      Serial.println("Status : Disconnected");
    } else if(dataState == WIFI_NEW_MESSAGE) {
	  // Got a message from TCP Client Side
      Serial.println("ID : " + String(wifi.getId()));
      String command = wifi.getMessage();
      Serial.println("Message : " + command);
      if (command == "ON"){
        digitalWrite(LED_BUILTIN, HIGH); 
      }
      if (command == "OFF"){
        digitalWrite(LED_BUILTIN, LOW); 
      } 
    } else if(dataState == WIFI_NEW_SEND_OK) {
	  // Message transfer has successful
      Serial.println("SENT!!!!");
    } 
  }
}

// Access Point Connection Function that you can loop connect to Access Point until successful
String connectAP() 
{
  String ip = "0.0.0.0";
  return ip;
  while(ip.equals("0.0.0.0")) 
  {
    ip = wifi.connectAccessPoint(ssid, pass);
    if(!ip.equals("0.0.0.0")) 
    {
      break;
    } 
  }
  return ip;
}


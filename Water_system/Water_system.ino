#include <ESP8266_TCP.h>
#include <SoftwareSerial.h>
#include <DS1302.h>
#include <EEPROM.h>



#define ESP8266_SW_RX  9 // ESP8266 UART0 RXI goes to Arduino pin 9
#define ESP8266_SW_TX 8 // ESP8266 UART0 TXO goes to Arduino pin 8
#define PIN_RESET 6
#define PUMP_CONTROL 13
const int kCePin   = 2;  // Chip Enable RST
const int kIoPin   = 3;  // Input/Output DAT
const int kSclkPin = 4;  // Serial Clock CLK
#define DAYS_IN_WEEK 7
#define ALARM_PER_DAY 3


static SoftwareSerial swSerial(ESP8266_SW_TX, ESP8266_SW_RX);
// Target Access Point
//const char ssid[] = "HomeSweetHome";
const char mySSID[] = "Andrey-home";
const char myPSK[] = "ch96Q82A";

int eeAddress = 0;
DS1302 rtc(kCePin, kIoPin, kSclkPin);

// ESP8266 Class
ESP8266_TCP wifi;
const String TURN_ON = "ON";
const String TURN_OFF = "OFF";
const String ECHO = "Echo";
const String SAVE = "Save";
const String LOAD = "Load";
const String SET_TIME = "Set_Time";
const String GET_TIME = "Get_Time";
const String UPDATE_ALARM = "Update_Alarm";
const String GET_ALARM = "Get_Alarm";
const String RESET_ALARM = "Reset_Alarm";
const String GET_NEXT_ALARM = "Get_Next_Alarm";


class Alarm
{
   public:
    int sHour;
    int sMinute;
    int eHour;
    int eMinute;
    bool active;
    Alarm(){};
    Alarm(int sHour,int sMinute,int eHour,int eMinute,bool active):sHour{sHour},sMinute{sMinute},eHour{eHour},eMinute{eMinute},active{active}{};  
};
struct AlarmArray
{
  Alarm _arr[ALARM_PER_DAY];
};
struct WeekAlarms
{
  AlarmArray _arr[DAYS_IN_WEEK];
};
Time nextAlarm{0000, 0, 0, 0, 0, 00, 0};
Time nextAlarmEnd{0000, 0, 0, 0, 0, 00, 0};
bool isActive = false;
WeekAlarms weekArr;

void setup()
{
  pinMode(PUMP_CONTROL, OUTPUT);
  digitalWrite(PUMP_CONTROL, LOW);
  // Initialize a new chip by turning off write protection and clearing the
  // clock halt flag. These methods needn't always be called. See the DS1302
  // datasheet for details.
  rtc.writeProtect(false);
  rtc.halt(false);
  delay(3000);
  // We use Serial1 to interface with ESP8266 
  // and use Serial to debugging
  swSerial.begin(9600);
  Serial.begin(9600);
  
  //wifi.begin(&swSerial, &Serial, PIN_RESET);
  //init wifi;
  
  initializeWifi();
  delay(1000);
  connectESP8266();
  delay(1000);
  displayConnectInfo();
  delay(1000);
  wifi.openTCPServer(8888, 30);
  
  printTime(rtc.time(),"Now: ");
  printTime(nextAlarm,"Next alarm: ");
  //Serial.println("!!!!!");
  //errorLoop(55);
  //load the array from eeprom
  load();
}

void errorLoop(int error)
{
  Serial.print(F("Error: ")); Serial.println(error);
  Serial.println(F("Looping forever."));
  for (;;)
    ;
}
void turnOn()
{
  Serial.println(TURN_ON);
    isActive = true;
    digitalWrite(PUMP_CONTROL, HIGH); 
}
void turnOff()
{
  Serial.println(TURN_OFF);
    isActive = false;
    digitalWrite(PUMP_CONTROL, LOW); 
}
void load()
{
     Serial.println(LOAD);
     EEPROM.get( eeAddress, weekArr );
     findNextAlarm(false);
}
void save()
{
    Serial.println(SAVE);
    EEPROM.put( eeAddress, weekArr );
    findNextAlarm(false);
}

void sendTime(Time t,int source)
{
  char buf[50];
  const String day = dayAsString(t.day);
  // Format the time and date and insert into the temporary buffer.
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  Serial.println("In send Time");
  Serial.println(buf);
  //wifi.send(source,String(buf));
}

int dispatcher(String data, int source)
{
  if( data == TURN_ON )
  {
  turnOn();
  return 1;
  }
  if( data == TURN_OFF )
  {
    turnOff();
    return 1; 
  }
  if( data == ECHO )
  {
    Serial.println(ECHO);
    wifi.send(source,"OK");
    return 1;
  }
  if (data == GET_TIME)
  { 
    printTime(rtc.time(),"Current time: ");
    sendTime(rtc.time(),source);
    return 1;
  }
  if( data.substring(0,SET_TIME.length()) == SET_TIME )
  {
    Serial.println(SET_TIME);
    //Format Set_Time:[YEAR-MONTH-DAY] [HH]:[MM] [WEEKDAY] : Set_Time:[4]-[2]-[2] [2]:[2] [1]
    short year = SET_TIME.length() + 1;
    short month = year + 1 + 4;
    short day = month + 1 + 2;
    short hour = day +1 + 2;
    short minute = hour + 1 + 2;
    //short //second = minute + 2;
    short dayWeek = minute + 1 + 2;
    Time t(data.substring(year, month).toInt(), data.substring(month,day ).toInt(), data.substring(day, hour).toInt(), data.substring(hour, minute).toInt(), data.substring(minute, dayWeek).toInt(), 00, data.substring(dayWeek, dayWeek+2).toInt());
    printTime(t,"Got time: ");
    rtc.time(t);
    wifi.send(source,"OK");
    return 1;
  }
  
  if (data == SAVE)
  {
    save();
    return 1;
  }
  if (data == LOAD)
  {
    load();
    return 1;
  }
    if (data == RESET_ALARM)
  {
    Serial.println(RESET_ALARM);
    weekArr = WeekAlarms{};
    nextAlarm = Time{0000, 0, 0, 0, 0, 00, 0};
    nextAlarmEnd = Time{0000, 0, 0, 0, 0, 00, 0};
    turnOff();
    return 1;
  }
  if (data.substring(0,GET_ALARM.length()) == GET_ALARM)
  {
    //Format GET_ALARM:DAY SLOT ; UPDATE_ALARM:[1] [1]

    short day = GET_ALARM.length() + 1;
    short slot = day + 1 + 1;
    int sHour = weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].sHour;
    int sMinute = weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].sMinute;
    int eHour = weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].eHour;
    int eMinute = weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].eMinute;
    bool active = weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].active;
    
    
    Serial.println(weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].sHour);
    Serial.println(weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].sMinute);
    Serial.println(weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].eHour);
    Serial.println(weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].eMinute);
    Serial.println(weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, slot + 1).toInt()].active);
    
    
    char buf[50];
    snprintf(buf, sizeof(buf), "%02d:%02d %02d:%02d %01d",
           sHour,sMinute,eHour, eMinute,active);
     wifi.send(source,buf);
     return 1;
  }
  if (data.substring(0,UPDATE_ALARM.length()) == UPDATE_ALARM)
  {
     Serial.println(UPDATE_ALARM);
    //Format UPDATE_ALARM:DAY SLOT [HH]:[MM] [HH]:[MM] ACTIVE; UPDATE_ALARM:[1] [1] [2]:[2] [2]:[2] [1]
    short day = UPDATE_ALARM.length() + 1;
    short slot = day + 1 + 1;
    short sHour = slot + 1 + 1;
    short sMinute = sHour + 1 + 2;
    short eHour = sMinute + 1 + 2;
    short eMinute = eHour + 1 + 2;
    short active = eMinute + 1 + 2;
    weekArr._arr[data.substring(day, slot).toInt() - 1]._arr[data.substring(slot, sHour).toInt()] = Alarm{data.substring(sHour, sMinute).toInt(),data.substring(sMinute,eHour ).toInt(),data.substring(eHour,eMinute ).toInt(),data.substring(eMinute,active ).toInt(),data.substring(active,active + 1 ).toInt()};
    wifi.send(source,"OK");
    findNextAlarm(false);
    return 1;
  }
  if (data == GET_NEXT_ALARM)
  {

    printTime(nextAlarm,"Next Alarm: ");
    sendTime(nextAlarm,source );
    return 1;
  }
  return 0;  
}
void loop()
{  
  // Check for any data has coming to ESP8266
  int dataState = wifi.isNewDataComing();
  String command;
  Time curTime = rtc.time();
  if (!isActive && curTime == nextAlarm)
  {
    turnOn();
  }
   if (isActive && curTime == nextAlarmEnd)
  {
    turnOff();
    findNextAlarm(false); 
  }
  if(dataState != WIFI_NEW_NONE)
  {
    switch (dataState) 
    {
      case WIFI_NEW_CONNECTED:
        Serial.println("Status : Connected");
        break;
      case WIFI_NEW_DISCONNECTED:
        Serial.println("Status : Disconnected");
        break;
      case WIFI_NEW_MESSAGE:
        Serial.println("ID : " + String(wifi.getId()));
        command = wifi.getMessage();
        Serial.println("Message : " + command);
        if( command == ECHO )
        {
          Serial.println(ECHO);
          wifi.send(wifi.getId(),"OK");
          //return 1;
        } 
        dispatcher(command,wifi.getId()); 
        break;
      case WIFI_NEW_SEND_OK:
        Serial.println("SENT!!!!");
        break;
    }
  }
}

String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

void printTime(Time t,String txt) {
  // Get the current time and date from the chip.
  // Name the day of the week.
  char buf[50];
  const String day = dayAsString(t.day);
  // Format the time and date and insert into the temporary buffer.
  snprintf(buf, sizeof(buf), "%s %s %04d-%02d-%02d %02d:%02d:%02d",
           txt.c_str(),day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  Serial.println(buf);
}


void findNextAlarm(bool all)
{
  Serial.println("In find next Alarm");
  Serial.println("all val : " + String(all));
  
  Time curTime = rtc.time();
  long curVal = 60*24*7;
  bool isNextGood = true;
  if (all)
  {
      isNextGood = false;
      curVal = 0;
  }
  for (int i = 0; i < DAYS_IN_WEEK; ++i)
  {
    for (int j = 0; j < ALARM_PER_DAY ; ++j)
    {
        if (weekArr._arr[i]._arr[j].active)
        {
          Time temp(curTime.yr, curTime.mon, curTime.date, weekArr._arr[i]._arr[j].sHour, weekArr._arr[i]._arr[j].sMinute, 00, i + 1);
          long diff = curTime - temp;
          if (all)
          {
            diff = abs(diff);
          } 
          if (!all && diff > 0 && diff < curVal)
          {
            Serial.println("Found One");
            isNextGood = false;
            curVal = diff;
            nextAlarm = temp;
            nextAlarmEnd = Time(curTime.yr, curTime.mon, curTime.date, weekArr._arr[i]._arr[j].eHour, weekArr._arr[i]._arr[j].eMinute, 00, i + 1);
            if (isActive)
            {
                  isActive = false;
                  digitalWrite(PUMP_CONTROL, LOW);
            } 
          }
          else if(all && diff > curVal)
          {
            curVal = diff;
            nextAlarm = temp;
            nextAlarmEnd = Time(curTime.yr, curTime.mon, curTime.date, weekArr._arr[i]._arr[j].eHour, weekArr._arr[i]._arr[j].eMinute, 00, i + 1);
            if (isActive)
            {
                  isActive = false;
                  digitalWrite(PUMP_CONTROL, LOW);
            } 
          }
        } 
    } 
  }
  if (isNextGood)
  {
    findNextAlarm(true);
    return;
  }
  printTime(nextAlarm,"Next alarm : ");
}


void initializeWifi()
{
  // esp8266.begin() verifies that the ESP8266 is operational
  // and sets it up for the rest of the sketch.
  // It returns either true or false -- indicating whether
  // communication was successul or not.
  // true
  wifi.begin(&swSerial, &Serial, PIN_RESET);
  bool test = wifi.test();
  if (!test)
  {
    Serial.println(F("Error talking to ESP8266."));
  }
  Serial.println(F("ESP8266 Shield Present"));
}

void connectESP8266()
{
  // The ESP8266 can be set to one of three modes:
  //  1 - ESP8266_MODE_STA - Station only
  //  2 - ESP8266_MODE_AP - Access point only
  //  3 - ESP8266_MODE_STAAP - Station/AP combo
  // Use esp8266.getMode() to check which mode it's in:
  int retVal = wifi.getMode();
  if (retVal != ESP8266_MODE_STA)
  {
    Serial.println(F("Mode was not station"));
    // If it's not in station mode.
    // Use esp8266.setMode([mode]) to set it to a specified
    // mode.
    retVal = wifi.setMode(ESP8266_MODE_STA);
    Serial.println(F("Mode set to station"));
    if (retVal < 0)
    {
      Serial.println(F("Error setting mode, trying again"));
      //errorLoop(1);
      delay(5000);
      connectESP8266();
      return;
    }
  }
  Serial.println(F("Mode was station"));

  // esp8266.status() indicates the ESP8266's WiFi connect
  // status.
  // A return value of 1 indicates the device is already
  // connected. 0 indicates disconnected. (Negative values
  // equate to communication errors.)
  retVal = wifi.status();
  if (retVal == ESP8266_STATUS_CONNECTED || retVal == ESP8266_STATUS_NOWIFI)
  {
    
    Serial.println("No connection, connecting to " + String(mySSID));
    // esp8266.connect([ssid], [psk]) connects the ESP8266
    // to a network.
    // On success the connect function returns a value >0
    // On fail, the function will either return:
    //  -1: TIMEOUT - The library has a set 30s timeout
    //  -3: FAIL - Couldn't connect to network.
    retVal = wifi.connect(mySSID, myPSK);
    if (retVal < 0)
    {
      Serial.println("Error connecting, trying again");
      delay(5000);
      connectESP8266();
      return;
    }
  }
  Serial.println(F("Connection good"));
}

void displayConnectInfo()
{
  char buf[24];
  memset(buf, 0, 24);
  // esp8266.getAP() can be used to check which AP the
  // ESP8266 is connected to. It returns an error code.
  // The connected AP is returned by reference as a parameter.
  int retVal = wifi.getAP(buf);
  if (retVal > 0)
  {
    Serial.println("Connected to: " + String(buf));
  }
  else
  {
    Serial.print(F("Error getting AP"));
  }
  memset(buf, 0, 24);
  // esp8266.localIP returns an IPAddress variable with the
  // ESP8266's current local IP address.
  retVal = wifi.getIP(buf);
  if (retVal > 0)
  {
    Serial.println("My IP: " + String(buf));
  }
  else
  {
    Serial.println(F("Error getting IP"));
  }
}


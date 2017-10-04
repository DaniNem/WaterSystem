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
int eeAddress = 0;
static SoftwareSerial swSerial(ESP8266_SW_TX, ESP8266_SW_RX);
// ESP8266 Class
ESP8266_TCP wifi;
// Target Access Point
//const char ssid[] = "HomeSweetHome";
const char ssid[] = "Andrey-home";
const char pass[] = "";

DS1302 rtc(kCePin, kIoPin, kSclkPin);

const String TURN_ON = "ON";
const String TURN_OFF = "OFF";
const String ECHO = "Echo";
const String SAVE = "Save";
const String LOAD = "Load";
const String SET_TIME = "Set_Time";
const String GET_TIME = "Get_Time";
const String UPDATE_ALARM = "Update_Alarm";



class Alarm
{
   public:
    short sHour;
    short sMinute;
    short eHour;
    short eMinute;
    bool active;
    Alarm(){};
    Alarm(short sHour,short sMinute,short eHour,short eMinute,bool active):sHour{sHour},sMinute{sMinute},eHour{sHour},eMinute{sMinute},active{active}{};  
};
struct AlarmArray
{
  Alarm _arr[ALARM_PER_DAY];
};
struct WeekAlarms
{
  AlarmArray _arr[DAYS_IN_WEEK];
};
UTime nextAlarm(0000, 0, 0, 0, 0, 00, 0);
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
  wifi.begin(&swSerial, &Serial, PIN_RESET); 
  // Check that ESP8266 is available
  wifi.openTCPServer(8888, 30);
}

int dispacher(String& data, int && source)
{
  if( data == TURN_ON )
  {
    digitalWrite(PUMP_CONTROL, HIGH); 
    return 1;
  }
  if( data == TURN_OFF )
  {
    digitalWrite(PUMP_CONTROL, LOW);
    return 1; 
  }
  if( data == ECHO )
  {
    wifi.send(source,"OK");
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
    //Serial.println( formatTime(t));
    //rtc.time(t);
    wifi.send(source,"OK");
    return 1;
  }
  if (data == GET_TIME)
  {
    String curTime = formatTime(rtc.time());
    Serial.println(curTime);
    wifi.send(source, curTime);
    return 1;
  }
  if (data == SAVE)
  {
    Serial.println(SAVE);
    EEPROM.put( eeAddress, weekArr );
    findNextAlarm();
    return 1;
  }
  if (data == LOAD)
  {
    Serial.println(LOAD);
     EEPROM.get( eeAddress, weekArr );
     findNextAlarm();
     return 1;
  }
  if (data == UPDATE_ALARM)
  {
     Serial.println(UPDATE_ALARM);
    //Format UPDATE_ALARM:DAY SLOT [HH]:[MM] [HH]:[MM] ACTIVE; UPDATE_ALARM:[1] [1] [2]:[2] [2]:[2] [1]
    short day = UPDATE_ALARM.length() + 1;
    day -= 1;
    short slot = day + 1 + 1;
    short shour = slot + 1 + 2;
    short sminute = shour + 1 + 2;
    short ehour = sminute + 1 + 2;
    short eminute = ehour + 1 + 2;
    bool active = eminute + 1;
    return 1;
  //UTime t(data.substring(year, month).toInt(), data.substring(month,day ).toInt(), data.substring(day, hour).toInt(), data.substring(hour, minute).toInt(), data.substring(minute, dayWeek).toInt(), 00, data.substring(dayWeek, dayWeek+2).toInt());
  //  weekArr._arr[day]._arr[slot] = Alarm{data.substring(shour, sminute).toInt(),data.substring(sminute,ehour ).toInt(),data.substring(ehour,eminute ).toInt(),data.substring(eminute,active ).toInt(),data.substring(active,active + 1 ).toInt()}
  Serial.println(weekArr._arr[day]._arr[slot].sHour);
  Serial.println(weekArr._arr[day]._arr[slot].sMinute);
  Serial.println(weekArr._arr[day]._arr[slot].eHour);
  Serial.println(weekArr._arr[day]._arr[slot].eMinute);
  Serial.println(weekArr._arr[day]._arr[slot].active);
    wifi.send(source,"OK");
    return 1;
  }
  return 0;  
}
void loop()
{  
  // Check for any data has coming to ESP8266
  int dataState = wifi.isNewDataComing();
  String command;
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
        dispacher(command,wifi.getId() ); 
        break;
      case WIFI_NEW_SEND_OK:
        Serial.println("SENT!!!!");
        break;
    }
  }
}

String dayAsString(const UTime::Day day) {
  switch (day) {
    case UTime::kSunday: return "Sunday";
    case UTime::kMonday: return "Monday";
    case UTime::kTuesday: return "Tuesday";
    case UTime::kWednesday: return "Wednesday";
    case UTime::kThursday: return "Thursday";
    case UTime::kFriday: return "Friday";
    case UTime::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

String formatTime(UTime t) {
  // Get the current time and date from the chip.
  //Time t = rtc.time();

  // Name the day of the week.
  const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[50];
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  String retVal = buf;
  // Print the formatted string to serial so we can see the time.
  return buf;
}


void findNextAlarm()
{
  Serial.println(sizeof(weekArr));
  Serial.println(sizeof(AlarmArray));
  Serial.println(sizeof(Alarm));
  UTime curTime = rtc.time();
  int m = curTime.mon;
  int yr = curTime.yr;
  long curMin = 3600*24*7;
  for (int i = 0; i < DAYS_IN_WEEK; ++i)
  {
    for (int j = 0; j < ALARM_PER_DAY ; ++j)
    {
      UTime temp(curTime.mon, m, curTime.date, weekArr._arr[i]._arr[j].sHour, weekArr._arr[i]._arr[j].sMinute, 00, j + 1);
      long diff = curTime - temp;
      Serial.print(formatTime(nextAlarm));
      Serial.print(formatTime(temp));
      Serial.print(diff);
      if (weekArr._arr[i]._arr[j].active && diff > 0 && diff < curMin)
      {
        curMin = diff;
        nextAlarm = temp;
      }
    }  
  }
  Serial.print("Next alarm : ");
  Serial.println(formatTime(nextAlarm));
}


/*
  WiFlyTCP.h - WiFly Library for Arduino
*/
// ensure this library description is only included once
#ifndef _ESP8266_TCP_H_
#define _ESP8266_TCP_H_


#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "util/ESP8266_TYPE.h"
#include "util/ESP8266_AT.h"
#include <Stream.h>
#include <avr/pgmspace.h>
//#include <IPAddress.h>



#define WIFI_MODE_STATION			0x01
#define WIFI_MODE_AP				0x02
#define WIFI_MODE_BOTH				0x03

#define WIFI_MUX_SINGLE				0x00
#define WIFI_MUX_MULTI				0x01

#define WIFI_TCP_DISABLE			0x00
#define WIFI_TCP_ENABLE				0x01

#define WIFI_CLIENT					0x00
#define WIFI_SERVER					0x01

#define WIFI_STATE_IDLE				0x00
#define WIFI_STATE_UNAVAILABLE		0x01
#define WIFI_STATE_SEND				0x02
#define WIFI_STATE_CONNECT			0x03

#define WIFI_NEW_NONE				0x00
#define WIFI_NEW_MESSAGE			0x01
#define WIFI_NEW_CONNECTED			0x02
#define WIFI_NEW_DISCONNECTED		0x03
#define WIFI_NEW_SEND_OK			0x04
#define WIFI_NEW_SEND_ERROR			0x05
#define WIFI_NEW_RESET				0x06
#define WIFI_NEW_ALREADY_CONNECT	0x07
#define WIFI_NEW_ETC				0x08

///////////////////////////////
// Command Response Timeouts //
///////////////////////////////
#define COMMAND_RESPONSE_TIMEOUT 1000
#define COMMAND_PING_TIMEOUT 3000
#define WIFI_CONNECT_TIMEOUT 30000
#define COMMAND_RESET_TIMEOUT 5000
#define CLIENT_CONNECT_TIMEOUT 5000


#define ESP8266_SOCK_NOT_AVAIL 255


// library interface description

class ESP8266_TCP {

	public:
		ESP8266_TCP();
		void begin(Stream *serial, Stream *serialDebug, int pinReset);
		void begin(Stream *serial, int pinReset);
		bool test();
		int getIP(char * ip);
		bool setStaticIP(const char* IP, const char* subnet, const char* dg);
		int getAP(char * ssid);
		void reset();
		void hardReset();
		int status();
		int updateStatus();
		bool closeTCPServer();
		int connect(const char * ssid);
		int connect(const char * ssid, const char * pwd);
		void openTCPServer(int port, int timeout);
		void connectTCP(String ip, int port);
		void closeTCPConnection();
		void closeTCPConnection(int id);

		int getRunningState();

		int getId();
		String& getMessage();
		void clearNewMessage();
		bool isNewDataComing();

		bool send(String message);
		int send(int id, String message);
		
		//void printClientList();
		int  getMode();
		int setMode(esp8266_wifi_mode mode);
	private:	

		//void waitingForReset();
		//void waitingForReset(unsigned long timeout);
		//void waitingForHardReset();

		void waitingForTCPConnection();

		int setMux(uint8_t mux);

		bool enableTCPServer(int port);
		bool setTCPTimeout(int timeout);

		void flush();
		void setRunningState(int state);

		void debugPrintln(String str);
		void debugPrint(String str);

		void clearBuffer();
		void clearBufferInner();
		int findChar(String str, int start, char c);
		void clear();

		void write(String str);
		
		int available();

		String read();
		String readData();
		//String readData(unsigned long timeout);
		String readTCPData();
		//////////////////////////
		// Command Send/Receive //
		//////////////////////////
		
		void sendCommand(const char * cmd, enum esp8266_command_type type = ESP8266_CMD_EXECUTE, const char * params = NULL);
		
		int readForResponse(const char * rsp, unsigned int timeout);
		int readForResponses(const char * pass, const char * fail, unsigned int timeout);
		
		/// searchBuffer([test]) - Search buffer for string [test]
		/// Success: Returns pointer to beginning of string
		/// Fail: returns NULL
		//! TODO: Fix this function so it searches circularly
		char * searchBuffer(const char * test);
		/// readByteToBuffer() - Read first byte from UART receive buffer
		/// and store it in rxBuffer.
		unsigned int readByteToBuffer();

		//bool TCPConnected;
		bool TCPEnable;
		bool isDebug;

		int pinReset;
		
		int clientId;
		String clientMessage;
		
		Stream *serial;
		Stream *serialDebug;
		esp8266_status _status;
		int runningState;
};
#endif


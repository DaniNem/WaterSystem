#include "ESP8266_TCP.h"
#include <Arduino.h>

////////////////////////
// Buffer Definitions //
////////////////////////
//128
#define ESP8266_RX_BUFFER_LEN 128 // Number of bytes in the serial receive buffer
char esp8266RxBuffer[ESP8266_RX_BUFFER_LEN];
unsigned int bufferHead; // Holds position of latest byte placed in buffer.

ESP8266_TCP::ESP8266_TCP() {

}
void ESP8266_TCP::begin(Stream *serial, Stream *serialDebug, int pinReset)
{
	pinMode(pinReset, OUTPUT);
	digitalWrite(pinReset, HIGH);
	this->pinReset = pinReset;
	this->serial = serial;
	this->serialDebug = serialDebug;
	this->isDebug = true;
	this->clientId = -1;
	this->clientMessage = "";
	this->runningState = WIFI_STATE_UNAVAILABLE;
	delay(2000);
}

void ESP8266_TCP::begin(Stream *serial, int pinReset)
{
	pinMode(pinReset, OUTPUT);
	digitalWrite(pinReset, HIGH);
	this->pinReset = pinReset;
	this->serial = serial;
	this->isDebug = false;
	this->clientId = -1;
	this->clientMessage = "";
	this->runningState = WIFI_STATE_UNAVAILABLE;
	delay(2000);

}




void ESP8266_TCP::reset() {
	clearBuffer();
	write("AT+RST");
	test();
	//waitingForReset();
}

void ESP8266_TCP::hardReset() {
	digitalWrite(this->pinReset, LOW);
	delay(1000);
	digitalWrite(this->pinReset, HIGH);
	test();
	//waitingForHardReset();
}


int ESP8266_TCP::setMux(uint8_t mux)
{
	char params[2] = {0, 0};
	params[0] = (mux > 0) ? '1' : '0';
	sendCommand(ESP8266_TCP_MULTIPLE, ESP8266_CMD_SETUP, params);
	
	return readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}

bool ESP8266_TCP::enableTCPServer(int port) {
	clearBuffer();
	write("AT+CIPSERVER=1," + String(port));
	
	String data = readData();
	debugPrintln(data);
	if(data.equals("no change")) 
		return true;
	data = readData();
	debugPrintln(data);
	return data.equals("OK"); 
}

bool ESP8266_TCP::closeTCPServer() {
	clearBuffer();
	write("AT+CIPSERVER=0");
	debugPrintln(readData());
	debugPrintln(readData());
	String data = readData();
	debugPrintln(data);
	return data.equals("OK");
}


void ESP8266_TCP::closeTCPConnection() {
	delay(1000);
	clearBuffer();
	write("AT+CIPCLOSE");
	debugPrintln(readData());
}

void ESP8266_TCP::closeTCPConnection(int id) {
	delay(1000);
	clearBuffer();
	write("AT+CIPCLOSE=" + String(id));
	debugPrintln(readData());
}

void ESP8266_TCP::openTCPServer(int port, int timeout) {
	clearBuffer();
	//clearBufferInner();
	setMux(WIFI_MUX_MULTI);
	enableTCPServer(port);
	setTCPTimeout(timeout);
}

bool ESP8266_TCP::setTCPTimeout(int timeout) {
	clearBuffer();
	write("AT+CIPSTO=" + String(timeout));
	debugPrintln(readData());
	debugPrintln(readData());
	String data = readData();
	debugPrintln(data);
	return data.equals("OK");
}

void ESP8266_TCP::connectTCP(String ip, int port) {
	clearBuffer();
	write("AT+CIPSTART=\"TCP\",\"" + ip + "\"," + String(port) );
	debugPrintln(readData());

	setRunningState(WIFI_STATE_CONNECT);
}

void ESP8266_TCP::waitingForTCPConnection() {

}

void ESP8266_TCP::setRunningState(int state) {
	this->runningState = state;
}

int ESP8266_TCP::getRunningState() {
	return this->runningState;
}

void ESP8266_TCP::flush() 
{
	this->serial->flush();
}
int ESP8266_TCP::findChar(String str, int start, char c) {
	for(int i = start ; i < str.length() ; i++) {
		if(str.charAt(i) == c)
			return i;
	}
	return -1;
}

int ESP8266_TCP::getId() {
	return this->clientId;
}

String& ESP8266_TCP::getMessage() {
	//clearBuffer();
	clearBufferInner();
	return this->clientMessage;
}

void ESP8266_TCP::clearBuffer() {
	while(available() > 0) {
		char t = serial->read();
		//debugPrintln("Clear buffer : " + t);
	}
}
void ESP8266_TCP::clearBufferInner() 
{
	memset(esp8266RxBuffer, '\0', ESP8266_RX_BUFFER_LEN);
	bufferHead = 0;
}

void ESP8266_TCP::clear() {
	clearBuffer();
	this->clientId = -1;
	this->clientMessage = "";
	//this->TCPConnected = false;	
}

void ESP8266_TCP::debugPrintln(String str) {
	if(this->isDebug)
	{
		serialDebug->println(str);
		serialDebug->flush();

	}
}
void ESP8266_TCP::debugPrint(String str) {
	if(this->isDebug)
	{
		serialDebug->print(str);
		serialDebug->flush();
	}
}

String ESP8266_TCP::readData() {
    String data = "";
    while(available() > 0) {
        char r = serial->read();
        if (r == '\n') {
            return data;
        } else if(r == '\r') {
        } else {
            data += r;  
        }
    }
    return "";
}

void ESP8266_TCP::write(String str) {
	this->serial->println(str);
	flush();
	delay(50);
}

bool ESP8266_TCP::send(String message) {
	if(getRunningState() == WIFI_STATE_IDLE) {
		debugPrintln("Send : " + message);
		write("AT+CIPSEND=" + String(message.length() + 1));
		serial->print(message + " ");
		flush();
		debugPrintln(readData());
		debugPrintln(readData());
		setRunningState(WIFI_STATE_SEND);
		return true;
	}
	return false;
}

/*bool ESP8266_TCP::send(int id, String message) {
	if(getRunningState() == WIFI_STATE_IDLE) {
		debugPrintln("ID : " + String(id) + " Send : " + message);
		clearBuffer();
		write("AT+CIPSEND=" + String(id) + "," + String(message.length()));
		serial->print(message);
		flush();
		debugPrintln(readData());
		debugPrintln(readData());
		setRunningState(WIFI_STATE_SEND);
		return true;
	}
	return false;
}*/
int ESP8266_TCP::send(int linkID,String message )
{
	const uint8_t *buf = message.c_str();
	size_t size = message.length();
	if (size > 2048)
		return ESP8266_CMD_BAD;
	char params[8];
	sprintf(params, "%d,%d", linkID, size);
	sendCommand(ESP8266_TCP_SEND, ESP8266_CMD_SETUP, params);
	
	int rsp = readForResponses(RESPONSE_OK, RESPONSE_ERROR, COMMAND_RESPONSE_TIMEOUT);
	//if (rsp > 0)
	if (rsp != ESP8266_RSP_FAIL)
	{
		this->serial->println(((const char *)buf));;
		
		rsp = readForResponse("SEND OK", COMMAND_RESPONSE_TIMEOUT);
		
		if (rsp > 0)
			return size;
	}
	
	return rsp;
}


int ESP8266_TCP::available() {
	return this->serial->available();
}

String ESP8266_TCP::read() {
	String data = readTCPData();
	if(data.equals("Unlink")) {
		//this->TCPConnected = false;
		clearBuffer();
		return "";
	}
	return data;
}
String ESP8266_TCP::readTCPData() {
	unsigned long timeout = 100;
	unsigned long t = millis();
    String data = "";
    while(millis() - t < timeout) {
    	if(available() > 0) {
	        char r = serial->read();
	        if(data.equals("Unlink")) {
				return data;
	        } else {
	            data += r;  
                t = millis();
	        }
	    }
    }
    return data;
}



bool ESP8266_TCP::test() {
	debugPrintln("In Test");
	clearBuffer();
	sendCommand(ESP8266_TEST,ESP8266_CMD_EXECUTE);
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
    //debugPrintln(data);
	if(rsp > 0) 
	{
    	return true;
	}
	else 
	{
    	debugPrintln("Test Failed!! trying to hard reset");
		hardReset();
		return true;
	} 
}

bool ESP8266_TCP::isNewDataComing(){
	if (available())
	{
		unsigned long timeIn = millis();	// Timestamp coming into function
		unsigned int received = 0; // received keeps track of number of chars read
		unsigned int timeout = 100;
		clearBufferInner();	// Clear the class receive buffer (esp8266RxBuffer)
		while (timeIn + timeout > millis()) // While we haven't timed out
		{
			if (available()) // If data is available on UART RX
			{
				received += readByteToBuffer();
		}
		}
		if (searchBuffer("+IPD"))
		{	
			char * p = strstr(esp8266RxBuffer,"+IPD");
			if (p != NULL)
			{
					p += strlen("+IPD") + 1;
					char * q = strchr(p, ',');
					if (q == NULL) return ESP8266_RSP_UNKNOWN;
					char temp[1];
					memset(temp, 0, 1); // Clear tempOctet
					
					strncpy(temp, p, q-p); // Copy string to temp char array:
					this->clientId =  atoi(temp);
					p = ++q;
					q = strchr(p, ':');
					if (q == NULL) return ESP8266_RSP_UNKNOWN;
					memset(temp, 0, 1); // Clear tempOctet
					strncpy(temp, p, q-p); // Copy string to temp char array:
					int length =  atoi(temp);
					
					//debugPrintln(String(length));
					this->clientMessage = "";
					q++;
					for (int i = 0; i < length; i++)
					{
						this->clientMessage += *q;
						q++;
					}
					//debugPrintln(this->clientMessage);
					return true;
			}
			
		}
		return false;
		
	}
	return false;
}
int ESP8266_TCP::getAP(char * ssid)
{
	sendCommand(ESP8266_CONNECT_AP, ESP8266_CMD_QUERY); // Send "AT+CWJAP?"
	
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	// Example Responses: No AP\r\n\r\nOK\r\n
	// - or -
	// +CWJAP:"WiFiSSID","00:aa:bb:cc:dd:ee",6,-45\r\n\r\nOK\r\n
	if (rsp > 0)
	{
		// Look for "No AP"
		if (strstr(esp8266RxBuffer, "No AP") != NULL)
			return 0;
		
		// Look for "+CWJAP"
		char * p = strstr(esp8266RxBuffer, ESP8266_CONNECT_AP);
		if (p != NULL)
		{
			p += strlen(ESP8266_CONNECT_AP)*2 + 6;
			char * q = strchr(p, '"');
			if (q == NULL) return ESP8266_RSP_UNKNOWN;
			strncpy(ssid, p, q-p); // Copy string to temp char array:
			return 1;
		}
	}
	
	return rsp;
}
int ESP8266_TCP::getIP(char * ip) {
	sendCommand(ESP8266_GET_LOCAL_IP, ESP8266_CMD_EXECUTE); // Send "AT+CIFSR"
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if ( rsp > 0)
	{
		char * p = strstr(esp8266RxBuffer, ESP8266_GET_LOCAL_IP);
		if (p != NULL)
		{
			p  = strchr(p, '"') + 1;
			char * q = strchr(p, '"');
			if (q == NULL) return ESP8266_RSP_UNKNOWN;
			strncpy(ip, p, q-p); // Copy string to temp char array:
			return 1;
		}
	}
	
	return rsp; 
}
bool ESP8266_TCP::setStaticIP( const char* IP, const char* subnet, const char* dg) {
	char params[55] = {0};
	sprintf(params, "\"%s\",\"%s\",\"%s\"", IP ,dg , subnet);
	//debugPrintln(params);
	//return 1;
	sendCommand(ESP8266_SET_STA_IP, ESP8266_CMD_SETUP,params); // Send "AT+CIPSTA_CUR"
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if ( rsp > 0)
	{
		debugPrintln(String(esp8266RxBuffer));
		return true;
	}
	
	return false; 
}

// getMode()
// Input: None
// Output:
//    - Success: 1, 2, 3 (ESP8266_MODE_STA, ESP8266_MODE_AP, ESP8266_MODE_STAAP)
//    - Fail: <0 (esp8266_cmd_rsp)
int ESP8266_TCP::getMode()
{
	sendCommand(ESP8266_WIFI_MODE, ESP8266_CMD_QUERY);
	
	// Example response: \r\nAT+CWMODE_CUR?\r+CWMODE_CUR:2\r\n\r\nOK\r\n
	// Look for the OK:
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{
		// Then get the number after ':':
		char * p = strchr(esp8266RxBuffer, ':');
		if (p != NULL)
		{
			char mode = *(p+1);
			if ((mode >= '1') && (mode <= '3'))
				return (mode - 48); // Convert ASCII to decimal
		}
		
		return ESP8266_RSP_UNKNOWN;
	}
	
	return rsp;
}

int ESP8266_TCP::connect(const char * ssid)
{
	return connect(ssid, "");
}

// connect()
// Input: ssid and pwd const char's
// Output:
//    - Success: >0
//    - Fail: <0 (esp8266_cmd_rsp)
int ESP8266_TCP::connect(const char * ssid, const char * pwd)
{
	this->serial->print("AT");
	this->serial->print(ESP8266_CONNECT_AP);
	this->serial->print("=\"");
	this->serial->print(ssid);
	this->serial->print("\"");
	if (pwd != NULL)
	{
		this->serial->print(',');
		this->serial->print("\"");
		this->serial->print(pwd);
		this->serial->print("\"");
	}
	this->serial->print("\r\n");
	
	return readForResponses(RESPONSE_OK, RESPONSE_FAIL, WIFI_CONNECT_TIMEOUT);
}
// setMode()
// Input: 1, 2, 3 (ESP8266_MODE_STA, ESP8266_MODE_AP, ESP8266_MODE_STAAP)
// Output:
//    - Success: >0
//    - Fail: <0 (esp8266_cmd_rsp)
int ESP8266_TCP::setMode(esp8266_wifi_mode mode)
{
	char modeChar[2] = {0, 0};
	sprintf(modeChar, "%d", mode);
	sendCommand(ESP8266_WIFI_MODE, ESP8266_CMD_SETUP, modeChar);
	
	return readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
}
void ESP8266_TCP::sendCommand(const char * cmd, enum esp8266_command_type type, const char * params)
{

	this->serial->print("AT");
	this->serial->print(cmd);
	if (type == ESP8266_CMD_QUERY)
	{	this->serial->print('?');
		debugPrintln("AT" + String(cmd)+"?");
	}
	else if (type == ESP8266_CMD_SETUP)
	{
		this->serial->print("=");
		this->serial->print(params);
		debugPrintln("AT" + String(cmd) + "=" + String(params) );	
	}
	this->serial->print("\r\n");
	flush();
	delay(50);
}

int ESP8266_TCP::readForResponse(const char * rsp, unsigned int timeout)
{
	unsigned long timeIn = millis();	// Timestamp coming into function
	unsigned int received = 0; // received keeps track of number of chars read
	
	clearBufferInner();	// Clear the class receive buffer (esp8266RxBuffer)
	while (timeIn + timeout > millis()) // While we haven't timed out
	{
		if (available()) // If data is available on UART RX
		{
			received += readByteToBuffer();
			//debugPrintln("Have data, received: "+ String(received));
			if (searchBuffer(rsp))	// Search the buffer for goodRsp
				return received;	// Return how number of chars read
		}
	}
	
	if (received > 0) // If we received any characters
		return ESP8266_RSP_UNKNOWN; // Return unkown response error code
	else // If we haven't received any characters
		return ESP8266_RSP_TIMEOUT; // Return the timeout error code
}

int ESP8266_TCP::readForResponses(const char * pass, const char * fail, unsigned int timeout)
{
	unsigned long timeIn = millis();	// Timestamp coming into function
	unsigned int received = 0; // received keeps track of number of chars read
	
	clearBufferInner();	// Clear the class receive buffer (esp8266RxBuffer)
	while (timeIn + timeout > millis()) // While we haven't timed out
	{
		if (this->serial->available()) // If data is available on UART RX
		{
			received += readByteToBuffer();
			if (searchBuffer(pass))	// Search the buffer for goodRsp
				return received;	// Return how number of chars read
			if (searchBuffer(fail))
				return ESP8266_RSP_FAIL;
		}
	}
	
	if (received > 0) // If we received any characters
		return ESP8266_RSP_UNKNOWN; // Return unkown response error code
	else // If we haven't received any characters
		return ESP8266_RSP_TIMEOUT; // Return the timeout error code
}
char * ESP8266_TCP::searchBuffer(const char * test)
{
	int bufferLen = strlen((const char *)esp8266RxBuffer);
	// If our buffer isn't full, just do an strstr
	if (bufferLen < ESP8266_RX_BUFFER_LEN)
		return strstr((const char *)esp8266RxBuffer, test);
	else
	{	//! TODO
		// If the buffer is full, we need to search from the end of the 
		// buffer back to the beginning.
		
		//int testLen = strlen(test);
		for (int i=0; i<ESP8266_RX_BUFFER_LEN; i++)
		{
			
		}
	}
}

unsigned int ESP8266_TCP::readByteToBuffer()
{
	// Read the data in
	char c = this->serial->read();
	
	// Store the data in the buffer
	esp8266RxBuffer[bufferHead] = c;
	//! TODO: Don't care if we overflow. Should we? Set a flag or something?
	bufferHead = (bufferHead + 1) % ESP8266_RX_BUFFER_LEN;
	//debugPrintln("BufferHead = " + String(bufferHead));
	
	return 1;
}
int ESP8266_TCP::status()
{
	int statusRet = updateStatus();
	if (statusRet > 0)
	{
		if (_status.stat  == 0)
			return status();
		return _status.stat;
	}
	return statusRet;
}
int ESP8266_TCP::updateStatus()
{
	sendCommand(ESP8266_TCP_STATUS); // Send AT+CIPSTATUS\r\n
	// Example response: (connected as client)
	// STATUS:3\r\n
	// +CIPSTATUS:0,"TCP","93.184.216.34",80,0\r\n\r\nOK\r\n 
	// - or - (clients connected to ESP8266 server)
	// STATUS:3\r\n
	// +CIPSTATUS:0,"TCP","192.168.0.100",54723,1\r\n
	// +CIPSTATUS:1,"TCP","192.168.0.101",54724,1\r\n\r\nOK\r\n 
	int rsp = readForResponse(RESPONSE_OK, COMMAND_RESPONSE_TIMEOUT);
	if (rsp > 0)
	{
		char * p = searchBuffer("STATUS:");
		if (p == NULL)
			return ESP8266_RSP_UNKNOWN;
		
		p += strlen("STATUS:");
		_status.stat = (esp8266_connect_status)(*p - 48);
		
		for (int i=0; i<ESP8266_MAX_SOCK_NUM; i++)
		{
			p = strstr(p, "+CIPSTATUS:");
			if (p == NULL)
			{
				// Didn't find any IPSTATUS'. Set linkID to 255.
				for (int j=i; j<ESP8266_MAX_SOCK_NUM; j++)
					_status.ipstatus[j].linkID = 255;
				return rsp;
			}
			else
			{
				p += strlen("+CIPSTATUS:");
				// Find linkID:
				uint8_t linkId = *p - 48;
				if (linkId >= ESP8266_MAX_SOCK_NUM)
					return rsp;
				_status.ipstatus[linkId].linkID = linkId;
				
				// Find type (p pointing at linkID):
				p += 3; // Move p to either "T" or "U"
				if (*p == 'T')
					_status.ipstatus[linkId].type = ESP8266_TCP_Connection;
				else if (*p == 'U')
					_status.ipstatus[linkId].type = ESP8266_UDP_Connection;
				else
					_status.ipstatus[linkId].type = ESP8266_TYPE_UNDEFINED_Connection;
				
				// Find remoteIP (p pointing at first letter or type):
				p += 6; // Move p to first digit of first octet.
				for (uint8_t j = 0; j < 4; j++)
				{
					char tempOctet[4];
					memset(tempOctet, 0, 4); // Clear tempOctet
					
					size_t octetLength = strspn(p, "0123456789"); // Find length of numerical string:
					
					strncpy(tempOctet, p, octetLength); // Copy string to temp char array:
					_status.ipstatus[linkId].remoteIP[j] = atoi(tempOctet); // Move the temp char into IP Address octet
					
					p += (octetLength + 1); // Increment p to next octet
				}
				
				// Find port (p pointing at ',' between IP and port:
				p += 1; // Move p to first digit of port
				char tempPort[6];
				memset(tempPort, 0, 6);
				size_t portLen = strspn(p, "0123456789"); // Find length of numerical string:
				strncpy(tempPort, p, portLen);
				_status.ipstatus[linkId].port = atoi(tempPort);
				p += portLen + 1;
				
				// Find tetype (p pointing at tetype)
				if (*p == '0')
					_status.ipstatus[linkId].tetype = ESP8266_CLIENT;
				else if (*p == '1')
					_status.ipstatus[linkId].tetype = ESP8266_SERVER;
			}
		}
	}
	
	return rsp;
}

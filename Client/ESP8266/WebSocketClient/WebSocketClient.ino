#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define USE_SERIAL Serial

const char *ssid           = "";
const char *password       = "";

const char *address_server = "192.168.9.66";
const int port_server      = 5000;

void websocket_event(WStype_t type, uint8_t * payload, size_t length) {      // Обработчик событий
	switch(type) {
		case WStype_DISCONNECTED: // Событие - отключение клиента/сервера
			USE_SERIAL.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED: { // Событие - подключение клиента/сервера 
			USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
			// отправка сообщения на сервер при подключении
			webSocket.sendTXT("Connected");
		}
			break;
		case WStype_TEXT: // Событие - получение текстовой информации от сервера 
			USE_SERIAL.printf("[WSc] get text: %s\n", payload);
			// отправка сообщения на сервер
			// webSocket.sendTXT("message here");
			break;
		case WStype_BIN: // Событие - получение байтовой байтовой информации от сервера
			USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
			hexdump(payload, length);
			// отправка сообщения на сервер
			// webSocket.sendBIN(payload, length);
			break;
    case WStype_PING:
      // pong will be send automatically
      USE_SERIAL.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      USE_SERIAL.printf("[WSc] get pong\n");
      break;
    }
}

void customization_websocket_clint() {
  // адрес, порт и url сервера
	webSocket.begin(address_server, port_server, "/");

	// обработчик событий
	webSocket.onEvent(websocket_event);

	// // use HTTP Basic Authorization this is optional remove if not needed
	// webSocket.setAuthorization("user", "Password");

	// try ever 5000 again if connection has failed
	webSocket.setReconnectInterval(5000);
  
  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  webSocket.enableHeartbeat(15000, 3000, 2);
}

void customization_wifi() {
  WiFiMulti.addAP(ssid, password);

	//WiFi.disconnect();
	while(WiFiMulti.run() != WL_CONNECTED) {
		delay(100);
	}
}

void customization_serial() {
  USE_SERIAL.begin(115200);

	USE_SERIAL.setDebugOutput(true);

	for(uint8_t t = 4; t > 0; t--) {
		USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
		USE_SERIAL.flush();
		delay(1000);
	}
}

void create_and_send_json_object(uint64_t now) {
  // creat JSON message for Socket.IO (event)
  DynamicJsonDocument doc(1024);
  JsonArray array = doc.to<JsonArray>();

  // add evnet name
  // Hint: socket.on('event_name', ....
  array.add("some_event");

  // add payload (parameters) for the event
  JsonObject param1 = array.createNestedObject();
  param1["now"] = now;
  param1["id_device"] = 1444;

  // JSON to String (serializion)
  String output;
  serializeJson(doc, output);

  // Send event
  webSocket.sendTXT(output);
  // socketIO.sendEVENT(output);

  // Print JSON for debugging
  Serial.println(output);
}

void setup() {
  customization_serial();
  customization_wifi();
  customization_websocket_clint();
}

unsigned long messageTimestamp = 0;

void loop() {
  webSocket.loop();

  uint64_t now = millis();

  // if(now - messageTimestamp > 2000) {
  //     messageTimestamp = now;
  //   create_and_send_json_object(now); 
  // }
}














/*  ============================================================================================================

    Sample code for create a websocket on an ESP8266 board.

  ============================================================================================================== */

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // V0.16.0  https://github.com/tzapu/WiFiManager

#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>

#include <Hash.h>

String old_value, value;

WiFiClient client;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int rCounter = 0;
int currentState;
int initState;

int angle = 0;
int aState;
int aLastState;

char html_template[] PROGMEM = R"=====(
<html lang="en">
   <head>
      <meta charset="utf-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <title>Esp8266 Socket IO</title>
      <style>
            .gauge {
                position: relative;
                border-radius: 50%/100% 100% 0 0;
                background-color: var(--color, #a22);
                overflow: hidden;
            }
            .gauge:before{
                content: "";
                display: block;
                padding-top: 50%;   /* ratio of 2:1*/
            }

            .gauge .mask {
                position: absolute;
                left: 20%;
                right: 20%;
                bottom: 0;
                top: 40%;
                background-color: #fff;
                border-radius: 50%/100% 100% 0 0;
            }
            .gauge .percentage {
                position:  absolute;
                top: -1px;
                left: -1px;
                bottom: 0;
                right: -1px;
                background-color: var(--background, #aaa);
                transform:rotate(var(--rotation)); 
                transform-origin: bottom center; 
                transition-duration: 600;
            }
            .gauge:hover {
                --rotation: 100deg;
            }
            .gauge .value {
                position:absolute; bottom:0%; left:0;   
                width:100%; 
                text-align: center;
            }

      </style>
      <script>
        socket = new WebSocket("ws:/" + "/" + location.host + ":81");
        socket.onopen = function(e) {  console.log("[socket] socket.onopen "); };
        socket.onerror = function(e) {  console.log("[socket] socket.onerror "); };
        socket.onmessage = function(e) {  
            console.log("[socket] " + e.data);
            document.getElementById("mrdiy_value").innerHTML = e.data;
            document.documentElement.style.setProperty("--rotation", e.data + "deg");
        };
      </script>
   </head>
   <body style="max-width:400px;margin: auto;font-family:Arial, Helvetica, sans-serif;text-align:center">
      <div id="gauge" class="gauge" style="width: 200px; --color:#5cb85c; --background:#e9ecef;">
            <div class="percentage"></div>
            <div class="mask"></div>
            <span class="value" id="mrdiy_value">50</span><em>%</em>
        </div>
   </body>
</html>
)=====";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[%u] Disconnected!\n", num);
        break;

    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        webSocket.sendTXT(num, "0");
    }
    break;

    case WStype_TEXT:
        Serial.printf("[%u] get Text: %s\n", num, payload);
        // send message to client
        // webSocket.sendTXT(num, "message here");
        // send data to all connected clients
        // webSocket.broadcastTXT("message here");
        break;

    case WStype_BIN:
        Serial.printf("[%u] get binary length: %u\n", num, length);
        hexdump(payload, length);
        // send message to client
        // webSocket.sendBIN(num, payload, length);
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    pinMode(D5, INPUT_PULLUP);
    pinMode(D6, INPUT_PULLUP);

    aLastState = digitalRead(D5);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    // reset saved settings
    // wifiManager.resetSettings();

    // set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("esp-8fi-manager");
    // or use this for auto generated name ESP + ChipID
    // wifiManager.autoConnect();

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.localIP());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    server.on("/", handleMain);
    server.onNotFound(handleNotFound);
    server.begin();
}

void handleMain()
{
    server.send_P(200, "text/html", html_template);
}
void handleNotFound()
{
    server.send(404, "text/html", "<html><body><p>404 Error</p></body></html>");
}

int lastValue = 0;

void loop()
{
    webSocket.loop();
    server.handleClient();

    compute_angle();

    value = (String)(rCounter);
    if (rCounter != lastValue)
        webSocket.broadcastTXT(value);
    lastValue = rCounter;
}

void compute_angle()
{
    aState = digitalRead(D5);

    if (aState != aLastState)
    {
        if (digitalRead(D6) != aState)
        {
            rCounter++;
            if (rCounter > 179)
                rCounter = 179;
        }
        else
        {
            rCounter--;
            if (rCounter < 0)
                rCounter = 0;
        }
        Serial.print("Position: ");
        Serial.print(rCounter);
        Serial.print("deg");
        Serial.println("");
    }
    aLastState = aState;
}


#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>

#define WIFI_AP ""
#define WIFI_PASSWORD ""

String TOKEN = "Wod6D3AlxIDyfR7btpvg";  //9jPma5EpXY0blSVNot3P
char thingsboardServer[] = "mqtt.thingcontrol.io";

//static const char *fingerprint PROGMEM = "69 E5 FE 17 2A 13 9C 7C 98 94 CA E0 B0 A6 CB 68 66 6C CB 77"; // need to update every 3 months
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long startTeleMillis;
unsigned long starSendTeletMillis;
unsigned long currentMillis;
const unsigned long periodCallBack = 1000;  //the value is a number of milliseconds
const unsigned long periodSendTelemetry = 10000;  //the value is a number of milliseconds


WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
String downlink = "";
char *bString;
int PORT = 8883;
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {

  Serial.begin(115200);

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("@Thingcontrol_AP")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
    delay(1000);
  }

//  wifiClient.setFingerprint(fingerprint);
  client.setServer( thingsboardServer, PORT );
  client.setCallback(callback);
  reconnectMqtt();

  Serial.print("Start..");
  startMillis = millis();  //initial start time

}

void loop()
{
  status = WiFi.status();
  if ( status == WL_CONNECTED)
  {
    if ( !client.connected() )
    {
      reconnectMqtt();
    }
    client.loop();
  }

  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)


  //check call back to connect Switch
  if (currentMillis - startMillis >= periodCallBack)  //test whether the period has elapsed
  {
    processCalled();
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }

  //send telemetry
  if (currentMillis - starSendTeletMillis >= periodSendTelemetry)  //test whether the period has elapsed
  {
    char json[] = "{\"tem1\":33,\"tem2\":34,\"tem3\":35,\"hum1\":88,\"lux1\":888,\"pres\":100,\"nh3\":1,\"co2\":300,\"led1\":1}";
    processTele(json);
    //
    starSendTeletMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
}


void getMac()
{
  Serial.println("OK");
  Serial.print("+TOKEN: ");
  Serial.println(WiFi.macAddress());
}

void viewActive()
{
  Serial.println("OK");
  Serial.print("+:WiFi, ");
  Serial.println(WiFi.status());
  if (client.state() == 0)
  {
    Serial.print("+:MQTT, [CONNECT] [rc = " );
    Serial.print( client.state() );
    Serial.println( " : retrying]" );
  }
  else
  {
    Serial.print("+:MQTT, [FAILED] [rc = " );
    Serial.print( client.state() );
    Serial.println( " : retrying]" );
  }
}

void setWiFi()
{
  Serial.println("OK");
  Serial.println("+:Reconfig WiFi  Restart...");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  if (!wifiManager.startConfigPortal("ThingControlCommand"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
    delay(5000);
  }

  Serial.println("OK");
  Serial.println("+:WiFi Connect");

//  wifiClient.setFingerprint(fingerprint);
  client.setServer( thingsboardServer, PORT );  // secure port over TLS 1.2
  client.setCallback(callback);
  reconnectMqtt();
}

void processCalled()
{
  Serial.println("OK");
  Serial.print("+:");
  Serial.println(downlink);
}



void processAtt(char jsonAtt[])
{

  char *aString = jsonAtt;

  Serial.println("OK");
  Serial.print(F("+:topic v1/devices/me/attributes , "));
  Serial.println(aString);

  client.publish( "v1/devices/me/attributes", aString);
}

void processTele(char jsonTele[])
{
  char *aString = jsonTele;
  Serial.println("OK");
  Serial.print(F("+:topic v1/devices/me/telemetry , "));
  Serial.println(aString);
  client.publish( "v1/devices/me/telemetry", aString);
}

void processToken()
{
  char *aString;

  //  aString = cmdHdl.readStringArg();

  Serial.println("OK");
  Serial.print("+:TOKEN , ");
  Serial.println(TOKEN);
//  TOKEN = aString;

  reconnectMqtt();
}

void unrecognized(const char *command)
{
  Serial.println("ERROR");
}

void callback(char* topic, byte* payload, unsigned int length)
{

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  String methodName = String((const char*)data["method"]);
  Serial.println(methodName);

  if (methodName.startsWith("setValue"))
  {
    char json[length + 1];
    strncpy (json, (char*)payload, length);
    json[length] = '\0';

    downlink = json;
  }
  else
  {
    downlink = "";
  }
}

void reconnectMqtt()
{
  if ( client.connect("Thingcontrol_AT", TOKEN.c_str(), NULL) )
  {
    Serial.println( F("Connect MQTT Success."));
    client.subscribe("v1/devices/me/rpc/request/+");
  }

}

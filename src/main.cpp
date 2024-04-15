#include <Arduino.h>
// #include <NeoPixelBus.h>
#include <Adafruit_NeoPixel.h>
#include <painlessMesh.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>

#include "team_config.h"
#include "neopixel_config.h"

#define NUMLEDS 69
const uint8_t PIXEL_PIN = D7;
bool alive = false;
int brightness = 64;
bool isRunning = false;
const uint8_t sensoren = D5;
int TeamID;
uint32_t boundtagger = 0; // if this value is zero,no tagger is bound
IRrecv sensor(sensoren);
decode_results results;

Scheduler meshScheduler;
painlessMesh Lasermesh;
// std::list<unsigned int> nodes;

// NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(NUMLEDS, LEDpin);
Adafruit_NeoPixel pixels(LED_NUM, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// put function declarations here:
void onchangedConnection();
void onConnection(uint32_t nodeID);
void onReceive(uint32_t from, String &msg);
void decode();
void ConnectTagger();

Task Task_decode(100, TASK_FOREVER, &decode);

Task Task_Connect_Tagger(100, TASK_FOREVER, &ConnectTagger);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  // pixels.begin();

  Lasermesh.setDebugMsgTypes(ERROR | DEBUG | STARTUP); // set before init() so that you can see error messages
  Lasermesh.init("Lasermesh", "Password", &meshScheduler, 5555);
  Lasermesh.onChangedConnections(&onchangedConnection);
  Lasermesh.onNewConnection(&onConnection);
  Lasermesh.onReceive(&onReceive);

  sensor.enableIRIn();

  Serial.printf("NodeId = %u", Lasermesh.getNodeId());
  // while(true){
  //   Serial.print(".");
  //   delay(250);
  // }
  meshScheduler.addTask(Task_decode);
  meshScheduler.addTask(Task_Connect_Tagger);
  Serial.println("Enabling Vest connect!");
  Task_Connect_Tagger.enable();
}
void loop()
{
  // put your main code here, to run repeatedly:
  Lasermesh.update();
}

// put function definitions here:
void onchangedConnection()
{
  Serial.printf("Changed connections\n");
  if (isRunning == false)
  {
    players.clear();
    for (uint32_t node : Lasermesh.getNodeList(true))
    {
      players.push_back(node);
    }
    players.shrink_to_fit();
    std::sort(players.begin(), players.end());
    Serial.printf("Player id is: %u \n", std::distance(players.begin(), std::find(players.begin(), players.end(), Lasermesh.getNodeId())));
  }
}
void onConnection(uint32_t nodeID)
{
  Serial.printf("New Connection from %u\n", nodeID);
  Serial.printf("--> startHere: New Connection, %s\n", Lasermesh.subConnectionJson(true).c_str());
}
void onReceive(uint32_t from, String &msg)
{
  Serial.printf("Message from %u :", from);
  Serial.println(msg);
  if (isRunning == true)
  { // Messages during the game like downtime
    if (msg == "dead" && from == boundtagger)
    {
      alive = false;
      pixels.fill(); // turn off everything.
      pixels.show();
    }
    else if (msg == "alive" && from == boundtagger)
    {
      alive = true;
      pixels.fill(TeamList[TeamID].color);
      pixels.show();
    }
  }
  else
  {

    // Messages sent before gamestart eg. binding a tagger.
    if (msg == "Start_Game" && boundtagger != 0)
    {
      isRunning = true;
    }
  }
}
void decode()
{

  // Serial.println("Trying to decode");

  if (sensor.decode(&results)/* && alive*/)
  {

    Serial.println("#################");
    Serial.print("Decode Type:");
    Serial.println(results.decode_type);

    Serial.print("Team:");
    Serial.println(results.command >> 4);

    Serial.print("Player:");
    Serial.println(results.address);

    Serial.print("Dammage:");
    Serial.println(results.command & 0B001111);

    if (results.decode_type == MILESTAG2)
    {

      Lasermesh.sendSingle(boundtagger, "IR-Data:" + String(results.value));
    }
    sensor.resume();
  }
}
void ConnectTagger()
{
  if (Task_Connect_Tagger.getRunCounter() % 15 == 1)
  {
    Serial.println("Still searching for tagger who wants to connect!");
  }
  if (sensor.decode(&results))
  {

    Serial.println("#################");
    Serial.print("Decode Type:");
    Serial.println(results.decode_type);

    Serial.print("Team:");
    Serial.println(results.command >> 4);

    Serial.print("Player:");
    Serial.println(results.address);

    Serial.print("Dammage:");
    Serial.println(results.command & 0B001111);
    if (results.decode_type == MILESTAG2)
    {
      boundtagger = players[results.address];
      Serial.printf("Connecting to Player: %u     NodeIp is: %u\n", results.address, players[results.address]);

      TeamID = results.command >> 4;
      Serial.printf("Joining team: %i\n", results.command >> 4);
      Serial.println("Now enabling normal decoding, have fun!");
      Task_decode.enable();
      Task_Connect_Tagger.disable();
    }
    sensor.resume();
  }
}
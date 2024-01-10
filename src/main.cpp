#include <Arduino.h>
#include <NeoPixelBus.h>
#include <painlessMesh.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#define sensoren D1
#define NUMLEDS 69
#define LEDpin 11
#define meshport 5555
#define ssid "Lasertag"
#define mesh_password "PWA_Lasertag"
bool alive = false;
int brightness = 64;
bool gamerunnig = false;
RgbColor Teamcoulor[4] = {
    RgbColor(brightness, 0, 0),
    RgbColor(0, 0, brightness),
    RgbColor(brightness / 2, brightness / 2, 0),
    RgbColor(0, brightness, 0),
};
int TeamID;
uint32_t boundtagger = 0; // if this value is zero,no tagger is bound
IRrecv sensor(sensoren);
decode_results results;

Scheduler meshScheduler;
painlessMesh Lasermesh;
std::list<unsigned int> nodes;
uint32_t *node_array;
int size;

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(NUMLEDS, LEDpin);

// put function declarations here:
void onchangedConnection();
void onConnection(uint32_t nodeID);
void onReceive(uint32_t from, String &msg);
void decode();
void ConnectTagger();
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  strip.Begin();

  Lasermesh.setDebugMsgTypes(ERROR | DEBUG | STARTUP); // set before init() so that you can see error messages
  Lasermesh.init(ssid, mesh_password, &meshScheduler, meshport);
  Lasermesh.onChangedConnections(&onchangedConnection);
  Lasermesh.onNewConnection(&onConnection);
  Lasermesh.onReceive(&onReceive);

  sensor.enableIRIn();

  while(gamerunnig==false && boundtagger==0){
    ConnectTagger();
    if(boundtagger!=0){
    Lasermesh.sendSingle(boundtagger,"bound");
  }
  }
}
void loop()
{
  // put your main code here, to run repeatedly:
}

// put function definitions here:
void onchangedConnection(){
nodes = Lasermesh.getNodeList(true);
  size = nodes.size();
  int k = 0;
  node_array = new uint32_t[size];
  for (int const &i : nodes)
  {
    node_array[k++] = i;
  }
  std::sort(node_array, node_array + size);
  Serial.println("Sorting finished!");

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
  if (gamerunnig == true)
  { // Messages during the game like downtime
    if (msg == "dead")
    {
      alive = false;
      for (int i = 0; i < NUMLEDS; i++)
      {
        strip.SetPixelColor(i, 0);
      }
      strip.Show();
    }else if(msg=="alive"){
      alive=true;
            for (int i = 0; i < NUMLEDS; i++)
      {
        strip.SetPixelColor(i, Teamcoulor[TeamID]);
      }
      strip.Show();
    }
  }else{
  
    //Messages sent before gamestart eg. binding a tagger.
    if(msg=="Start_Game"&& boundtagger!=0){
      gamerunnig=true;
    }
  }
}
void decode()
{

  // Serial.println("Trying to decode");

  if (sensor.decode(&results)&& alive)
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

    if (results.decode_type == 97)
    {

      Lasermesh.sendSingle(boundtagger, String(results.value));
    }
    sensor.resume();
  }
}
void ConnectTagger(){
  if(sensor.decode(&results)){
    if(results.decode_type==97){
      boundtagger=node_array[results.address];
      TeamID=results.command >>4;
      sensor.resume();
    }
  }
}
#include "Config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include "PubSubClient.h"
#include <WiFiClientSecure.h>
#include "time.h"
#include "SPI.h"          // for communication with the TFT LCD screen
#include "V_Display.h"     // for TFT LCD screen register
#include <XPT2046_Calibrated.h> // touchscreen driver
#include "V_Graph.h"

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 25
#define TEMPERATURE_PRECISION 9 // Lower resolution

// Used Pins on Display
#define TFT_CS   5
#define TFT_DC   4
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  22
#define TFT_MISO 19
#define TFT_LED  15

#define HAVE_TOUCHPAD
#define TOUCH_CS 14
#define TOUCH_IRQ 2

// Parameters for touchscreen
#define ILI9341_ULTRA_DARKGREY    0x632C
#define MINPRESSURE 10
#define MAXPRESSURE 2000
// Measurement range may need to be calibrated
#define TS_MINX 165
#define TS_MINY 200
#define TS_MAXX 3707
#define TS_MAXY 3889
#define TFT_ROTATION 1U

#define TS_HOLD 1000 // time before a touch becomes a hold in ms

// drivers instances
V_Display dsply = V_Display(); // TFT LCD screen object
XPT2046_Calibrated touch(TOUCH_CS, TOUCH_IRQ);

// current locations
int tsx, tsy, tsxraw, tsyraw;
// current touch state
bool tsdown = false;
// current screen orientation
uint8_t rotation = 0;

V_Graph graph(&dsply, 10, 10, 220, 220, true);
DatasetCollection dsc;

bool touch_active = false;
unsigned long touch_last;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found
int sens_num;
#define NUM_EXPECTED_DEVICES 10

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address


//WiFiClientSecure wifiClient;
WiFiClient wifiClient;

// MQTT
const char* mqtt_server = "192.168.178.30";  // IP of the MQTT broker
std::vector<String> temperature_topics = {
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_hahn/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_oben/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_mitte/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_unten/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_eimer/val_temperature",
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_hahn/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_oben/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_mitte/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_unten/val_temperature", 
                                          "top_beer/tsk_brewing/loc_brewkitchen/sen_eimer/val_temperature"
                                          };
DeviceAddress temperature_sensors_addr[NUM_EXPECTED_DEVICES] = {
                                          {0x28, 0xAA, 0xB3, 0xBE, 0x4F, 0x14, 0x01, 0xEF},
                                          {0x28, 0xA8, 0x54, 0x94, 0x97, 0x09, 0x03, 0x84},
                                          {0x28, 0xAA, 0x92, 0xC9, 0x4F, 0x14, 0x01, 0xF4},
                                          {0x28, 0xAA, 0x07, 0xCA, 0x4F, 0x14, 0x01, 0x19},
                                          {0x28, 0x48, 0x04, 0x94, 0x97, 0x08, 0x03, 0xFC},
                                          {0x28, 0x21, 0x2E, 0xA2, 0x13, 0x21, 0x01, 0x9E},
                                          {0x28, 0x26, 0xB4, 0x82, 0x13, 0x21, 0x01, 0x61},
                                          {0x28, 0x02, 0xF8, 0x17, 0x13, 0x21, 0x01, 0xA3},
                                          {0x28, 0x39, 0xB1, 0x9E, 0x13, 0x21, 0x01, 0xE6},
                                          {0x28, 0xDA, 0x6B, 0xBF, 0x13, 0x21, 0x01, 0x8A}
                                          };
std::vector<String> temperature_sensors_name = {
                                          "28AAB3BE4F1401EF",
                                          "28A8549497090384",
                                          "28AA92C94F1401F4",
                                          "28AA07CA4F140119",
                                          "28480494970803FC",
                                          "28212EA21321019E",
                                          "2826B48213210161",
                                          "2802F817132101A3",
                                          "2839B19E132101E6",
                                          "28DA6BBF1321018A"
                                          };
std::vector<uint16_t> temperature_sensors_graph_colors = {
                                          TFT_GREEN,
                                          TFT_BLUE,
                                          TFT_WHITE,
                                          TFT_RED,
                                          TFT_YELLOW,
                                          TFT_GREEN,
                                          TFT_BLUE,
                                          TFT_WHITE,
                                          TFT_RED,
                                          TFT_YELLOW
                                          };
std::vector<int> temperature_sensors_graph_ids;

//const char* temperature_topics = ["top_beer/tsk_brewing/sen_temp1/val_temperature", "top_beer/tsk_brewing/sen_temp2/val_temperature"];

PubSubClient client(mqtt_server, 1883, wifiClient); 
//PubSubClient client(wifiClient);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

String starttime;
unsigned long startmillis;

inline bool touched();

void setup(void)
{
  // start serial port
  Serial.begin(9600);

  initializeLCD();
  touch.begin();
  connectToWiFi();

  // Reset current values
  tsx = 0;
  tsy = 0;
  tsxraw = 0;
  tsyraw = 0;
  tsdown = false;
  rotation = TFT_ROTATION;

  //delay(1000*2);

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
  client.setKeepAlive(60);

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  starttime = LocalTimeString();  
  startmillis = millis();

  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
  
  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      
      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);
      
      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
      
      Serial.print("Resolution actually set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
      Serial.println();

      temperature_sensors_graph_ids.push_back(dsc.create_dataset());
    }
    else{
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  connect_MQTT();
  //Serial.setTimeout(2000);  
  
  // Loop through each device, print out temperature data
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      sens_num = NUM_EXPECTED_DEVICES + 1;
      for(int j=0;j<NUM_EXPECTED_DEVICES; j++)
      {
        int n;

        n = memcmp(tempDeviceAddress, temperature_sensors_addr[j], sizeof(tempDeviceAddress));
        if(n == 0)
        {
          sens_num = j;
        }
      }
      
      // Output the device ID
      Serial.print("Temperature for device: ");
      printAddress(tempDeviceAddress);
      Serial.println("");
      
      // It responds almost immediately. Let's print out the data
      printTemperature(tempDeviceAddress); // Use a simple function to print out the data

      float tempC = sensors.getTempC(tempDeviceAddress);

      if(sens_num < NUM_EXPECTED_DEVICES + 1)
      {
        const char* temperature_topic;
        temperature_topic = temperature_topics[sens_num].c_str();

        Serial.print("MQTT Topic: ");
        Serial.println(temperature_topic);

        String message;
        message = "{\"value\": [" + String(tempC) + "], \"unit\": \"DEG\", \"timestamp\": \"" + LocalTimeString() + "\", \"measurement_start\": \"" + starttime + "\"}";
        //message = "{\"fields\": {\"value\": [" + String(tempC) + "]}}";
        //message = "test";
        Serial.println(message);

        dsc.add_data_to_dataset(temperature_sensors_graph_ids[i], static_cast<float>((millis()-startmillis)/1000), tempC);

        //client.beginPublish(temperature_topic, message.length(), false);

        //client.print(message.c_str());

        // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
        //if (client.endPublish()) {
        //  Serial.println("Temperature sent!");
        //}

        // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
        if (client.publish(temperature_topic, message.c_str())) {
          Serial.println("Temperature sent!");
        }
        // Again, client.publish will return a boolean value depending on whether it succeeded or not.
        // If the message failed to send, we will try again, as the connection may have broken.
        else {
          Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
          client.connect(clientID, mqtt_username, mqtt_password);
          delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
          client.publish(temperature_topic, message.c_str());
        }
      }
      else
      {
        Serial.println("No associated MQTT topic found");
      }
    } 
    //else ghost device! Check your power requirements and cabling
  }

  if(numberOfDevices > 0)
  {
    dsply.fillScreen(TFT_BLACK);
    graph.set_limits_auto(&dsc);
    graph.Axis();
  }

  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      sens_num = NUM_EXPECTED_DEVICES + 1;
      for(int j=0;j<NUM_EXPECTED_DEVICES; j++)
      {
        int n;

        n = memcmp(tempDeviceAddress, temperature_sensors_addr[j], sizeof(tempDeviceAddress));
        if(n == 0)
        {
          sens_num = j;
        }
      }
      graph.draw_dataset(dsc.get_ds_p(temperature_sensors_graph_ids[i]), temperature_sensors_graph_colors[sens_num]);
    }
  }

  client.disconnect();  // disconnect from the MQTT broker
  delay(1000*10);       // print new values every 10 Seconds
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT(){
    // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }

}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

String LocalTimeString()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "0000-00-00T00:00:00.000Z";
  }

  char buffer [80];
  strftime (buffer,80,"%FT%T.000Z",&timeinfo);
  return String(buffer);
}

void initializeLCD() {
  /*  This function is used to turn ON the LCD and to initialize dsply object
   *  Arguments:
   *  none
   *  
   *  Returns:
   *  nothing
   */
   
  // turn ON the dispaly
  pinMode(TFT_LED, OUTPUT);    // Backlight pin of the display is connecte to this pin of ESP32
  digitalWrite(TFT_LED, LOW);  // we have to drive this pin LOW in order to turn ON the display
  delay(1);               // delay for 1ms

  dsply.begin();
  dsply.setRotation(1);
  dsply.fillScreen(TFT_BLACK);    //  fill the screen with black color
  //dsply.setTextColor(TFT_GREEN);  //  set the text color
  
  //dsply.setFreeFont(FSS9);      //  you can use some other font, 
                                  //  but you have to change positions of every element, 
                                  //  because for different font there are different text sizes
}

void connectToWiFi() {
  /*  This function is used to connect to local WiFi network
   *  Arguments:
   *  none
   *  
   *  Retruns:
   *  nothing
   */

  dsply.setTextColor(TFT_GREEN);
   
  WiFi.begin(ssid, pass);
  delay(500);
  dsply.printText(3, 3, "connecting...", 1, 1); delay(1000);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(isTouch() > 0)
    {
      WiFi.begin(ssid, pass);
      delay(500);
    }
  }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  dsply.printText(3, 23, "WiFi Connected", 1, 1); delay(1000);
  printWiFiStatus(); // call the function to display wifi status on the screen
}

void printWiFiStatus() {
  /*  This function is used to print the status of wifi connection
   *  Arguments:
   *  none
   *  
   *  Returns:
   *  nothing
   */

  // print the SSID of the network you're attached to:
  dsply.printText(3, 33, "SSID: ", 1, 1); delay(10);
  dsply.printText(35, 33, String(WiFi.SSID()), 1, 1); delay(10);

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  dsply.printText(3, 53, "IP Address: ", 1, 1); delay(10);
  
  String cont = "";
  for (int i = 0; i < 4; i++) {
    cont += ip[i];
    if (i == 3) {
      break;
    }
    cont += ".";
  }
  dsply.printText(70, 53, cont, 1, 1); delay(10);

  // print the received signal strength:
  dsply.printText(3, 73, "Signal strength (RSSI):", 1, 1); delay(10);
  dsply.printText(150, 73, String(WiFi.RSSI()), 1, 1); delay(10);
  dsply.printText(170, 73, "dBm", 1, 1); delay(10);
}

int isTouch() // 0 = no touch or old touch, 1 = new touch, 2 = hold, 3 = release
{
  if (!touch_active)
  {
    if (touched())
    {
      touch_active = true;
      touch_last = millis();
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if (touched())
    {
      if (touch_last - millis() > TS_HOLD)
      {
        return 2;
      }
      else
      {
        return 0;
      }      
    }
    else
    {
      touch_active = false;
      return 3;
    }
  }
}

inline bool touched() {
  if (touch.tirqTouched()) {
    return touch.touched();
  }
  return false;
}
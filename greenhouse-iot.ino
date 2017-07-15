/*
 * Links:
 * https://www.carnetdumaker.net/articles/utiliser-des-leds-rgb-avec-une-carte-arduino-genuino/
 * 
 */

#include <Ethernet.h>
#include "DHT.h"

#define WDWAIT      20000  // Timeout de reponse serveur IoT
#define WDREAD      5000   // Timeout de lecture serveur IoT
#define REQUEST_INTERVAL 5 // in seconds
#define RETRY_INTERVAL 5 // interval when the request have failed

#define MAX_RESPONSE  1024

#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11

#define NB_SENSORS 4

// PINS
// ANALOG
const int PIN_SENSOR_SOIL_HUMIDITY = A0; // soil humidity
const int PIN_SENSOR_LUMINOSITY = A1; // luminosity
// PWM
const int PIN_LED_R = 9;
const int PIN_LED_G = 10;
const int PIN_LED_B = 11;

// INIT DHT11
DHT dht(DHTPIN, DHTTYPE);

// NETWORK INTERFACE CLIENT
EthernetClient client;

// API CONFIGURATION
boolean http_debug_enabled = true;
const char* server = "dev.greenduino.info";
int port = 80;
char* plant_id = "656a8ac1-bebb-4b7a-acf8-dcdf1ca88f06";


// ROUTES
const char *route = "sensors";

// REQUESTS CONFIGURATION
char body[2048];
char buffer[2048];
char response[MAX_RESPONSE];

char* sensors[] = {"soil_humidity", "luminosity", "air_humidity", "temperature"};
int values[NB_SENSORS];
// -------------------------------------------------------------------
// -- BEGINNING OF THE CODE ------------------------------------------

// INITIALIZATION, executed only once
void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PIN_SENSOR_SOIL_HUMIDITY, INPUT); // soil  humidity
  pinMode(PIN_SENSOR_LUMINOSITY, INPUT); // luminosity
  pinMode(PIN_LED_R, OUTPUT); // RGB LED -> R
  pinMode(PIN_LED_G, OUTPUT); // RGB LED -> G
  pinMode(PIN_LED_B, OUTPUT); // RGB LED -> B
  displayColor(255,0,0);
  if(http_debug_enabled == true) {
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
  }
  Serial.println("Initializing Ethernet...");
  Ethernet.begin();
  Serial.println("Ethernet shield is ready !");
  displayColor(0,0,255);
  
}

/////////////////////////////// MAIN CODE ///////////////////////////////////////
void loop() {
  displayColor(0, 0, 255);
  delay(1000);
  sensors_to_json(body);
  if(send_values(body) == true) {
    displayColor(0, 255, 0);
    delay(1000*REQUEST_INTERVAL);
  } else {
    displayColor(255, 0, 0);
    delay(1000*RETRY_INTERVAL);
  }
  Serial.println("============================================================");
}

//////////////////////// SENSORS /////////////////

void sensors_to_json(char* body) {

  int values[] = {analogRead(PIN_SENSOR_SOIL_HUMIDITY), analogRead(PIN_SENSOR_LUMINOSITY), dht.readHumidity(), dht.readTemperature()};
  char entry[255];
  sprintf(body, "{");
  
  for(int i = 0; i < NB_SENSORS; i++) {
    sprintf(entry, "\"%s\":\"%f\"%c", sensors[i], (float) values[i], (i < NB_SENSORS - 1)?',':'}');
    strcat(body, entry);
  }
}

//////////////////////// LEDs ////////////////////
void displayColor(byte r, byte g, byte b) {
  // RGB LED with a common cathode
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

//////////////////////// NETWORK /////////////////

void print_request(char* buffer) {
  client.println(buffer);
  if(http_debug_enabled) {
    Serial.println(buffer);
  }
}

boolean send_values(char* body)
{
  
  if(connect() == false) {
    return false;
  }
  send_post_sensor(body);
  if( timeout_response() == false )
  {  
    read_response();
  }
  return end_request();
}


// Establishing a connection with the server
boolean connect(void)
{
  Serial.println("connecting...");
  if (client.connect(server, port))
  {
    Serial.println("connected");
    return true;
   
  } else {
      Serial.println("connection failed");
      return false;
  }
}

// GET request
void GET_request(char* buffer, const char* route)
{  
  // POST URI
  //sprintf(buffer, "GET /tables/%s HTTP/1.1", table_etat);
  sprintf(buffer, "GET /%s/ HTTP/1.1", route);
  print_request(buffer);

  // Authorization header
  //sprintf(buffer, "Authorization: %s", api_token);
  //print_request(buffer);
  
  // Host header
  sprintf(buffer, "Host: %s", server);
  print_request(buffer);  

  // Content type
  print_request("Content-Type: application/json");

  // Content length
  print_request("Content-Length: 0");
  
  // End of headers
  client.println();
  Serial.println();
}

// Ajout d'une valeur dans la table movdata
void send_post_sensor(char* body)
{  
  // POST URI
  sprintf(buffer, "POST /%s HTTP/1.1", route);
  print_request(buffer);
    
  // Host header
  sprintf(buffer, "Host: %s", server);
  print_request(buffer);

  // plant_id header
  sprintf(buffer, "PlantId: %s", plant_id);
  print_request(buffer);

  // JSON content type
  print_request("Content-Type: application/json");

  // Content length
  // Content length
  client.print("Content-Length: ");
  client.println(strlen(body));
  Serial.print("Content-Length: ");
  Serial.println(strlen(body));

  // Authorization header
  //sprintf(buffer, "Authorization: %s", api_token);
  Serial.println();
  client.println(); 
  
  // Request body
  print_request(body);
}

// Attente de réponse du serveur
boolean timeout_response()
{
  boolean wait_timeout = false;
  unsigned long Now = 0;
  unsigned long Debut = millis();
  
  do
  {
    if (!client.connected()) { 
      return false;
    } 
    Now = millis();
  // Tant qu'il n'y a pas de données à lire et que le timeout n'est pas atteind
  }while (!client.available() && (Now - Debut < WDWAIT )); 
  
  if(Now - Debut > WDWAIT )
  {
    wait_timeout = true;
  }
 
  return wait_timeout;
}

// Lecture de la réponse et affichage sur la console série
void read_response()
{ 
  unsigned long Debut = 0;
  unsigned long Now = 0;
  
  Debut = millis();
  do{
    char c = client.read();
    Serial.print(c);
    
    Now = millis();
  // Tant q'u'il y a des données à lire et que le timeout n'est pas atteint
  } while (client.available() && (Now - Debut < WDREAD )); 
  
  Serial.print("\n");
}

// Lecture de la réponse et affichage sur la console série
void read_response_date()
{ 
  unsigned long Debut = 0;
  unsigned long Now = 0;
  int index = 0;
  
  Debut = millis();
  do
  {
    char c = client.read();
    //Serial.print(c);
  if (index < MAX_RESPONSE)
  {
      response[index++] = c;
    }
    
    Now = millis();
  // Tant qu'il y a des données à lire et que le timeout n'est pas atteind
  } while (client.available() && (Now - Debut < WDREAD )); 
  
  response[index] = 0;
}

// fermeture connexion
boolean end_request()
{
  client.stop();
  return true;
}


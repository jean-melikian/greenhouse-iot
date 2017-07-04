/*
 * Links:
 * https://www.carnetdumaker.net/articles/utiliser-des-leds-rgb-avec-une-carte-arduino-genuino/
 */

#include <Ethernet.h>

#define WDWAIT      20000  // Timeout de reponse serveur IoT
#define WDREAD      5000   // Timeout de lecture serveur IoT
#define REQUEST_INTERVAL 5 // in seconds
#define RETRY_INTERVAL 5 // interval when the request have failed

#define MAX_RESPONSE  1024

#define NB_SENSORS 2

// PINS
// ANALOG
const int PIN_SENSOR_HYGROMETER = A0; // soil humidity
const int PIN_SENSOR_LUMINOSITY = A1; // luminosity
// PWM
const int PIN_LED_R = 9;
const int PIN_LED_G = 10;
const int PIN_LED_B = 11;

// SENSORS VALUES
int humidity = 0;
int luminosity = 0;

// NETWORK INTERFACE CLIENT
EthernetClient client;

// API CONFIGURATION
boolean http_debug_enabled = true;
const char* server = "192.168.1.14";
int port = 3000;
char* api_token = "Basic a362b247ad991a9da225c0d31549480f4c727764050e633cff3bcc7d390d3ed9";

// ROUTES
const char *route = "sensors";

// REQUESTS CONFIGURATION
char body[2048];
char buffer[2048];
char response[MAX_RESPONSE];

char* sensors[] = {"hygrometer", "luminosity"};
int values[NB_SENSORS];
// -------------------------------------------------------------------
// -- BEGINNING OF THE CODE ------------------------------------------

// INITIALIZATION, executed only once
void setup() {
  Serial.begin(9600);
  pinMode(PIN_SENSOR_HYGROMETER, INPUT); // soil  humidity
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
char* fetch_hygrometry() {
  
  Serial.print(humidity); Serial.print(" - ");
  
  if(humidity >= 1000) {
   Serial.println("Hygrometer is not in the Soil or DISCONNECTED");
  }
  if(humidity < 1000 && humidity >= 600) { 
   Serial.println("Soil is DRY");
  }
  if(humidity < 600 && humidity >= 370) {
   Serial.println("Soil is HUMID"); 
  }
  if(humidity < 370) {
   Serial.println("Hygrometer in WATER");
  }
  
}
void sensors_to_json(char* body) {

  int values[] = {analogRead(PIN_SENSOR_HYGROMETER), analogRead(PIN_SENSOR_LUMINOSITY)};
  char entry[255];
  sprintf(body, "{");
  
  for(int i = 0; i < NB_SENSORS; i++) {
    sprintf(entry, "\"%s\":\"%d\"%c", sensors[i], values[i], (i < NB_SENSORS - 1)?',':'}');
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
  sprintf(buffer, "Authorization: %s", api_token);
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


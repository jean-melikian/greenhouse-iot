/*
 * Links:
 * https://www.carnetdumaker.net/articles/utiliser-des-leds-rgb-avec-une-carte-arduino-genuino/
 */

#include <Ethernet.h>

#define WDWAIT      20000  // Timeout de reponse serveur IoT
#define WDREAD      5000   // Timeout de lecture serveur IoT
#define REQUEST_INTERVAL 5 // in seconds

#define MAX_RESPONSE  1024

// PINS
// ANALOG
const int PIN_SENSOR_HYGROMETER = A0; // soil humidity
const int PIN_SENSOR_LUX = A1; // luminosity
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
const char* server = "192.168.1.14";
int port = 3000;

// ROUTES
const char *table_name = "sensors";

// REQUESTS CONFIGURATION
char buffer[64];
char response[MAX_RESPONSE];

// -------------------------------------------------------------------
// -- BEGINNING OF THE CODE ------------------------------------------

// INITIALIZATION, executed only once
void setup() {
  Serial.begin(9600);
  pinMode(PIN_SENSOR_HYGROMETER, INPUT); // soil  humidity
  pinMode(PIN_SENSOR_LUX, INPUT); // luminosity
  pinMode(PIN_LED_R, OUTPUT); // RGB LED -> R
  pinMode(PIN_LED_G, OUTPUT); // RGB LED -> G
  pinMode(PIN_LED_B, OUTPUT); // RGB LED -> B
  displayColor(255,0,0);
  Serial.println("Initializing Ethernet...");
  Ethernet.begin();
  Serial.println("Ethernet shield is ready !");
  displayColor(0,255,0);
}

// MAIN CODE
void loop() {

  humidity = analogRead(PIN_SENSOR_HYGROMETER);
  luminosity = analogRead(PIN_SENSOR_LUX);
  
  /*
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
  */
  Serial.print("Hygrometer ");
  Serial.println(humidity);
  Trace_Sensor(humidity);
  //displayColor(255, 0, 0);
  delay(1000*REQUEST_INTERVAL);
  
  //digitalWrite(R, LOW);
}

//////////////////////// LEDs ////////////////////
void displayColor(byte r, byte g, byte b) {
  // As we are using a common cathod LED, we have to invert the bytes values to get the color we want
  // '~' inverts each of the word's bits, for example:
  // ~r == 255-r
  // OR
  // ~0b101 == 0b010
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}

//////////////////////// NETWORK /////////////////



// Envoi d'un TOP cycle
void Trace_Sensor(int value)
{
  connection_server();
  send_post_sensor(value);
  if( timeout_response() == false )
  {  
    read_response();
  }
  end_request();
}

// recupere la date sur le serveur AZURE
boolean get_response_date()
{
  if (connection_server())
  {
  send_request_date();
    if( timeout_response() == false )
    {  
      read_response_date();
    }
    end_request();
  }
  else
  return false;

  return true;
}

// Connexion au service mobile AZURE climsecours.azure-mobile.net
boolean connection_server(void)
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

// GET request to mobile AZURE
void send_request_date()
{  
  // POST URI
  //sprintf(buffer, "GET /tables/%s HTTP/1.1", table_etat);
  sprintf(buffer, "GET /tables/%s?&$top=1 HTTP/1.1", table_name);  // un seul etat...
  client.println(buffer);
  //Serial.println(buffer);

  // Host header
  sprintf(buffer, "Host: %s", server);
  client.println(buffer);
  //Serial.println(buffer);
    
  // Azure Mobile Services application key
  //sprintf(buffer, "X-ZUMO-APPLICATION: %s", ams_key);
  //client.println(buffer); 
  //Serial.println(buffer);

  // JSON content type
  client.println("Content-Type: application/json");
  //Serial.println("Content-Type: application/json");

  // Content length
  client.println("Content-Length: 0");
  //Serial.println("Content-Length: 0");
  
  // End of headers
  client.println();
  //Serial.println();
}

// Ajout d'une valeur dans la table movdata
void send_post_sensor(int value)
{  
  // POST URI
  sprintf(buffer, "POST /%s HTTP/1.1", table_name);
  client.println(buffer);
  Serial.println(buffer);
    
  // Host header
  sprintf(buffer, "Host: %s", server);
  client.println(buffer);
  Serial.println(buffer);
    
  // Azure Mobile Services application key
  //sprintf(buffer, "X-ZUMO-APPLICATION: %s", ams_key);
  //client.println(buffer); 
  //Serial.println(buffer);

  // JSON content type
  client.println("Content-Type: application/json");
  Serial.println("Content-Type: application/json");
    
  // POST body
  sprintf(buffer, "{\"sensor\": \"humidity\",\"value\": \"%d\"}", humidity);

  // Content length
  client.print("Content-Length: ");
  client.println(strlen(buffer));
  Serial.print("Content-Length: ");
  Serial.println(strlen(buffer));

  // Request header
  client.println();
  Serial.println();
    
  // Request body
  client.println(buffer);
  Serial.println(buffer); 
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
  // Tant q'u'il y a des données à lire et que le timeout n'est pas atteind
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
void end_request()
{
  client.stop();
}


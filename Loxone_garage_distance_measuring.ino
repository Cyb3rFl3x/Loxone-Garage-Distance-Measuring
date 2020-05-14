/*
 * A simple tool to measure the status of the garage door and send it with low traffic to a Loxone Smarthome.
 * It includes: Connecting to a local network, calculating the distance and sending it to the Loxone.
 */

// Libraries 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WIFI SETTINGS
#define WIFI_SSID           "wifi-ssid"
#define WIFI_PW             "wifi-password"

// LOXONE SETTINGS
#define LOXONE_IP           "url or ip from your loxone"
#define LOXONE_USER         "loxone user"
#define LOXONE_PW           "loxone pw"
#define LOXONE_DEVICENAME   "device name for loxone"

// SENSOR SETTINGS
#define DISTANCE_IF_CLOSED  100                           // Distance
#define TRIGGER             5                             // D1 == 5
#define ECHO                4                             // D2 == 4

// APPLICATION SETTINGS
#define FREQUENCY           100                           // In milliseconds  ( 1000 ms == 1 s )
#define DEBUG               true 


HTTPClient http;
int httpCode = 0;

/*
 * Function: connect_to_wifi
 * ----------------------------
 *   Uses WIFI_SSID and WIFI_PW to connect ESP8266 with local wifi.
 *   returns: NULL 
 */
void connect_to_wifi() {
    if(DEBUG) {
        delay(10);
        Serial.print(F("Connecting with: "));
        Serial.println(WIFI_SSID);
    }

    const char *ssid = WIFI_SSID;
    const char *pw = WIFI_PW;
    
    WiFi.begin(ssid, pw);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    
    if(DEBUG) {
        Serial.print(F("Connected with: "));
        Serial.println(WIFI_SSID);
        Serial.print(F("Local IP-Address is: "));
        Serial.println(WiFi.localIP());
    }
}

/*
 * Function: setup
 * ----------------------------
 *   Initialize modes of the pins and serial terminal bitrate.
 *   returns: NULL 
 */
void setup() {
    pinMode(TRIGGER, OUTPUT);       // Sets the TRIGGER Pin on ESP as an Output
    pinMode(ECHO, INPUT);           // Sets the ECHO    Pin on ESP as an Input

    Serial.begin(115200);           // Init Bitrate 
    Serial.println("");             // Linebreak Bugfix

    connect_to_wifi();

    if(DEBUG) {
        Serial.print(F("Sampling Frequency is "));
        Serial.print(FREQUENCY);
        Serial.println(F(" milliseconds"));
    }
}

/*
 * Function: check_status
 * ----------------------------
 *   Converts distance in a status.
 *   distance: Is the distance between sensor and door.
 *   returns (bool): Status True if door is open or False if door is closed.
 */
bool check_status(int distance) {
    bool open = true;

    if(distance <= DISTANCE_IF_CLOSED){
        open = true;
    } else {
        open = false;
    }
    
    return open;
}

/*
 * Function: check_status
 * ----------------------------
 *   Calculates the distance between door and sensor with the speed of sound.
 *   returns (int): The distance in cm between door and sensor (tol. +/- 4 cm).
 */
int measure_distance() {
    digitalWrite(TRIGGER, LOW);                 // Makes TRIGGER PIN low
    delayMicroseconds(2);                       // 2 micro second delay 

    digitalWrite(TRIGGER, HIGH);                // TRIGGER PIN to high
    delayMicroseconds(10);                      // TRIGGER PIN to high for 10 microseconds
    digitalWrite(TRIGGER, LOW);                 // TRIGGER PIN to low

    int duration = pulseIn(ECHO, HIGH);         // Read ECHO PIN, time in microseconds
    int distance = duration * 0.034 / 2;        // Calculating actual/real distance

    return distance;
}

/*
 * Function: send_to_loxone
 * ----------------------------
 *   Authorize against loxone and send status.
 *   status: The string which has to be send to loxone.
 *   returns: NULL
 */
void send_to_loxone(String status) {
    String url = LOXONE_IP;

    url.concat("/dev/sps/io/");
    url.concat(LOXONE_DEVICENAME);
    url.concat("/");
    url.concat(status);

    http.begin(url);
    http.setAuthorization(LOXONE_USER, LOXONE_PW);
    
    httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        if(DEBUG) {
            Serial.print("Garage status sent to Loxone. ");
        }
    }
    
    http.end();
}

/*
 * Function: loop
 * ----------------------------
 *   Measure distance, generate status and send status to loxone in a specific FREQUENCY
 *   returns: NULL
 */
void loop() {
    delay(FREQUENCY);

    int distance = measure_distance();
    bool is_open = check_status(distance);

    if(DEBUG) {
        delay(500);
        if(is_open == true) {
            Serial.print(F("Door is open"));
        } else {
            Serial.print(F("Door is closed"));
        }
        Serial.print(F(" with size of: "));
        Serial.println(distance);
    }

    bool lock = false;

    if(lock == false && is_open == true) {
        send_to_loxone(true);
        lock = true;
        if(DEBUG) {
            Serial.print(F("Switched to status open"));
        }

    } else if (lock == true && is_open == false) {
        send_to_loxone(false);
        lock = false;
        if(DEBUG) {
            Serial.print(F("Switched to status closed"));
        }

    }   
}

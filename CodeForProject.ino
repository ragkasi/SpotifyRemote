#include <Arduino.h>
#include <WiFi.h>
#include <pw.h>
#define DISABLE_AUDIOBOOKS
#define DISABLE_CATEGORIES
#define DISABLE_CHAPTERS
#define DISABLE_EPISODES
#define DISABLE_GENRES
#define DISABLE_MARKETS
#define DISABLE_PLAYLISTS
#define DISABLE_SEARCH
#define DISABLE_ALBUM
#define DISABLE_ALBUM
#define DISABLE_USER
#include <WiFiClientSecure.h>
#include <SpotifyEsp32.h>
#include <base64.h>
#include <LiquidCrystal_I2C.h>
#include <string>
using namespace Spotify_types;
JsonDocument filter;
JsonDocument filter2;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* PROGMEM SSID = ssid;
const char* PROGMEM PASSWORD = password;
const char* PROGMEM CLIENT_ID = clientid;
const char* PROGMEM CLIENT_SECRET = clientsecret;
const char* PROGMEM REFRESH_TOKEN = refreshtoken;

//Create an instance of the Spotify class Optional: you can set the Port for the webserver the debug mode(This prints out data to the serial monitor) and number of retries
Spotify sp(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);
String accessToken = "";
String current_track;
const int potPin = 35; // Potentiometer connected to
const int buttonPin = 14;  // Pause pin
const int buttonPin1 = 15;  // Previous pin
const int buttonPin2 = 16;  // Skip pin
// const int freq = 5000; // PWM frequency
const int resolution = 12; // PWM resolution (bits)
int currentButton = 0;
int lastButton = 0;

void setup() {
    Serial.begin(115200);
    // initialize the button pin as an input
    pinMode(buttonPin, INPUT_PULLUP);
    lcd.init();
    lcd.clear();
    lcd.backlight();
    // Connect to your wifi
    connect_to_wifi();
    // Start the webserver
    sp.begin();
    // Wait for the user to authenticate
    while(!sp.is_auth()){
        // Handle the client, this is necessary otherwise the webserver won't work
        sp.handle_client();
    }
    Serial.println("Authenticated");
    Serial.println(sp.current_artist_names());
    if (requestAccessToken()) {
    Serial.println("Access token obtained successfully: " + accessToken);
  } else {
    Serial.println("Failed to obtain access token");
  }
  // Create tasks for two different loops
  xTaskCreate(
    // Function to implement the task
    mainExecTask,
    // Name of the task
    "Main Task",
    // Stack size in words
    5000,
    // Task input parameter              
    NULL,
    // Priority of the task
    1,
    // Task handle
    NULL);

  xTaskCreate(
    buttonTask,
    "Button Task",
    5000,
    NULL,
    1,
    NULL);

  xTaskCreate(
    volumeTask,
    "Volume Task",
    5000,
    NULL,
    1,
    NULL);

  xTaskCreate(
    skipPrevTask,
    "SkipPrev Task",
    5000,
    NULL,
    1,
    NULL);
}
void loop() {
  
}
// Task to handle LCD
void mainExecTask(void * parameter) {
  while (true) {
    static String lastTrackname;
    String currentTrackname = sp.current_track_name();
    if (lastTrackname != currentTrackname && currentTrackname != "Something went wrong" && currentTrackname != "null") {
        lastTrackname = currentTrackname;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(lastTrackname);
        printDuration();
        Serial.println("Track: " + lastTrackname);
    }
    printPlayedTime();
    // Add a delay to avoid spamming the API
    //delay(100);  // Wait 1 seconds before sending another request
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay for 1000 ms (1 second)
  }
}

// Task to handle skip and previous buttons
void skipPrevTask(void * parameter) {
  while (true) {
  // read the state of the button value
  // buttonState = digitalRead(buttonPin);
  int x = 0;
  if (buttonPin1 == HIGH){
    x = 1;
    skipOrPrev(x);
  }else if (buttonPin2 == HIGH){
    x = 0;
    skipOrPrev(x);
  }
  // Delay for 200 ms to debounce
  vTaskDelay(450 / portTICK_PERIOD_MS);  
  }
}

// Task to monitor button press and print message
void buttonTask(void * parameter) {
  while (true) {
  // read the state of the button value
  // buttonState = digitalRead(buttonPin);
  currentButton = debounce(lastButton);
  if (lastButton == 0 && currentButton == 1){
    playOrPause();
  }
  // Delay for 200 ms to debounce
  vTaskDelay(200 / portTICK_PERIOD_MS);  
  }
}

// Task to monitor button press and print message
void volumeTask(void * parameter) {
  while (true) {
    // read the value of the potentiometer
    int potValue = analogRead(potPin); 
    // Read the voltage in millivolts
    // uint32_t voltage_mV = analogReadMilliVolts(potPin); 
    // Serial.print("Potentiometer Value: ");
    // Serial.print(potValue);
    // Serial.print(", Voltage: ");
    // Serial.println(voltage_mV / 1000.0); // Convert millivolts to volts
    // Map to PWM range
    double pwmValue = map(potValue, 0, 4095, 0, pow(2, resolution) - 1);  
    // Convert 0-4095 range to 0-100 range
    int volumePercent = (pwmValue/(pow(2, resolution) - 1.0))*100.0; 
    Serial.println(volumePercent);
  
    setSpotifyVolume(volumePercent);
    // Delay for 3000 ms (3 seconds)
    vTaskDelay(3000 / portTICK_PERIOD_MS);  
  }
}

int debounce (bool last){
  int current = digitalRead(buttonPin);
  if (last != current){
    delay(25);
    current = digitalRead(buttonPin);
  }
  return current;
}

bool requestAccessToken() {
  WiFiClientSecure client;
  // Disable SSL verification for prototyping purposes
  client.setInsecure();  

  const char* host = "accounts.spotify.com";
  int httpsPort = 443;

  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection to Spotify token endpoint failed");
    return false;
  }

  String authorization = base64::encode(String(CLIENT_ID) + ":" + CLIENT_SECRET);

  // Construct the POST request
  String requestBody = "grant_type=refresh_token&refresh_token=" + String(REFRESH_TOKEN);
  String request = String("POST /api/token HTTP/1.1\r\n") +
                   "Host: " + host + "\r\n" +
                   "Authorization: Basic " + authorization + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded\r\n" +
                   "Content-Length: " + String(requestBody.length()) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   requestBody;

  // Send the request
  client.print(request);

  // Wait for response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      // Headers ended, JSON response starts after
      break;  
    }
  }

  // Read the response body
  String response = client.readString();
  client.stop();

  // Parse the access token from the response
  int startIndex = response.indexOf("access_token\":\"") + 15;
  int endIndex = response.indexOf("\"", startIndex);
  if (startIndex > 0 && endIndex > startIndex) {
    accessToken = response.substring(startIndex, endIndex);
    return true;
  } else {
    Serial.println("Failed to parse access token");
    Serial.println("Response: " + response);
    return false;
  }
}

void setSpotifyVolume(int volumePercent) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    // Disable SSL verification for prototyping purposes
    client.setInsecure();  

    const char* host = "api.spotify.com";
    int httpsPort = 443;

    // Attempt to connect to Spotify's API
    if (!client.connect(host, httpsPort)) {
      Serial.println("Connection to Spotify API failed");
      return;
    }

    // Construct the request URL
    String url = "/v1/me/player/volume?volume_percent=" + String(volumePercent);

    // Construct the PUT request
    String request = String("PUT ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Authorization: Bearer " + accessToken + "\r\n" +
                     "Content-Length: 0\r\n" +
                     "Connection: close\r\n\r\n";

    // Send the request
    client.print(request);

    // Wait for response and print it
    int timeout = 1000;
    long start = millis();
    while (client.connected() && (millis() - start < timeout)) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
        if (line == "\r") {
          break; // Headers ended
        }
      }
      if (!client.connected()) {
        break;
      }
    }
    client.stop();
  } else {
    Serial.println("WiFi not connected");
  }
}

void skipOrPrev(int x) {
  if (WiFi.status() == WL_CONNECTED) {

    // Control Spotify playback based on the current state
    WiFiClientSecure client;
    // Disable SSL verification for prototyping purposes
    client.setInsecure();  

    const char* host = "api.spotify.com";
    int httpsPort = 443;
    String url = "/v1/me/player/";
    if (x == 1){
    // Skip to next
      url += "next";
    } else{
      // Skip to previous
      url += "previous";
    }

    // Attempt to connect to Spotify's API
    if (!client.connect(host, httpsPort)) {
      Serial.println("Connection to Spotify API failed");
      return;
    }

    // Construct the PUT request
    String request = String("POST ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Authorization: Bearer " + accessToken + "\r\n" +
                     "Content-Length: 0\r\n" +
                     "Connection: close\r\n\r\n";

    // Send the request
    client.print(request);

    // Wait for response and print it
    int timeout = 1000;
    long start = millis();
    while (client.connected() && (millis() - start < timeout)) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          // Headers ended
          break; 
        }
      }
      if (!client.connected()) {
        break;
      }
    }
    client.stop();
  } else {
    Serial.println("WiFi not connected");
  }
}

void playOrPause() {
  if (WiFi.status() == WL_CONNECTED) {
    // Check if the music is currently playing
    bool playing = isCurrentlyPlaying();

    // Determine the action (play or pause)
    String action = playing ? "pause" : "play";

    // Control Spotify playback based on the current state
    WiFiClientSecure client;
    // Disable SSL verification for prototyping purposes
    client.setInsecure();  

    const char* host = "api.spotify.com";
    int httpsPort = 443;

    // Determine the endpoint based on the action
    String url = "/v1/me/player/";
    if (action == "play") {
      url += "play";
    } else if (action == "pause") {
      url += "pause";
    }

    // Attempt to connect to Spotify's API
    if (!client.connect(host, httpsPort)) {
      Serial.println("Connection to Spotify API failed");
      return;
    }

    // Construct the PUT request
    String request = String("PUT ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Authorization: Bearer " + accessToken + "\r\n" +
                     "Content-Length: 0\r\n" +
                     "Connection: close\r\n\r\n";

    // Send the request
    client.print(request);

    // Wait for response and print it
    int timeout = 1000;
    long start = millis();
    while (client.connected() && (millis() - start < timeout)) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          // Headers ended
          break; 
        }
      }
      if (!client.connected()) {
        break;
      }
    }
    client.stop();
  } else {
    Serial.println("WiFi not connected");
  }
}

bool isCurrentlyPlaying() {
  WiFiClientSecure client;
  // Disable SSL verification for prototyping purposes
  client.setInsecure();  

  const char* host = "api.spotify.com";
  int httpsPort = 443;

  // Connect to Spotify API
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection to Spotify API failed");
    return false;
  }

  // Construct the GET request to check current playback state
  String request = String("GET /v1/me/player HTTP/1.1\r\n") +
                   "Host: " + host + "\r\n" +
                   "Authorization: Bearer " + accessToken + "\r\n" +
                   "Connection: close\r\n\r\n";

  // Send the request
  client.print(request);

  // Wait for response and parse the "is_playing" state
  bool playing = false;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      // Headers ended, JSON response starts after
      break;  
    }
  }

  // Read the response body
  String response = client.readString();
  client.stop();

  int startIndex = response.indexOf("is_playing") + 14;
  // Serial.println("mmmmm");
  // Serial.println(response.substring(startIndex, startIndex + 5));
  // Serial.println("mmmmm");
  if (startIndex > 0) {
    if (response.substring(startIndex, startIndex + 4) == "true") {
      playing = true;
    }
  }

  return playing;
}

void printPlayedTime(){
  lcd.setCursor(0, 1);

  response data = sp.currently_playing();

  int index = data.reply.indexOf("progress_ms");
  String num = data.reply.substring(index);
  index = num.indexOf(":");
  num = num.substring(index);
  int endIndex = num.indexOf(",");
  char arr[endIndex - 12];
  
  for (int i = 0; i < endIndex - 2; i++){
    arr[i] = num.charAt(i + 2);
  }

  printTime(arr);
}

void printDuration(){
  lcd.setCursor(5, 1);
  lcd.print("/");
  response data = sp.currently_playing();

  int index = data.reply.indexOf("duration_ms");
  String num = data.reply.substring(index);
  index = num.indexOf(":");
  num = num.substring(index);
  int endIndex = num.indexOf(",");
  char arr[endIndex - 12];
  
  for (int i = 0; i < endIndex - 2; i++){
    arr[i] = num.charAt(i + 2);
  }
  
  printTime(arr);
}

void printTime(char time[]){
  int totalSeconds = atoi(time) / 1000;
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  if (minutes < 10){
    lcd.print("0");
  }
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10){
    lcd.print("0");
  }
  lcd.print(seconds);
}

void connect_to_wifi(){
    WiFi.begin(SSID, PASSWORD);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println(".");
    }
    Serial.printf("\nConnected to WiFi\n");
}
/* Copyright (c) 2017 pcbreflux. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */

#include <PubSubClient.h>
#include <WiFi.h>

#define WIFI_AP "gemini"
#define WIFI_PASSWORD "tes12345"

#define TOKEN "5OOoSI79F7TXu7b3M6hi"

char thingsboardServer[] = "demo.thingsboard.io";

WiFiClient wifiClient;

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
unsigned long lastSend;

#define PLOTT_DATA 
#define MAX_BUFFER 100

uint32_t prevData[MAX_BUFFER];
uint32_t sumData=0;
uint32_t maxData=0;
uint32_t avgData=0;
uint32_t roundrobin=0;
uint32_t countData=0;
uint32_t period=0;
uint32_t lastperiod=0;
uint32_t millistimer=millis();
double frequency;
double beatspermin=0;
uint32_t newData;


/*
 * This is just a homebrew function.
 * Don't take this for critical measurements !!! 
 * Do your own research on frequencydetection for arbitrary waveforms.
 */
void freqDetec() {
  if (countData==MAX_BUFFER) {
   if (prevData[roundrobin] < avgData*1.5 && newData >= avgData*1.5){ // increasing and crossing last midpoint
    period = millis()-millistimer;//get period from current timer value
    millistimer = millis();//reset timer
    maxData = 0;
   }
  }
 roundrobin++;
 if (roundrobin >= MAX_BUFFER) {
    roundrobin=0;
 }
 if (countData<MAX_BUFFER) {
    countData++;
    sumData+=newData;
 } else {
    sumData+=newData-prevData[roundrobin];
 }
 avgData = sumData/countData;
 if (newData>maxData) {
  maxData = newData;
 }

 /* Ask your Ask your cardiologist
 * how to place the electrodes and read the data!
 */
#ifdef PLOTT_DATA
//  Serial.print(newData);
// Serial.print("\t");
// Serial.print(avgData);
// Serial.print("\t");
// Serial.print(avgData*1.5);
// Serial.print("\t");
// Serial.print(maxData);
// Serial.print("\t");
 Serial.print("Beats per minute: ");
 Serial.print(beatspermin);
 Serial.print(" bpm");
#endif
 prevData[roundrobin] = newData;//store previous value
 
 String bpm = String(beatspermin);
 
 // Just debug messages
 Serial.print( "Sending beats per min: [" );
 Serial.print( bpm );
 Serial.print( "]   -> " );

 // Prepare a JSON payload string
 String payload = "{";
 payload += "\"beatsperminute\":"; payload += bpm;
 payload += "}";

 // Send payload
 char attributes[100];
 payload.toCharArray( attributes, 100 );
 client.publish( "v1/devices/me/telemetry", attributes );
 Serial.println( attributes );
}

void setup() {
  Serial.begin(115200);
  InitWiFi();
  client.setServer( thingsboardServer, 1883 );
  lastSend = 0;
}

void loop() {
  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after 1 seconds
    newData = analogRead(34);
    freqDetec();
    if (period!=lastperiod) {
       frequency = 1000/(double)period;//timer rate/period
       if (frequency*60 > 20 && frequency*60 < 200) { // supress unrealistic Data
         beatspermin=frequency*60;
         #ifndef PLOTT_DATA
          Serial.print(frequency);
          Serial.print(" hz");
          Serial.print(" ");
          Serial.print(beatspermin);
          Serial.println(" bpm");
         #endif
         lastperiod=period;
       }
    }
    delay(5);
    lastSend = millis();
  }

  client.loop();
  
}

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
//    WiFi.mode(WIFI_STA);
    // Attempt to connect (clientId, username, password)
    if ( client.connect("ESP32 Demo", TOKEN, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

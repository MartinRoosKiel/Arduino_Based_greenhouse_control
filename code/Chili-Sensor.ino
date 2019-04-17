#include <OneWire.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information

// OneWire DS18S20, DS18B20, DS1822 Temperature Example
//
// http://www.pjrc.com/teensy/td_libs_OneWire.html
//
// The DallasTemperature library can do all this work for you!
// http://milesburton.com/Dallas_Temperature_Control_Library

bool debug = true;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 177);       // Set your IP address for Arduino Board

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
EthernetClient client;

typedef struct tempStruct{int count = 0;
float temp[3]={9999,9999,9999};String nameTemp; bool heating = false; int heatOut;float temperatur;};

tempStruct temp1; 
tempStruct temp2; 

OneWire  ds1(22);  // on pin 14 (a 4.7K resistor is necessary)
OneWire  ds2(23);  // on pin 15 (a 4.7K resistor is necessary)

float heating= 23;
float temp_hysterese = 3;

int heizung1 = 26;   // Digital 26 Heizung 1 anschließen
int heizung2 = 27;   // Digital 27 Heizung 2 anschließen

int ldr = 15;             //analog pin to which LDR is connected
int ldr_value = 0;        //variable to store LDR values
//int in = 0;              // Wert um nicht immer auslesen zu müssen
int light_low = 100;    // Min-Wert ( in Prozent )
int light_high = 0;     // MaxWert ( in Prozent )
int input = 0;
int light_hysterese = 5;
int light_an = 35; // Lichtschwellenwert
bool light; // Lichtzustand

int light_stunden_an = 5;
int light_stunden_aus = 20;

int light_minuten_an = 0;
int light_minuten_aus = 30;

int licht = 24; // Digital 24 Lampe anschließen


int light_tol = 5;      // Toleranz ( in Prozent )

unsigned int localPort = 8888;       // local port to listen for UDP packets

char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

int hours = 0; 
int minutes = 0;
int seconds = 0;

bool mailSend = false;

void setup(void) {
  Serial.begin(9600);

temp1.nameTemp = "Gr&uuml;n";
temp2.nameTemp = "Blau";
temp1.heatOut =  heizung1;
temp2.heatOut =  heizung2;
light =false;

pinMode(temp1.heatOut, OUTPUT);
digitalWrite(temp1.heatOut, HIGH); 
pinMode(temp2.heatOut, OUTPUT);
digitalWrite(temp2.heatOut, HIGH); 
pinMode(licht, OUTPUT);
digitalWrite(licht, HIGH); 
pinMode(25, OUTPUT);
digitalWrite(25, HIGH); 

    // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  server.begin();


  Udp.begin(localPort);
  
}




void loop(void) {

 
timePoll();
  runTemp(&ds1,&temp1);
  delay(500);
  runTemp(&ds2,&temp2);
  delay(500);
lightRead();
  
  ether();


}


void sendEMail(String Message){
if(debug)
{
Serial.print("eMail wird verschickt mit Text: ");
Serial.println(Message);
}
    TembooChoreo SendEmailChoreo(client);

    // Invoke the Temboo client
    SendEmailChoreo.begin();

    // Set Temboo account credentials
    SendEmailChoreo.setAccountName(TEMBOO_ACCOUNT);
    SendEmailChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    SendEmailChoreo.setAppKey(TEMBOO_APP_KEY);

    // Set Choreo inputs
    SendEmailChoreo.addInput("FromAddress", FromEMailAddressValue);
    SendEmailChoreo.addInput("Username", FROMEMailAddress);
    String SubjectValue = "Hier spricht Ihr Gewächshaus";
    SendEmailChoreo.addInput("Subject", SubjectValue);
    SendEmailChoreo.addInput("ToAddress", ToEMailAddressValue);
    String MessageBodyValue = "Dies ist eine Meldung aus Ihrem Gewächshaus \n \n " + Message;
    SendEmailChoreo.addInput("MessageBody", MessageBodyValue);
    SendEmailChoreo.addInput("Password", PasswordValue);

    // Identify the Choreo to run
    SendEmailChoreo.setChoreo("/Library/Google/Gmail/SendEmail");

    // Run the Choreo; when results are available, print them to serial
    SendEmailChoreo.run();

    while(SendEmailChoreo.available()) {
      char c = SendEmailChoreo.read();
      Serial.print(c);
    }
    SendEmailChoreo.close();
  }


void timePoll() {
  sendNTPpacket(timeServer); // send an NTP packet to a time server

  // wait to see if a reply is available
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;

     hours = ((epoch  % 86400L) / 3600); // calculate the hour (86400 equals secs per day)
    minutes = (epoch % 3600) / 60; // calculate the minutes
   
    seconds = epoch % 60;

if(debug){
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
     Serial.print(":");
    Serial.println(seconds);  }
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void lightRead( void )
{
  input = 100-analogRead(ldr)/10.23;
  
  if(abs(input-ldr_value) > light_tol)
  {
  ldr_value = input;          //reads the LDR values
  
  if(ldr_value > light_high){ light_high = ldr_value;}
  if(ldr_value < light_low){ light_low = ldr_value;}
  }

if( (light_stunden_an < hours & light_stunden_aus > hours )| (light_stunden_an == hours & light_minuten_an < minutes)| (light_stunden_aus == hours & light_minuten_aus > minutes))
{  if(ldr_value<light_an){light = true;}
  if(ldr_value>light_an+light_hysterese){light = false;}
}
else
{
  light = false;
  }

  if(light){
digitalWrite(licht, LOW); }
else
{
digitalWrite(licht, HIGH); }
}

void ether(void)
{
     // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {

    
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 30");  // refresh the page automatically every 30 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("<p style='text-align: center;'>&nbsp;</p>");
          client.print("<p style='text-align: center;'><span style='font-size: x-large;'><strong>Chili Zuchstation</strong></span></p>");
          client.print("<p style='text-align: center;'>Lichtwert aktuell: ");
                   client.print(ldr_value);
          client.print("%</p>");
         
           client.print("<p style='text-align: center;'>Licht ist: ");

           if(light)
          {
          client.println("an");
          }
          else
          {
          client.println("aus");
          }
           client.print("</p>");
          client.print("<table style=\"margin-left:auto;margin-right:auto\"><tr><td>Feld: </td><td>");
          client.print(temp1.nameTemp);
          client.print("</td><td></td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>Feld: </td><td>");
          client.println(temp2.nameTemp);
          client.print("</td><td></td></tr><tr><td>Temperatur: ");
          client.print("</td><td>");
          client.println(temp1.temperatur);
          client.print("</td><td><sup>o</sup>C</td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>Temperatur: ");
          client.print("</td><td>");
          client.println(temp2.temperatur);
          client.print("</td><td><sup>o</sup>C<br></td></tr><tr><td>Heizung: ");
          client.print("</td><td>");
          if(temp1.heating)
          {
          client.println("an");
          }
          else
          {
          client.println("aus");
          }
          client.print("</td><td></td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>Heizung: ");
          client.print("</td><td>");
          if(temp2.heating)
          {
          client.println("an");
          }
          else
          {
          client.println("aus");
          }
          client.print("</td><td></td></tr></table><p style='text-align: center;'>&nbsp;</p>");
          client.print("<p style='text-align: center;'>&nbsp;</p>");
          client.print("<p style='text-align: center;'>&nbsp;");
          client.print("<table><tr><td><table>");
          client.print("<tr><td>Licht Setpoint:</td><td>");
          client.print(light_an);
          client.print("%</td></tr>");
                    client.print("<tr><td>Licht Toleranz:</td><td>");
          client.print(light_tol);
          client.print("%</td></tr>");
          client.print("<tr><td>Licht Hyterese:</td><td>");
          client.println(light_hysterese);
          client.print("%</td></tr>");
          client.print("<tr><td>Licht max:</td><td>");
          client.println(light_high);
          client.print("%</td></tr>");
          client.print("<tr><td>Licht min:</td><td>");
          client.println(light_low);
          client.print("%</td></tr>"); 
                    client.print("<tr><td>Licht an:</td><td>");
          client.println(light_stunden_an);
          client.print(":");
          if(light_minuten_an < 10)
          {
            client.print("0");
            }
          client.println(light_minuten_an);
          client.print("</td></tr>");
                    client.print("<tr><td>Licht aus:</td><td>");
          client.println(light_stunden_aus);
          client.print(":");
          if(light_minuten_aus < 10)
          {
            client.print("0");
            }
          client.println(light_minuten_aus);
          client.print("</td></tr>");
          client.print("</table></td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>");
                    client.print("<table>");
          client.print("<tr><td>Temperatur Setpoint:</td><td>");
          client.println(heating);
          client.print("</td></tr>");
          client.print("<tr><td>Temperatur Hysterese:</td><td>");
          client.println(temp_hysterese);
          client.print("</td></tr>");
          client.print("</table></td></tr></table>");
          client.print("<p style='text-align: center;'><span style='font-size: small;'><strong>Uhrzeit ( UTC ): ");
          client.println(hours);
          client.print(":");
          if(minutes < 10)
          {client.print("0");}
          client.println(minutes);
           client.print(":");
          if(seconds < 10)
          {client.print("0");}
          client.println(seconds);
          
          
          client.print("</strong></span></p>");
 
          client.print("</p>");
          client.print("<p style='text-align: center;'><span style='font-size: small;'><strong>Version: 0.8</strong></span></p>");
 
    
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
  }

void update_temp(struct tempStruct *tS)
{
float temperatur = 9999.00;
  
sort_temp(tS);

if(tS->temp[2] -tS->temp[0] > 5)
{  // logic vote
if(debug)
{
Serial.print("logic vote ");
}
if(tS->temp[1] -tS->temp[0] < 5)
{temperatur = (tS->temp[0] + tS->temp[1])/2;
}
if(tS->temp[2] -tS->temp[1] < 5)
{
  if(((tS->temp[1] + tS->temp[2])/2) < ((tS->temp[0] + tS->temp[1])/2))
  {temperatur = (tS->temp[1] + tS->temp[2])/2;}
}
}
  
  
  else // kleiner als 
  {
    
      temperatur = ( tS->temp[0] +tS->temp[1] +tS->temp[2]) /3 ;
      }

 if(temperatur < 9999.00)
 {
 tS->temperatur = temperatur;
  if(temperatur > heating + light_hysterese)
  { 
  //  digitalWrite(tS->heating, LOW); 
    tS->heating = false;
    
digitalWrite(tS->heatOut, HIGH); 
    }

    else // kleiner als heating
    {
     // digitalWrite(tS->heating, HIGH);
      tS->heating = true;

      digitalWrite(tS->heatOut, LOW); 
      }

      if(temperatur > 32 & !mailSend)
      {
        //sendEMail("Die Chilis haben Fieber!");
        Serial.println("Hier sollte eine eMail gesendet werden!");
        }

        if(temperatur < 32 & mailSend)
        {
          mailSend = false;
          Serial.println("wieder scharf!");
          }
if(debug){
  Serial.print("Name des Kompartments: "); 
  Serial.println(tS->nameTemp);
   Serial.print("Temperatur:  ");
   Serial.print(temperatur);
   Serial.println(" Grad Celsius");
   Serial.print("Heizung ist: ");
   Serial.println(tS->heating);
}
 }
 
    tS->temp[0] = 9999;
   tS->temp[1] = 9999;
   tS->temp[2] = 9999;
   tS->count = 0;
   
}

void runTemp(OneWire *oW, struct tempStruct *tS  )
{
   byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  
  if ( !oW->search(addr)) {
  


 update_temp(tS);
    return;
  }
  
 

  oW->reset();
  oW->select(addr);
  oW->write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = oW->reset();
  oW->select(addr);    
  oW->write(0xBE);         // Read Scratchpad


  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = oW->read();
    
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  tS->temp[tS->count] = celsius;
  tS->count++;
  }


void switch_temp(struct tempStruct *tS ,int a, int b)
{
  float temp = tS->temp[b];
  tS->temp[b] = tS->temp[a];
  tS->temp[a] = temp;
  }

void sort_temp(struct tempStruct *tS)
{
  if(tS->temp[0] > tS->temp[1])
  {
    switch_temp(tS,0,1);
    }
if(tS->temp[1] > tS->temp[2])
{
  switch_temp(tS,1,2);
  if(tS->temp[0] > tS->temp[1])
  {
    switch_temp(tS,0,1);
    }}
    
  }

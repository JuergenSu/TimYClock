// Whatch that displays text and many other things like days until next birthdays
//

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted. Uses GPIO Pin umbering
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8
#define CLK_PIN   14
#define DATA_PIN  13
#define CS_PIN    12

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

RTC_DS3231 rtc;

// the buffer for the time to display
char szTime[9]; 
// The scrolling message Buffer
char szMessage[200]; 

// The MOde of operation
int mode = 4;

// the text to display. this is read from eeprom
String text = "Bitte anmelden um den Text zu ändern";
int text_len = 33;

//instantiate server at port 80 (http port)
ESP8266WebServer server(80); 

//calculate days until from now to a spacified timestamp (%365) 
int daysUntil(long timestamp, DateTime now) {
 return ((31557600L - ((now.unixtime() - timestamp - 86400L)% 31557600L))  / 86400L);
}

void setup(void)
{
  Serial.begin(115200);

  // Read stored values from EEPROM
  EEPROM.begin(256);
  mode=EEPROM.read(0);
  Serial.println("mode");
  Serial.println(mode);
  text_len = EEPROM.read(1);
  char buf[text_len];
  for(int i=0; i< text_len; i++){
      buf[i] = EEPROM.read(i+2);
  }
  text = String(buf);

  Serial.println("text");
  Serial.println(text);
  EEPROM.end();


  // Initialize RTC
  Serial.println("Starting rtc");
  if(! rtc.begin() ) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

   if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F(__DATE__));
    Serial.println(F(__TIME__));

    DateTime now = rtc.now();
    char buf1[20];
    sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    Serial.print(F("Date/Time: "));
    Serial.println(buf1);
  }

  // Init SOFTAP
  WiFi.softAP("TimYClock", "TimYClock");

  // Define server for controlling the whatch
  server.on("/", [](){

    // Main Controll page
    Serial.println("-> INDEX");

    String resp = String("<head><title>Timy</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"/></head>") +
                  String("<body width=\"200px\"> <h1>TimY Einstellungen</h1> <b>Modus?</b> <form action=\"save\" method=\"get\">  ") + 
                  String("<select class=\"form-control\" name=\"m\" style=\"width:100%\"> ") +
                  String("<option value=\"1\" ") + 
                  String((mode ==1) ? "selected":"" ) + 
                  String(">Uhrzeit</option>") +
                  String("<option value=\"2\" " ) + 
                  String( (mode ==2) ? "selected":"" ) + 
                  String( ">Text</option>  ") +
                  String("<option value=\"3\" " ) + 
                  String( (mode ==3) ? "selected":"" ) + 
                  String( ">Uhrzeit + Text</option>  ") +
                  String("<option value=\"4\" " ) + 
                  String( (mode ==4) ? "selected":"" ) + 
                  String( ">Uhrzeit + Geburtstage</option> ") +
                  String("</select>  <br>  <b>Text ?</b><br>  <input type=\"text\" name=\"t\" maxlength=\"150\" value=\"" ) + 
                  String( text ) + 
                  String( "\" style=\"width:100%\"/>") +
                  String("<br><br>  <input type=\"submit\" value=\"Save\" style=\"width:100%\"/> </form><hr/>")+
                  String("<form action=\"saveTime\" method=\"get\">") + 
                  String("<input type=\"text\" name=\"date\" maxlength=\"150\" value=\"" ) + 
                  String( F(__DATE__) ) + 
                  String( "\" style=\"width:100%\"/><br/>") +
                  String("<input type=\"text\" name=\"time\" maxlength=\"150\" value=\"" ) + 
                  String( F(__TIME__) ) + 
                  String( "\" style=\"width:100%\"/><br/>") +
                  String("<br><br>  <input type=\"submit\" value=\"Save\" style=\"width:100%\"/>");
                  String("</form></body>");

                                  
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", resp);
            
  });

  server.on("/saveTime", [](){
    
    // Save Values to eeprom and return to the index page
    Serial.println("-> setTime");
    
    String fdate = server.arg("date");
    String ftime = server.arg("time");   
    rtc.adjust(DateTime(fdate.c_str(), ftime.c_str()));

    Serial.println("TimeSet");
    
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");

        
  }); 

  server.on("/save", [](){

    // Save Values to eeprom and return to the index page
    Serial.println("-> save");

    // needed to get a eeprom copy in memory to work with
    EEPROM.begin(256);
    server.sendHeader("Connection", "close");

    //store mode
    mode = server.arg("m").toInt();
    Serial.println(mode);
    EEPROM.write(0, mode);

    //store message length
    text = server.arg("t");   
    text_len = text.length() + 1; 
    EEPROM.write(1, text_len);
    Serial.println(text_len);
    //store Text
    for(int i=0; i< text.length(); i++){
      EEPROM.write(i+2, text[i]);
    }
    EEPROM.write(text.length()+2, 0);
    Serial.println(text);
    
    EEPROM.commit();
    EEPROM.end();

    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
        
  });





  Serial.println("Starting parola");
  P.begin();
  Serial.println("Setting intensity");
  P.setIntensity(2);
  Serial.println("server.begin();");
  server.begin();
  Serial.println("Setup Done");
}

void loop(void)
{
  server.handleClient();
  P.displayAnimate();


  
  if (P.getZoneStatus(0)) {
    //Serial.println("zone complete");
    DateTime now = rtc.now();
    char buf1[20];
    sprintf(buf1, "%02d:%02d:%02d %02d/%02d/%02d",  now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    //Serial.print(F("Date/Time: "));
    //Serial.println(buf1);
    if (mode == 1) {
      // Just show time
      Serial.println("using clock");
      if (summertime_EU(now.year(), now.month(), now.day(), now.hour(), 1)) { 
        Serial.println("using SummerTime");  
        sprintf(szTime, "%02d:%02d:%02d", now.hour()+1, now.minute(), now.second());
      } else {
         Serial.println("using WinterTime");
        sprintf(szTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      }
      P.displayText( szTime, PA_CENTER, 100, 1, PA_PRINT, PA_NO_EFFECT);
    }

    if (mode == 2) {
        // Just show text
        Serial.println("using text");
        // ensure the text does fit in the buffer and the last byte is 0
        if (text_len >= 200) {
          text.toCharArray(szMessage, 199);
          szMessage[199] = 0;
        } else {
          text.toCharArray(szMessage, text_len);
          szMessage[text_len] = 0;
        }
        P.displayText( szMessage , PA_CENTER, 75, 1, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }

    if (mode == 3) {
      Serial.println("using clock and text");
      if (now.second() ==0 && now.minute() % 2 == 0) {
        // ensure the text does fit in the buffer and the last byte is 0
        if (text_len >= 200) {
          text.toCharArray(szMessage, 199);
          szMessage[199] = 0;
        } else {
          text.toCharArray(szMessage, text_len);
          szMessage[text_len] = 0;
        }
        P.displayText( szMessage , PA_CENTER, 75, 1, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
      } else {
        if (summertime_EU(now.year(), now.month(), now.day(), now.hour(), 1)) { 
          Serial.println("using SummerTime");  
          sprintf(szTime, "%02d:%02d:%02d", now.hour()+1, now.minute(), now.second());
        } else {
           Serial.println("using WinterTime");
          sprintf(szTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        }
        P.displayText( szTime, PA_CENTER, 100, 1, PA_PRINT, PA_NO_EFFECT);
        
      }      
    }


    if (mode == 4) {
      if (now.second() ==0 && now.minute() % 2 == 0) {
        Serial.println("using birthday");

        // Use birth date asa unix timestamp 
        
        int gjuli = daysUntil(504576000L, now);
        int gsteffi = daysUntil(548294400L, now);
        int gole = daysUntil(1289001600L, now);
        int gjustus = daysUntil(1338595200L, now);
        int gLili = daysUntil(1431648000L, now);
        int gmila = daysUntil(1515024000L, now);
                            
        sprintf( szMessage, "Tage bis zum Geburtstag --- Papa: %3d Mama: %3d Ole-Anton: %3d Justus: %3d Lilien: %3d Mila: %3d",
                            gjuli,
                            gsteffi,
                            gole,
                            gjustus,
                            gLili,
                            gmila
                            );
                            
                          
         
        P.displayText( szMessage , PA_CENTER, 75, 1, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        
      } else {
        //Serial.println("using clock");
        if (summertime_EU(now.year(), now.month(), now.day(), now.hour(), 1)) { 
          //Serial.println("using SummerTime");  
          sprintf(szTime, "%02d:%02d:%02d", now.hour()+1, now.minute(), now.second());
        } else {
           //Serial.println("using WinterTime");
          sprintf(szTime, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        }
        P.displayText( szTime, PA_CENTER, 100, 1, PA_PRINT, PA_NO_EFFECT);
        
      }
    }
    P.displayReset();
  }
   
}


boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
// thnaks to Serenifly (https://forum.arduino.cc/index.php?topic=228063.0)
{
 if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
 if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
 if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)))
   return true;
 else
   return false;
}

#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include <Wire.h>
//#include <U8x8lib.h>

//U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED
// assign a MAC address for the ethernet controller.
// fill in your address here:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 178);
IPAddress myDns(192, 168, 1, 1);

// initialize the library instance:
EthernetClient client;
IPAddress server(192, 168, 1, 4);
//char server[] = "sms.local";
// initialize the SMS module
SoftwareSerial mySerial(7, 8);

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds

String paramMessage;
String message;
String number;
String textStatus;
String textRequestId;
boolean startReadingParamValues = false;
boolean getMessageContent = false;
boolean getNumber = false;
boolean getTextStatus = false;
boolean isGoingToText = false;
boolean getTextRequestId = false;
boolean errorSMS = false;
//void updateSerial(boolean readSignal = false);

void setup() {
  Serial.begin(9600);
  SIM900power();
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  mySerial.begin(9600);
  pinMode(A4, OUTPUT); // server connected
  pinMode(A3, OUTPUT); // high signal 11-31
  pinMode(A1, OUTPUT); // low signal 0-10
  //  Pins for LED

  //    u8x8.begin();
  //    u8x8.setPowerSave(0);
  //    u8x8.setFont(u8x8_font_chroma48medium8_r);
  //    u8x8.drawString(0, 0, "System Starting");
  //    delay(1000);
  // initialize SMS module

  Serial.println("SMS Initializing");
  mySerial.println("AT"); //Handshaking with SIM900
  updateSerial();
  mySerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
  mySerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  updateSerial();
  mySerial.println("AT+CREG?"); //Check whether it has registered in the network
  updateSerial();
  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  //  u8x8.drawString(0, 1, "Ethernet Starting");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. 🙁");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    Ethernet.begin(mac, ip, myDns);
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
  } else {
    Serial.print("DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
}

//void displayScreen() {
//  u8x8.setFont(u8x8_font_chroma48medium8_r);
//  u8x8.drawString(0, 0, "Hello world!");
//  u8x8.refreshDisplay();    // only required for SSD1606/7
//  delay(2000);
//}
void updateSerial()
{
  delay(500);
  String signalQuality;
  while (Serial.available())
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (mySerial.available())
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port''
  }
}

void SIM900power()
{
  Serial.println("SMS Powering Up");
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
  delay(1000);
  digitalWrite(9, HIGH);
  delay(2000);
  digitalWrite(9, LOW);
  delay(3000);
}

void loop() {
  // if there's incoming data DHfrom the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  //  displayScreen();
  if (client.connected() || client.available()) {
    char c = client.read();
    if (c == '{') {
      startReadingParamValues = true;
    }
    if (c == '}') {
      startReadingParamValues = false;
      paramMessage = "";
      if (isGoingToText) {
        sendSMS();
        updateHttpRequest();
        message = "";
        number = "";
        textStatus = "";
        textRequestId = "";
        getTextStatus = false;
        isGoingToText = false;
        //        errorSMS = false;
      }
    }
    if (startReadingParamValues) {
      if (c != '}') {
        paramMessage += c;
        if (c == '{' || c == ':' || c == '"' || c == ',') {
          paramMessage = "";
          return;
        }

        if (c == '%') {
          getMessageContent = false;
          getNumber = false;
          getTextStatus = false;
          getTextRequestId = false;
          paramMessage = "";
          return;
        }

        if (paramMessage == "message") {
          getMessageContent = true;
          paramMessage = "";
          return;
        }

        if (paramMessage == "id") {
          getTextRequestId = true;
          paramMessage = "";
          return;
        }

        if (paramMessage == "number") {
          getMessageContent = false;
          getNumber = true;
          paramMessage = "";
          return;
        }
        if (paramMessage == "status") {
          getNumber = false;
          getTextStatus = true;
          paramMessage = "";
          isGoingToText = true;
          return;
        }

        if (getMessageContent && c != ',' && c != '"' && c != ':') {
          message += c;
        }

        if (getNumber && c != ',' && c != '"' && c != ':') {
          number += c;
        }

        if (getTextStatus && c != ',' && c != '"' && c != ':') {
          textStatus += c;
        }

        if (getTextRequestId && c != ',' && c != '"' && c != ':') {
          textRequestId += c;
        }
      }
    }
  }

  if (millis() - lastConnectionTime > (30 * 1000)) {
    httpRequest();
    signalCheck();
  }

}

void sendSMS() {
  Serial.println("Contents of server data");
  Serial.print("Message : ");
  Serial.println(message);
  Serial.print("Number : ");
  Serial.println(number);
  Serial.print("Text Status : ");
  Serial.println(textStatus);
  Serial.print("Text Status ID : ");
  Serial.println(textRequestId);
  Serial.print("\n");
  paramMessage = "";
  String sendSMSParams = "AT+CMGS=\"" + number + "\"";
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  //  mySerial.println("AT+CMGS=\"+ZZxxxxxxxxxx\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  mySerial.println(sendSMSParams);
  updateSerial();
  mySerial.print(message); //text content
  updateSerial();
  mySerial.write(26);
  //  if (!WaitForResponse()) {
  //    errorSMS = true;
  //    Serial.println("ERROR or timeout waiting for response to CMGS command");
  //  }
}

//bool WaitForResponse()
//{
//  unsigned long startTime = millis();
//  while (millis() - startTime < 5000)
//  {
//    String reply = mySerial.readStringUntil('\n');
//    if (reply.length() > 0)
//    {
//      Serial.print("Received: \"");
//      Serial.print(reply);
//      Serial.println("\"");
//
//      if (reply.startsWith("OK"))
//        return true;
//
//      if (reply.startsWith("ERROR"))
//        return false;
//    }
//  }
//  return false;
//}
void signalCheck() {
  mySerial.println("AT+CSQ");
  String signalQuality = mySerial.readString();
  int str_len = signalQuality.length() + 1;
  char myString[str_len];
  signalQuality.toCharArray(myString, str_len);
  String signalNo = String(myString[str_len - 13]) + String(myString[str_len - 12]);
  Serial.println(signalQuality);
  Serial.print("Signal quality : ");

  Serial.println(signalNo);
  if (signalNo) {
    Serial.println("signal detected");
    if (signalNo.toInt() > 0) {
      // naay signal
      errorSMS  = false;
      digitalWrite(A1, HIGH);
      digitalWrite(A3, LOW);
    }
    else {
      // wlay signal
      errorSMS  = true;
      digitalWrite(A3, HIGH);
      digitalWrite(A1, LOW);
    }
    updateSerial();
  } else {
    Serial.println("no signal detected");
    digitalWrite(A1, LOW);
    digitalWrite(A3, LOW);
  }

}
void httpRequest() {
  client.stop();
  if (client.connect(server, 80) == 1) {
    digitalWrite(A4, HIGH);
    //    client.print("GET /sms_project/get_message.php HTTP/1.1");
    //    Serial.println("GET /sms_project /get_message.php HTTP/1.1");
    client.print("GET /ecarding10282021/api/send HTTP/1.1");
    Serial.println("GET /ecarding10282021/api/send HTTP/1.1");
    client.println();
    client.println("Host: localhost");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
    client.println();
    lastConnectionTime = millis();
    Serial.println("Connected to getting message");
  } else {
    Serial.println("connection failed");
    digitalWrite(A4, LOW);
  }
}

void updateHttpRequest() {
  client.stop();
  if (client.connect(server, 80) == 1) {
    Serial.println("connecting...");
    //    Serial.println("POST /sms_project/update_message.php?status=");
    //    client.print("POST /sms_project/update_message.php?status=");
    Serial.println("POST /ecarding10282021/api/send?status=");
    client.print("POST /ecarding10282021/api/send?status=");
    Serial.print(isGoingToText);
    client.print(isGoingToText);
    Serial.print("&id=");
    client.print("&id=");
    Serial.print(textRequestId);
    client.print(textRequestId);
    client.print("&status=");
    Serial.print("&status=");
    client.print(true);
    Serial.print(true);
    if (errorSMS) {
      client.print("&error_status=");
      client.print(true);
      client.print("&error_sms_remarks=ERROR or timeout waiting for response to CMGS command (NO signal)");
    }
    client.print(" HTTP/1.1");
    client.println();
    client.println("Host: localhost");
    client.println("User-Agent: arduino-ethernet");
    client.println("Connection: close");
    client.println();
    client.println();
    Serial.println("\nConnected: sending update status");
    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

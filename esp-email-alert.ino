#include <Arduino.h>
#include <ESP_Mail_Client.h>
#include <TinyGPS++.h>
TinyGPSPlus gps;

#if defined(ESP32)
  #include <WiFi.h>
  #define RXD2 16
  #define TXD2 17
  HardwareSerial neo6m(2);

  #define pir_sensor 5
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <SoftwareSerial.h>
  const int rxPin = D4, txPin = D3;
  SoftwareSerial neo6m(rxPin, txPin);

  #define pir_sensor D5
#endif
#define AUTHOR_EMAIL "ENTER_EMAIL_FOR_SMTP_SERVER"
#define AUTHOR_PASSWORD "ENTER_PASSWORD_FOR_SMTP_SERVER"
#define RECIPIENT_EMAIL "ENTER_RECEIVER_EMAIL"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 //SMTP port (SSL)

#define WIFI_SSID "ENTER_WIFI_SSID"
#define WIFI_PASSWORD "ENTER_WIFI_PASSWORD"
SMTPSession smtp;

void smtpCallback(SMTP_Status status);
void setup(){
  Serial.begin(115200);
  pinMode(pir_sensor, INPUT);
  #if defined(ESP32)
    neo6m.begin(9600, SERIAL_8N1, RXD2, TXD2);
  #elif defined(ESP8266)
    neo6m.begin(9600);
  #endif
  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
void loop(){
  int val = digitalRead(pir_sensor);

  if (val == HIGH) {
    Serial.println("Motion detected!");
    send_email_alert();
  }

  delay(1000);
}

void send_email_alert(){

  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neo6m.available()){
      if (gps.encode(neo6m.read()))
      {newData = true;break;}}
  }

  if(newData != true or gps.location.isValid() != 1)
  {Serial.println("No Valid GPS Data is Found.");return;}
 
  newData = false;
  String latitude = String(gps.location.lat(), 6);
  String longitude = String(gps.location.lng(), 6);
  Serial.println("Latitude: "+ latitude);
  Serial.println("Longitude: "+ longitude);
  return;

  smtp.debug(1);

  smtp.callback(smtpCallback);

  ESP_Mail_Session session;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
 
  SMTP_Message message;

  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Motion Alert";
  message.addRecipient("AhmadLogs", RECIPIENT_EMAIL);
   String htmlMsg = "<div style=\"color:#2f4468;\">";
  htmlMsg += "<h1>Latitude: "+latitude+"</h1>";
  htmlMsg += "<h1>Longitude: "+longitude+"!</h1>";
  htmlMsg += "<h1><a href=\"http://maps.google.com/maps?q=loc:"+latitude+","+longitude+"\">Check Location in Google Maps</a></h1>";
  htmlMsg += "<p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session))
    return;

  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
  //-------------------------------------------------
}

void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}

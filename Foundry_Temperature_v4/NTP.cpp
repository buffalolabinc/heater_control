#include "Common.h"
#include <WiFiUdp.h>

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR      true
#define NTP_SYNC_INTERVAL 86400  /*clock drift on M0 is apparently bad.  re-sync every hour */

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
//const char* ntpServerName = "time.nist.gov";
const char* ntpServerName = "us.pool.ntp.org";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

unsigned long sendNTPpacket(IPAddress& address);

void InitNTP()
{
  Serial.println("Starting NTP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

 //Set up NTP
  setSyncInterval(NTP_SYNC_INTERVAL);   //sync to NTP this often
  setSyncProvider(getNTPTime);

  Serial.println("Waiting for NTP sync");
  //wait for initial time sync to succeed.  getNTPTime will reset the sync interval to 15 seconds if NTP fails
  while (timeSet != timeStatus())
  {
    Serial.println("Waiting for NTP sync");
    Serial.print("timeStatus() = "); Serial.println(timeStatus());
    time_t timenow = now(); //allow time sync to occur by calling now()
    delay(1000);
  }
  Serial.println("NTP time is set");    //sync interval automatically reset to NTP_SYNC_INTERVAL
}

time_t getNTPTime()
{
  time_t epoch = 0;
  int retries = 0;

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  int replyLen;
  do {
    delay(1000);
    replyLen = udp.parsePacket();
  } while ((retries++ < 15) && (!replyLen));

  if (!replyLen) {
    Serial.println("no packet yet");
    setSyncInterval(30);   //try again in a little while
  }
  else {
    Serial.print("packet received, length="); Serial.println(replyLen);
    setSyncInterval(NTP_SYNC_INTERVAL);  //update again in 24 hours
    
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    CorrectDSTsettings(epoch);

    epoch += cur_UTC_offset;
    
  }
  return epoch;
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

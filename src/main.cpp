#include <Arduino.h>

/*  Code based on ARTNET RECEIVER v3.1, modified to run Neopixels. By @Shazman.visuals.

  Current offering/limitations:
  - 1 universe per pin (max 170 pixels)
  - Must start on first Channel of that universe
  - Universes must be sequentially increased after the first one
  - Only 3 channels per pixel (RGB)

*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const int uni1 = 0;
const int pin1 = 27;
const int pix1 = 170;

const int uni2 = 1;
const int pin2 = 26;
const int pix2 = pix1;

const int uni3 = 2;
const int pin3 = 25;
const int pix3 = pix1;

const int uni4 = 3;
const int pin4 = 33;
const int pix4 = pix1;

const int uni5 = 4;
const int pin5 = 32;
const int pix5 = pix1;

const int uni6 = 5;
const int pin6 = 2;
const int pix6 = pix1;

const int uni7 = 6;
const int pin7 = 4;
const int pix7 = pix1;

const int uni8 = 7;
const int pin8 = 21;
const int pix8 = pix1;

const int totalUniverses = 8; // this is temporarily only here for controlling amount of Universes to read in the buffer for-loop, and replaces "amountUniverses" there
const int SubnetID = 0;       // There are 16 Universes in a Subnet, so after 16 Universes, increase Subnet with 1 and start Universe from 0 again
const int channelsPerLed = 3;
const int startUniverse = ((SubnetID * 16) + uni1); // defines what universe to start reading from

// -----THIS IS FOR SIMPLE PIXEL CONFIGS (see "Current limitations" in the top)
int startChannel = 0;
int endChannel[8] = {pix1 * channelsPerLed, pix2 *channelsPerLed, pix3 *channelsPerLed, pix4 *channelsPerLed, pix5 *channelsPerLed, pix6 *channelsPerLed, pix7 *channelsPerLed, pix8 *channelsPerLed};

// -----THIS IS FOR COMPLEX PIXEL CONFIGS (>170pix/pin, not used at the moment)
/*
  const int pixelsPerOutput = 60;  // <<<<<needs to change to auto pick number
  const int totalChannels = pixelsPerOutput * channelsPerLed;
  const int amountUniverses = totalChannels / 510 + ((totalChannels % 510) ? 1 : 0);    // rounds up amount of needed universes for the pixel amount
  int channelsLeft = totalChannels;
  int start_address[amountUniverses];
  int channelsToRead[amountUniverses];
  int startPixel[amountUniverses];        // this is for when wanting more than 1 universe per pin
  const int start_address = 0;      // 0 if you want to read from channel 1
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <FastLED.h>
CRGB leds1[pix1];
CRGB leds2[pix2];
CRGB leds3[pix3];
CRGB leds4[pix4];
CRGB leds5[pix5];
CRGB leds6[pix6];
CRGB leds7[pix7];
CRGB leds8[pix8];
CRGB *stripNr[] = {leds1, leds2, leds3, leds4, leds5, leds6, leds7, leds8};

#include <SPI.h>
#include <Ethernet.h> // When using Arduino/ESP boards
//#include <Ethernet_STM.h>      // When using STM32 boards (https://github.com/Serasidis/Ethernet_STM)
#include <EthernetUdp.h> // UDP library from: bjoern@cs.stanford.edu 12/30/2008
#define short_get_high_byte(x) ((HIGH_BYTE & x) >> 8)
#define short_get_low_byte(x) (LOW_BYTE & x)
#define bytes_to_short(h, l) (((h << 8) & 0xff00) | (l & 0x00FF));

// hello world

byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0x4C, 0x8C}; //the mac adress in HEX of ethernet shield or uno shield board
byte ip[] = {192, 168, 0, 50};                     // the IP adress of your device, that should be in same universe of the network you are using

// the next two variables are set when a packet is received
byte remoteIp[4];        // holds received packet's originating IP
unsigned int remotePort; // holds received packet's originating port

//buffers
const int MAX_BUFFER_UDP = 768;
char packetBuffer[MAX_BUFFER_UDP]; //buffer to store incoming data

// art net parameters
unsigned int localPort = 6454; // artnet UDP port is by default 6454
const int art_net_header_size = 17;
const int max_packet_size = 576;
char ArtNetHead[8] = "Art-Net";
char OpHbyteReceive = 0;
char OpLbyteReceive = 0;

short incoming_universe = 0;
boolean is_opcode_is_dmx = 0;
boolean is_opcode_is_artpoll = 0;
boolean match_artnet = 1;
short Opcode = 0;
EthernetUDP Udp;

void setup()
{

  Serial.begin(115200);
  //Serial.print("Amount Universes: ");
  //Serial.println(amountUniverses);

  //setup ethernet and udp socket
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  FastLED.addLeds<WS2812, pin1, GRB>(leds1, pix1);
  FastLED.addLeds<WS2812, pin2, GRB>(leds2, pix2);
  FastLED.addLeds<WS2812, pin3, GRB>(leds3, pix3);
  FastLED.addLeds<WS2812, pin4, GRB>(leds4, pix4);
  FastLED.addLeds<WS2812, pin5, GRB>(leds5, pix5);
  FastLED.addLeds<WS2812, pin6, GRB>(leds6, pix6);
  FastLED.addLeds<WS2812, pin7, GRB>(leds7, pix7);
  FastLED.addLeds<WS2812, pin8, GRB>(leds8, pix8);

  // ------Setting up the arrays that contain what Artnet data to read and insert where (not used atm)    <<<<<<<<<<<<<<<<<<<<<<<
  /*
    for (int i = 0; i < amountUniverses; i++) {
    start_address[i] = 0;
    channelsToRead[i] = constrain(channelsLeft, 0, 510);
    //startPixel[i] = i * 170;
    channelsLeft = channelsLeft - channelsToRead[i];
    }
  */
}

void loop()
{

  int packetSize = Udp.parsePacket(); //Checks for the presence of a UDP packet, and reports the size
  //Serial.println(packetSize);

  if (packetSize)
  { //check size to avoid unneeded checks

    Udp.read(packetBuffer, MAX_BUFFER_UDP);

    //read header
    match_artnet = 1;
    for (int i = 0; i < 7; i++)
    {
      //if not corresponding, this is not an artnet packet, so we stop reading
      if (char(packetBuffer[i]) != ArtNetHead[i])
      {
        match_artnet = 0;
        break;
      }
    }

    //if its an artnet header, start sniffing
    if (match_artnet == 1)
    {

      Opcode = bytes_to_short(packetBuffer[9], packetBuffer[8]);

      //if opcode is DMX type
      if (Opcode == 0x5000)
      {
        is_opcode_is_dmx = 1;
        is_opcode_is_artpoll = 0;
      }

      //if opcode is artpoll
      else if (Opcode == 0x2000)
      {
        is_opcode_is_artpoll = 1;
        is_opcode_is_dmx = 0;
        //( we should normally reply to it, giving ip adress of the device)
      }

      //if its DMX data we will read it now
      if (is_opcode_is_dmx = 1)
      {

        //read incoming universe
        incoming_universe = bytes_to_short(packetBuffer[15], packetBuffer[14]);
        incoming_universe = incoming_universe - startUniverse; // just to reset it to a range starting from zero

        if (incoming_universe >= 0 && incoming_universe < totalUniverses)
        {
          memcpy(stripNr[incoming_universe], &packetBuffer[art_net_header_size +1], endChannel[incoming_universe]);
        }
      }
    } //end of sniffing
  }

EVERY_N_MILLISECONDS(40) { FastLED.show(); }   //update rate of leds (25fps = 40ms, 30fps = 33.33ms, 45fps = 22.22ms)

}
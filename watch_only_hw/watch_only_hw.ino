#include <Bitcoin.h>
#include <OpCodes.h>

// comment this line if you are using 
// resistive touchscreen
#define USE_FT6206 // capacitive touchscreen

#include <SPI.h>
#include "Adafruit_GFX.h"      // graphics library
#include "Adafruit_ILI9341.h"  // tft screen

// touchscreen
#ifdef USE_FT6206
  #include <Wire.h>      // this is needed for FT6206
  #include "Adafruit_FT6206.h"
#else
  #include "Adafruit_STMPE610.h"
#endif

#include "qrcode.h"

// SD card chip-select
#define SD_CS 4

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

// This line will only work if you solder pins to use ICSP header (and hardware SPI)
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#ifdef USE_FT6206
  Adafruit_FT6206 ts = Adafruit_FT6206();
#else
  // This is calibration data for the raw touch data to the screen coordinates
  #define TS_MINX 150
  #define TS_MINY 130
  #define TS_MAXX 3800
  #define TS_MAXY 4000
  #define STMPE_CS 8
  Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
#endif

// single key example
// state liquid toast lunch crunch profit lake dinner actress diamond trigger mushroom afford style path
HDPublicKey masterPubkey("xpub6BhFTNLmsibx7kp51zuLkDvfz9qKcG2QF5Qzf6dWJMf8MVHGfPu1fSvVrwhGKdBxbZSBUNxWqnwR1JW7aN8bU984drh5gGUz5W2YKAxeado");

// 2-of-3 multisig
// trezor T
HDPublicKey key1("xpub6APSzbcJN1zTroLEViJtjCF3cAVWwQuFTX36PxHaMGXbVbSpHqAGsrs1xmRsEPmKHtsQ8XKrii8hw9otbgEpQ674eiw2dBKkuu91eeFLWKK");
// ledger Blue
HDPublicKey key2("xpub6A6UyKUUPQoK6JBcHc1gLhCYkkybTZGRpBs7Fmu3mtWWEQnqXGcWTVQJgaWsQiydohgJsf9L7Edrg5rLmbMN6NDHV8k65zmcYRaH5iVhsgb");
// ledger Nano
HDPublicKey key3("xpub6Bb95ETB27TXcHw3LUugns8KEoXSQcrzu5kD424589yqu4ZDf8wPqcS6JHHnapnKdy8RKeDeDUkYyhsZAbxJYezjzXBVak17SnX4XfUb3d9");

unsigned int i = 0;
bool use_change = false;

void displayAddress(String addr, String derivation, bool eraseScreen=true){
  if(eraseScreen){
    tft.fillScreen(ILI9341_WHITE);
  }

  if(use_change){
    tft.setCursor(tft.width()/2 - 11*6, 20);
    tft.println("Your change address:");
  }else{
    tft.setCursor(tft.width()/2 - 12*6, 20);
    tft.println("Your receiving address:");
  }
  int x0 = tft.width()/2 - addr.length()*6/2;
  if(x0 < 0){ x0 = 0; }
  tft.setCursor(x0, 35);
  tft.println(addr);

  x0 = tft.width()/2 - (derivation.length() + 17)*6/2;
  if(x0 < 0){ x0 = 0; }
  tft.setCursor(x0, 50);
  tft.println("Derivation path: " + derivation);

  char buf[100] = { 0 }; // char array much larger than we would ever need
  addr.toCharArray(buf, sizeof(buf));
  showQRcode(buf, 70);
}

void showQRcode(char * text, int y0){
  // Start time
  uint32_t dt = millis();

  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = strlen(text);
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }

  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, text);

  int width = 17 + 4*qrSize;
  int scale = (tft.width()-20)/width;
  int padding = (tft.width() - width*scale)/2;
  for (uint8_t y = 0; y < qrcode.size; y++) {
      for (uint8_t x = 0; x < qrcode.size; x++) {
          if(qrcode_getModule(&qrcode, x, y)){
            tft.fillRect(padding+scale*x, y0+scale*y, scale, scale, ILI9341_BLACK);
          }
      }
  }
}

String generateMultisigAddress(bool change, unsigned int ind){
  Script redeemScript;
  redeemScript.push(OP_2);

  PublicKey cosigners[3] = {
    key1.child(change).child(ind).publicKey,
    key2.child(change).child(ind).publicKey,
    key3.child(change).child(ind).publicKey
  };

  // Careful here. Wallets should sort children public keys lexagraphicaly.
  // Electrum sorts the keys. But others may not do it.
  // https://github.com/bitcoin/bips/blob/master/bip-0045.mediawiki
  
  // Pushing pubkeys to script in alphabetical order
  for(int i=0; i<3; i++){
    PublicKey maxKey = cosigners[0];
    int k = 0;
    for(int j=0; j<3; j++){
      if(cosigners[j].isValid()){
        if((!maxKey.isValid()) || (String(cosigners[j]) < String(maxKey))){
          maxKey = cosigners[j];
          k = j;
        }
      }
    }
    redeemScript.push(maxKey);
    if(k>=0){
      memset(cosigners[k].point, 0, 64);
    }
  }

  redeemScript.push(OP_3);
  redeemScript.push(OP_CHECKMULTISIG);

  // Constructing ScriptPubkey and address for redeem script
  Script scriptPubkey = redeemScript.scriptPubkey();
  return scriptPubkey.address();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);  tft.setTextSize(1);
  tft.println();

  if (!ts.begin()) {
    tft.println("Couldn't start touchscreen controller");
    while(1);
  }

  // showing first address
  // can do single key or multisig
  
//  HDPublicKey pubkey = masterPubkey.child(0).child(i);
//  displayAddress(pubkey.address(), "m/0/0");

  String addr = generateMultisigAddress(use_change, 0);
  displayAddress(addr, "m/"+String(int(use_change))+"/0");
}

void loop() {
  delay(10);
#ifdef USE_FT6206
  if(ts.touched()){
#else
  if(!ts.bufferEmpty()){
#endif
    TS_Point p = ts.getPoint();
#ifdef USE_FT6206
    p.x = map(p.x, 0, 240, tft.width(), 0);
    p.y = map(p.y, 0, 320, tft.height(), 0);
#else
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
#endif
    Serial.print(p.x);
    Serial.print(" ");
    Serial.println(p.y);

    tft.fillScreen(ILI9341_WHITE); // erase screen just to see that we are doing something

    // touch on top part switches between receiving or change addresses
    if(p.y < tft.height()/3){
      use_change = !use_change;
    }else{ // touch on bottom right or left part increases or decreases address index
      if(p.x > tft.width()/2){
        i++;
      }else{
        i--;
      }
      if(i<0){
        i=0;
      }
    }
    // HDPublicKey pubkey = masterPubkey.child(0).child(i);
    // displayAddress(pubkey.address(), "m/0/"+String(i));
    String addr = generateMultisigAddress(use_change, i);
    displayAddress(addr, "m/"+String(int(use_change))+"/"+String(i), false);
    
    delay(300); // delay to distinguish between touches
#ifndef USE_FT6206
    while(!ts.bufferEmpty()){ // empty touch events
      ts.getPoint();
    }
#endif
  }
}

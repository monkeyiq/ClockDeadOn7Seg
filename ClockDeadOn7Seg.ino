//We always have to include the library
#include "LedControl.h"

#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <RTC_DS3234.h>
const int  cs=10; //chip select 
RTC_DS3234 RTC( cs );

// serial seven seg display
const int ssdcs = 9;


////////////

uint8_t DS3234_get_addr(const uint8_t pin, const uint8_t addr)
{
    uint8_t rv;

    digitalWrite(cs, LOW);
    SPI.transfer(addr);
    rv = SPI.transfer(0x00);
    digitalWrite(cs, HIGH);
    return rv;
}

float DS3234_get_treg(const uint8_t pin)
{
    float rv;
    uint8_t temp_msb, temp_lsb;
    int8_t nint;

    temp_msb = DS3234_get_addr(pin, 0x11);
    temp_lsb = DS3234_get_addr(pin, 0x12) >> 6;
    if ((temp_msb & 0x80) != 0)
        nint = temp_msb | ~((1 << 8) - 1); // if negative get two's complement
    else
        nint = temp_msb;

    rv = 0.25 * temp_lsb + nint;

    return rv;
}

////////////


/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 5 is connected to the DataIn 
 pin 6 is connected to the CLK 
 pin 2 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(5,6,2,1);

/* we always wait a bit between updates of the display */
unsigned long delaytime=250;

int toSpaceOr09( int t )
{
  if( !t )
     t = ' ';
  return(t);

}
//Given a number, spiSendValue chops up an integer into four values and sends them out over spi
void showTemp(int tempCycles)
{
    SPI.setClockDivider(SPI_CLOCK_DIV64); //Slow down the master a bit
     SPI.setDataMode( SPI_MODE0 );
//  digitalWrite(ssdcs, LOW); //Drive the CS pin low to select OpenSegment
//  SPI.transfer('v'); //Reset command
//  digitalWrite(ssdcs, HIGH); //Release the CS pin to de-select OpenSegment

  digitalWrite(ssdcs, LOW); //Drive the CS pin low to select OpenSegment
  int t = tempCycles / 1000;
  if( !t )
     t = ' ';
  SPI.transfer(t); //Send the left most digit
  tempCycles %= 1000; //Now remove the left most digit from the number we want to display
  tempCycles / 100;
  if( !t )
     t = ' ';
  SPI.transfer(t);
  tempCycles %= 100;
  t = tempCycles / 10;
  if( !t )
     t = ' ';
  SPI.transfer(t);
  tempCycles %= 10;
  SPI.transfer(tempCycles); //Send the right most digit
  digitalWrite(ssdcs, HIGH); //Release the CS pin to de-select OpenSegment
}

//Given a number, spiSendValue chops up an integer into four values and sends them out over spi
void showTemp(float floatval)
{
  Serial.println(floatval);
    SPI.setClockDivider(SPI_CLOCK_DIV64); //Slow down the master a bit
     SPI.setDataMode( SPI_MODE0 );
//  digitalWrite(ssdcs, LOW); //Drive the CS pin low to select OpenSegment
//  SPI.transfer('v'); //Reset command
//  digitalWrite(ssdcs, HIGH); //Release the CS pin to de-select OpenSegment
  digitalWrite(ssdcs, LOW); 
  SPI.transfer(0x77); 
  SPI.transfer(1<<2); 
  digitalWrite(ssdcs, HIGH);

  int tempCycles = floor( floatval );
  digitalWrite(ssdcs, LOW); //Drive the CS pin low to select OpenSegment
  tempCycles %= 1000;
  SPI.transfer( toSpaceOr09(tempCycles / 100) );
  tempCycles %= 100;
  SPI.transfer( toSpaceOr09(tempCycles / 10) );
  tempCycles %= 10;
  SPI.transfer( toSpaceOr09(tempCycles ) ); 
  floatval *= 10;
  tempCycles = floor(floatval);
  tempCycles %= 10;
  SPI.transfer(tempCycles);
  
  digitalWrite(ssdcs, HIGH); //Release the CS pin to de-select OpenSegment
}


void setup() 
{
   pinMode(ssdcs, OUTPUT);
   digitalWrite(ssdcs, HIGH); //By default, don't be selecting OpenSegment
  SPI.begin(); //Start the SPI hardware
    SPI.setClockDivider(SPI_CLOCK_DIV64); //Slow down the master a bit
  //Send the reset command to the display - this forces the cursor to return to the beginning of the display
  digitalWrite(ssdcs, LOW); //Drive the CS pin low to select OpenSegment
  SPI.transfer('v'); //Reset command
     digitalWrite(ssdcs, HIGH); //By default, don't be selecting OpenSegment

  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
  
  Serial.begin(57600);
  Serial.setTimeout( 10* 1000 );
  SPI.begin();
  RTC.begin();
 // if (! RTC.isrunning()) 
 {
   // nb: to get this epcoh number you can use    date +"%s"
   Serial.println("Please tell me the time in epoch second format:");
   Serial.flush();
   char buffer[100];
   int length = 99;
   memset( buffer, 0, length );
   Serial.readBytesUntil('\n', buffer, length);
   if( strlen(buffer))
   {
     uint32_t tt = atol(buffer);
     DateTime dt(tt);
     RTC.adjust( dt );
   
      Serial.print("Setting time to... ");
      Serial.println(dt.toString(buffer,length));
    }
  }
}



void loop() { 

    const int len = 32;
    static char buf[len];

    DateTime now = RTC.now();
    int h = now.hour();
    int m = now.minute();
//    lc.clearDisplay(0);
    if( h >= 10 ) 
      lc.setDigit(0,0, h/10,false);
    else
      lc.setChar(0,0,' ',false );
    lc.setDigit(0,1,h%10,false);
    lc.setDigit(0,2,m/10,false);
    lc.setDigit(0,3,m%10,false);

    Serial.println(now.toString(buf,len));
    
//    showTemp( now.second() );
    showTemp( DS3234_get_treg(cs) );

      
  delay(1000);
}

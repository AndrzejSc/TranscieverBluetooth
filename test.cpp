#include <Arduino.h>
#include<SPI.h>
#include<nRF24L01.h>
#include<RF24.h>
const uint64_t pipe[1] = {0xF0F0F0F0E1LL};
RF24 radio(8, 9);
byte ackMessg[5] = {0};
byte x[5] = {15,16,17,18,19};
boolean state = true;
void setup()
{
    Serial.begin(9600);
    radio.begin();
    radio.setDataRate(RF24_250KBPS);
    radio.setChannel(76); // default 76
    radio.enableAckPayload();

    radio.openReadingPipe(1, pipe[0]);
    radio.writeAckPayload(1, ackMessg, sizeof(ackMessg));
}
void loop()
{
    radio.writeAckPayload(1, ackMessg, sizeof(ackMessg));
    /*if ( radio.available() )
    {
      radio.read( &y, sizeof(y) );
      Serial.print("x have been received from 1 : ");
      Serial.println(y);
    }*/
    if (state)
    {
        radio.stopListening();
        radio.write(x, sizeof(x));
        for (int i = 0; i < sizeof(x); i++)
        {
            Serial.print("x[");
            Serial.print(i, DEC);
            Serial.print("]=");
            Serial.println(x[i]);
            delay (200);
            radio.startListening();
            state = false;
        }
    }
    else
    {
        if ( radio.isAckPayloadAvailable())
        {
            radio.read(ackMessg, sizeof(ackMessg));
            for (int i = 0; i < sizeof(ackMessg); i++)
            {
                Serial.print("ackMessg[");
                Serial.print(i, DEC);
                Serial.print("]=");
                Serial.println(ackMessg[i]);
                delay (200);
            }
        }
        state = true;
    }
}
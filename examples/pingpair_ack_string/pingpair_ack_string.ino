/*
  // August 2016 - shinjanxp - Modified from tmrh20's pingpair example to send strings
*/
/**
 * Example for efficient call-response using ack-payloads 
 *
 * Similar to TMRH20's pingpair example with ack payload enabled to test sending strings 
 * of varying lengths
 */
 
#define PAYLOAD_SIZE 4 //set the size of data to be sent

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(9,10);

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles
role_e role = role_ping_out;                                              // The role of the current running sketch

// A single byte to keep track of the data being sent back and forth
unsigned long counter = 999999;
char *payload = "ABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCDABCD";
void setup(){

  Serial.begin(115200);
  printf_begin();
  Serial.print(F("\n\rRF24/examples/throughput_with_ack/\n\rROLE: "));
  printf("Payload size: %d \n",PAYLOAD_SIZE);
  Serial.println(role_friendly_name[role]);
  Serial.println(role);
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));

  // Setup and configure rf radio

  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(PAYLOAD_SIZE);                // Here we are sending 1-byte payloads to test the call-response speed
//  radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
//  radio.openReadingPipe(1,pipes[0]);
//  radio.startListening();                 // Start listening
//  Role settings
if ( role == role_ping_out )
    {
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1,pipes[1]);
    }
    else 
    {     
       radio.openWritingPipe(pipes[1]);
       radio.openReadingPipe(1,pipes[0]);
       radio.startListening();
    }
  
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void) {

  if (role == role_ping_out){
    
    radio.stopListening();                                  // First, stop listening so we can talk.
        
    printf("Now sending %s as payload. ",payload);
    unsigned long gotByte;  
    char ackPayload[PAYLOAD_SIZE];
    unsigned long time = micros();                          // Take the time, and send it.  This will block until complete   
                                                            //Called when STANDBY-I mode is engaged (User is finished sending)
    if (!radio.write( payload, PAYLOAD_SIZE )){
      Serial.println(F("failed."));      
    }else{

      if(!radio.available()){ 
        Serial.println(F("Blank Payload Received.")); 
      }else{
        while(radio.available() ){
          unsigned long tim = micros();
          radio.read( ackPayload, PAYLOAD_SIZE );
          printf("Got response %s, round-trip delay: %lu microseconds\n\r",ackPayload,tim-time);
          counter++;
        }
      }

    }
    // Try again later
    delay(1000);
  }

  // Pong back role.  Receive each packet, dump it out, and send it back

  if ( role == role_pong_back ) {
    byte pipeNo;
    char ackPayload[PAYLOAD_SIZE];
    unsigned long gotByte;                                       // Dump the payloads until we've gotten everything
    while( radio.available(&pipeNo)){
      radio.read( ackPayload, PAYLOAD_SIZE );
      printf("Got %s \n",ackPayload);
      radio.writeAckPayload(pipeNo,ackPayload, min(16,PAYLOAD_SIZE) );    
   }
 }

  // Change roles

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == role_pong_back )
    {
      Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));

      role = role_ping_out;                  // Become the primary transmitter (ping out)
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1,pipes[1]);
    }
    else if ( c == 'R' && role == role_ping_out )
    {
      Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));
      
       role = role_pong_back;                // Become the primary receiver (pong back)
       radio.openWritingPipe(pipes[1]);
       radio.openReadingPipe(1,pipes[0]);
       radio.startListening();
    }
  }
}

// If You want to see debug infos on Serial interface
#define DEBUGGING

// These are Analogic pins
#define RPin 0  // A0 for Red
#define GPin 1  // A1 for Green
#define BPin 2  // A2 for Blue

#include <SPI.h>
#include <Ethernet.h>
// This is the include for 
#include <WebSocketServer.h>

int lastR = -1;
int lastG = -1;
int lastB = -1;

char hexColor[7] = "000000";
char hexNumbers[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// IMPORTANT ! Don't forget to set the arduino's IP Address !!!!!
IPAddress ip(192,168,1,120);
//IPAddress ip(150,1,3,78);

EthernetServer server(80);

void setup() 
{
  Ethernet.begin(mac, ip);

  // Intialize Serial comms
  Serial.begin(9600);
  
  server.begin();
  
  #ifdef DEBUGGING
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP()); 
  Serial.println("Entering Loop...");
  #endif
}

void checkForDie(EthernetClient pClient)
{
  uint8_t msgtype;
  unsigned int length;
  uint8_t mask[4];
  char data[10];
  unsigned int i;

  if (pClient.available())
  {
    msgtype = pClient.read();
    #ifdef DEBUGGING
    Serial.print("Message Type : ");
    Serial.println(msgtype, HEX);
    #endif
              
    length = pClient.read() & 127;
    #ifdef DEBUGGING
    Serial.print("Length : ");
    Serial.println(length);
    #endif
            
    mask[0] = pClient.read();
    mask[1] = pClient.read();
    mask[2] = pClient.read();
    mask[3] = pClient.read();
            
    #ifdef DEBUGGING
    Serial.print("Received : ");
    #endif
    for (i=0; i<length; ++i) 
    {
      data[i] = (char) (pClient.read() ^ mask[i % 4]);
      #ifdef DEBUGGING
      Serial.print(data[i]);
      #endif
    }
    #ifdef DEBUGGING
    Serial.println(".");
    #endif

    if (data[0] == 'D' && data[1] == 'I' && data[2] == 'E')            
    {
      pClient.stop();
      #ifdef DEBUGGING
      Serial.println("Stop Requested!");
      #endif
    }
  }
}

void loop() 
{
  lastR = -1;
  lastG = -1;
  lastB = -1;

  EthernetClient client = server.available();
  if (client)
  {
    #ifdef DEBUGGING
    Serial.println("New Client.");
    #endif
    
    if (client.connected()) 
    {
      #ifdef DEBUGGING
      Serial.println("Client is connected");
      #endif
      
      websocket::ServerHandshake wsck = websocket::ServerHandshake(client, "");
      if (wsck.run())
      {
        while (client.connected())
        {
          // Check if connected Client wants to end the conversation (with a DIE message)
          checkForDie(client);

          // Read RBG values from analog inputs
          int currR = map(analogRead(RPin), 0, 1023, 0, 255);
          int currG = map(analogRead(GPin), 0, 1023, 0, 255);
          int currB = map(analogRead(BPin), 0, 1023, 0, 255);
  
          // Treat them only if at least one has changed (otherwise is a waste of resources)
          if (currR != lastR || currG != lastG || currB != lastB)
          {
            SetHexVal(0, currR);
            SetHexVal(2, currG);
            SetHexVal(4, currB);
            hexColor[6] = 0;
    
            #ifdef DEBUGGING
            Serial.print(currR);
            Serial.print("/");
            Serial.print(currG);
            Serial.print("/");
            Serial.print(currB);
            Serial.print(" ==> ");
            Serial.print("Color is : #");
            Serial.println(hexColor);
            #endif
            
            lastR = currR;
            lastG = currG;
            lastB = currB;
        
            // Send data to the client (in websocket way)
            client.write(0x81);
            int size = strlen(hexColor);
            if (size > 125) 
            {
              client.write(126);
              client.write((uint8_t) (size >> 8));
              client.write((uint8_t) (size && 0xFF));
            } 
            else 
            {
              client.write((uint8_t) size);
            }

            for (int i=0; i<size; ++i) 
            {
              client.write(hexColor[i]);
            }          
          }
        }
    
        #ifdef DEBUGGING
        Serial.println("Client is gone...");
        #endif

      }
   
      delay(1);
    }
  }
}



void SetHexVal(int pOffset, int pDecVal)
{
  int highVal = pDecVal / 16;
  int lowVal = pDecVal % 16;
  
  hexColor[pOffset] = hexNumbers[highVal];
  hexColor[pOffset + 1] = hexNumbers[lowVal];
}

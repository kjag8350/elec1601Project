 //-----------------------------------------------------------------------------------------------------------//
//                                                                                                           //
//  Slave_ELEC1601_Student_2019_v3                                                                           //
//  The Instructor version of this code is identical to this version EXCEPT it also sets PIN codes           //
//  20191008 Peter Jones                                                                                     //
//                                                                                                           //
//  Bi-directional passing of serial inputs via Bluetooth                                                    //
//  Note: the void loop() contents differ from "capitalise and return" code                                  //
//                                                                                                           //
//  This version was initially based on the 2011 Steve Chang code but has been substantially revised         //
//  and heavily documented throughout.                                                                       //
//                                                                                                           //
//  20190927 Ross Hutton                                                                                     //
//  Identified that opening the Arduino IDE Serial Monitor asserts a DTR signal which resets the Arduino,    //
//  causing it to re-execute the full connection setup routine. If this reset happens on the Slave system,   //
//  re-running the setup routine appears to drop the connection. The Master is unaware of this loss and      //
//  makes no attempt to re-connect. Code has been added to check if the Bluetooth connection remains         //
//  established and, if so, the setup process is bypassed.                                                   //
//                                                                                                           //
//-----------------------------------------------------------------------------------------------------------//

#include <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>   

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1



// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 13;

// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);


Servo servoRight;
Servo servoLeft;
Servo servoHanger;

char instruction[1200];  // memory
int count = 0;   // intruction counter
int pos = 0;
void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    servoRight.attach(13);              // Attach right servo to pin 13
    servoLeft.attach(12);              // Attach left servo to pin 12
    servoHanger.attach(14);
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    pinMode(5,INPUT);pinMode(10,OUTPUT);//Front IR LED &Receiver
    pinMode(8,INPUT);pinMode(2,OUTPUT);//Left IR LED &Receiver
    pinMode(4,INPUT);pinMode(2,OUTPUT);//Right IR LED &Receiver
    pinMode(9,INPUT);pinMode(2,OUTPUT);//Left2 IR LED &Receiver
    pinMode(3,INPUT);pinMode(2,OUTPUT);//Right2 IR LED &Receiver
  
    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}


void loop()
{   
    //This is Set up 
    char recvChar;   // recieving char from master
    boolean isMazeOn = true;
    boolean isTrackOn = false;
    boolean isTrap = false;

    int irFront;     // front object sensore
    int irLeft;      // first left object sensor
    int irRight;     // first right object sensor
    int irLeft2;     // second left object sensor
    int irRight2;    // second right object sensor
    int mode = 1;    // robots mode  
    
    // Main Loop
    while(1)
    {      //jj

       irFront = irDetect(10, 5, 38000);       // Check for object
  irLeft = irDetect(2, 4, 38000);       // Check for object
  irLeft2 = irDetect(2, 3, 38000);       // Check for object
  irRight = irDetect(2, 8, 38000);       // Check for object
  irRight2 = irDetect(2, 9, 38000); 
  
  Serial.print(irFront);
  Serial.print(" ");
  Serial.print(irLeft);                    // Display 1/0 no detect/detect
  Serial.print(" ");
  Serial.print(irLeft2);   
  Serial.print(" ");
  Serial.print(irRight);                    // Display 1/0 no detect/detect
  Serial.print(" ");
  Serial.println(irRight2);  
//       delay(300);   
        if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
        {
            recvChar = blueToothSerial.read();
            Serial.print(recvChar);
            if (recvChar == '1'){  // Change mode to Stand By mode
                isMazeOn = false;  // incase user forget to stop
                isTrackOn = false; // incase user forget to stop 
                
                mode = 1; 
            }
            if (recvChar == '2'){  // Change mode to Manual nagivation mode
                mode = 2;
            }
            if (recvChar == '3'){  // Change mode to Maze nagivation mode
                mode = 3;
            }
            if (recvChar == '4'){  // Change mode to Track navigation mode
                mode = 4;
            }
            if (mode == 1) {                      // *******Standby mode commands ******

                if (recvChar == 'c'){             // c - clear instructions (aka, overwrite index)
                    count = 0;
                }

                if (recvChar == 'p'){             // p - play instructions 
                    for(int i =0; i < count; i++) {
                        repeat(i);
                    }
                
                }
                if (recvChar == 'r'){             // r - play instruction (reverse order)
                    for(int i =0; i < count; i++) {
                        replay(i);
                    }
                }
                if (recvChar == 't'){             // t - trap pingpong
                    trap();
                    isTrap = true;
                }
                if (recvChar == 'u'){             // u - untrap pingpong
                    unTrap();
                    isTrap = false;

                }
                
            }
            if (mode == 2) {                      // *******Manual mode commands ******
              
                if (recvChar == 'w') {            // w - move forward
                    moveForward();
                    instruction[count] = 'w';
                    count++;
                }
                if (recvChar == 's') {            // s - move backward
                    moveBackward();
                    instruction[count] = 's';
                    count++;
                }
                if (recvChar == 'a') {            // a - turn left (around 30 degree)
                    turnLeft();
                    instruction[count] = 'a';  
                    count++;
                }
                if (recvChar == 'd') {            // d - turn right ( around 30 degree)
                
                    turnRight();
                    instruction[count] = 'd';
                    count++;
                } 
            }
            if (mode == 3) {                      // *******Maze navigation mode commands ******
              
                if (recvChar == 's') {            // s - start 
                    isMazeOn = true;
                }
                if (recvChar == 'e') {            // e - stop
                    isMazeOn = false;
                }
            }

            if (mode == 4) {                      // *******Track navigation mode commands ******
              
                if (recvChar == 's') {            // s - start 
                    isTrackOn = true;
                }
                if (recvChar == 'e') {            // e - stop
                    isTrackOn = false;
                }
            }

  
            if (isTrap){
                servoHanger.write(120);   // if trap is true , servo hanger at position 120
            }
            else {
                servoHanger.write(0);     // if false, servo hanger at position 0
            }
            pause();
        }
     
          if (isMazeOn){                       // ****** Maze navigation mode logic ****
                // 1/0   no detect / detect
                irFront = irDetect(10,5,38000);    
                irLeft = irDetect(2,8,38000);
                irRight = irDetect(2,4,38000);
                irLeft2 = irDetect(2,9,38000);
                irRight2 = irDetect(2,3,38000);
                    if ( irLeft2 == 0) {     // right2 detect, robot not balance, turn left a bit
                        autoTurnRight();
                        instruction[count] = 'h';
                        delay(300);       
                                                      pause();
        

                        count++;
                    }
                    else if ( irRight2 == 0) {    // left2 detect, robot not balance,  turn right a bit
                        autoTurnLeft();
                                                delay(300);   
                                                                              pause();
            

                        instruction[count] = 'f';
                        count++;
                    }
                    else if (irRight2 == 1 && irLeft2 == 1){ // robot balanced
                        if (irFront == 1) {             //  -  front not detect, move forward
                            autoMoveForward();
                                                    delay(300);    
                                                                                  pause();
           

                            instruction[count] = 't';
                            count++;
                        }
                        else if (irFront == 0 ){        //  -  front detect
                            if ( irRight  == 0) {       //  ------ right not detect, turn left 
                                turnLeft();             //    *******maybe auto turn left better****
                                instruction[count] = 'a';
                                                        delay(300);   
                                                                                      pause();
            

                                count++;
                            }
                            else if (irRight == 0) {
                                turnRight();            //    *******maybe auto turn right better****
                                instruction[count] = 'd';
                                                        delay(300);
               

                                count++;
                            }
                            else {
                              pause();
                              delay(300);
                                //DEAD END 
                                //Maybe add 1 more sensor to detect ball, (Only have pin for 1)
                                blueToothSerial.print("Found object!");  // sent to master  
                               // isMazeOn = false;
                            }
                        }
                    } 
         }
         
        if(Serial.available())            // Check if there's any data sent from the local serial terminal. You can add the other applications here.
        {
            recvChar  = Serial.read();
            Serial.print(recvChar);
            blueToothSerial.print(recvChar);
        }
    }
}

void repeat(int index){
  
    char commandNow  = instruction[index];
    
    if (commandNow == 'w') {
        moveForward();
    }
    if (commandNow == 's') {
        moveBackward();
    }
    if (commandNow == 'a') {
        turnLeft();
    }
    if (commandNow == 'd') {
        turnRight();
    }
    if (commandNow == 't') {
        autoMoveForward();
    }
    if (commandNow == 'g') {
        autoMoveBackward();
    }
    if (commandNow == 'f') {
        autoTurnLeft();
    }
    if (commandNow == 'h') {
        autoTurnRight();
    }
    pause();
}

void replay(int index ){
  
    char commandNow  = instruction[count - index -1];
    
    if (commandNow == 'w') {
      moveBackward();
    }
    if (commandNow == 's') {
      moveForward();
    }
    if (commandNow == 'a') {
      turnRight();
    }
    if (commandNow == 'd') {
      turnLeft();
    }
    if (commandNow == 't') {
      autoMoveBackward();
    }
    if (commandNow == 'g') {
      autoMoveForward();
    }
    if (commandNow == 'f') {
      autoTurnRight();
    }
    if (commandNow == 'h') {
      autoTurnLeft();
    }
    pause();
}

void moveForward(){                             //Manaual forward
    servoRight.writeMicroseconds(1700);
    servoLeft.writeMicroseconds(1300);
    delay(180);
}
void moveBackward(){                            //Manaual backward
    servoRight.writeMicroseconds(1300);
    servoLeft.writeMicroseconds(1700);
    delay(180);
}
void turnLeft(){                                //Manaual turn left
    servoRight.writeMicroseconds(1300);
    servoLeft.writeMicroseconds(1300);
    delay(179);
}
void turnRight(){                               //Manaual turn right
    servoRight.writeMicroseconds(1700);
    servoLeft.writeMicroseconds(1700);
    delay(179);

}


void autoMoveForward(){                            //auto forward
    servoRight.writeMicroseconds(1700);
    servoLeft.writeMicroseconds(1300);
    delay(20);
}
void autoMoveBackward(){                           //auto backward
    servoRight.writeMicroseconds(1300);
    servoLeft.writeMicroseconds(1700);
    delay(20);
}
void autoTurnLeft(){                               //auto turn left
    servoRight.writeMicroseconds(1300);
    servoLeft.writeMicroseconds(1300);
    delay(20);
}
void autoTurnRight(){                              //auto turn right
    servoRight.writeMicroseconds(1700);
    servoLeft.writeMicroseconds(1700);
    delay(20);

}

void pause(){                                       // pause
    servoRight.writeMicroseconds(1500);
    servoLeft.writeMicroseconds(1500);
}

void trap(){
  while ( pos <120){
    servoHanger.write(pos++);          
    delay(15);                       
  }
}

void unTrap(){
  while ( pos > 0){
    servoHanger.write(pos--);          
    delay(15);                       
  }
}
int irDetect(int irLedPin, int irReceiverPin, long frequency)
{
    tone(irLedPin, frequency, 8);              // IRLED 38 kHz for at least 1 ms
    delay(1);                                  // Wait 1 ms
    int ir = digitalRead(irReceiverPin);       // IR receiver -> ir variable
    delay(1);                                  // Down time before recheck
    return ir;                                 // Return 1 no detect, 0 detect
} 


void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}

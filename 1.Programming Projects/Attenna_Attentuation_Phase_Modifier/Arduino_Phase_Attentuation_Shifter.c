/* Shift Register Configuration Variables */
//Pin connected to SRCLK of 74HC595
int clockPin = 5;
//Pin connected to RCLK of 74HC595
int latchPin = 6;
////Pin connected to SER of 74HC595
int dataPin = 7;
/* GPIO Outputs*/
int antenna1 = 8;
int antenna2 = 9;
int antenna3 = 10;
int pwm1 = 2;
int pwm2 = 3;
int pwm3 = 4;
/* State Variables */
byte inputsize[6];
int current_state[6] = {0,0,0,0,0,0}; 
int prev_state[6] = {999,999,999,999,999,999}; //initially invalid state values
/* Counting varaibles */
int packetcount = 0;
int count = 0;
int input = 0;
int i;
int len;
/* Control Variables */
bool att1_decimal = false;
bool att2_decimal = false;
bool att3_decimal = false;
bool c_pwm1 = false; 
bool c_pwm2 = false;
bool c_pwm3 = false;
bool c_att1 = false;
bool c_att2 = false;
bool c_att3 = false;
bool c_i = true;

void setup() {
  pinMode(pwm1, OUTPUT);
  pinMode(pwm2, OUTPUT);
  pinMode(pwm3, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(antenna1, OUTPUT);
  pinMode(antenna2, OUTPUT);
  pinMode(antenna3, OUTPUT);
  digitalWrite(antenna1, LOW);
  digitalWrite(antenna2, LOW);
  digitalWrite(antenna3, LOW);
  attenuation(0);
  Serial.begin(9600);
  while(!Serial) {
    
  }
  SerialUSB.begin(9600);
}

void loop() {
  // Serial.println("Start");
  // Serial.print(prev_state[0]);
  // Serial.print(", ");
  // Serial.print(prev_state[1]);
  // Serial.print(", ");
  // Serial.print(prev_state[2]);
  // Serial.print(", ");
  // Serial.print(prev_state[3]);
  // Serial.print(", ");
  // Serial.print(prev_state[4]);
  // Serial.print(", ");
  // Serial.println(prev_state[5]);
  // Serial.println("Lets wait");
  SerialUSB.flush();
  while(SerialUSB.available() <= 0) {
  }
  //Serial.println("Start Reading");
  while(SerialUSB.available() > 0) {
    if(count < 6) {
      //Serial.print("inputsize: ");
      inputsize[count] = SerialUSB.read();
      //Serial.println(inputsize[count]);
      count++;
    }
    else{
      len = inputsize[packetcount];
      int adder = 0;
      for(int i = len-1; i >= 0; i--) {
        if(i == 0) {
            input = SerialUSB.read();
            if((packetcount == 1 || packetcount == 3 || packetcount == 5)){
              if(input == 5) {
                if(packetcount == 1) {
                  att1_decimal = true;  
                }
                else if(packetcount == 3) {
                  att2_decimal = true;   
                }
                else if(packetcount == 5) {
                  att3_decimal = true;  
                }
                else {
                  Serial.println("Error: Unexpeceted Decimal Check");
                }
              }
              else {
                if(packetcount == 1) {
                  att1_decimal = false;
                }
                else if(packetcount == 3) {
                  att2_decimal = false;  
                }
                else if(packetcount == 5) {
                  att3_decimal = false;
                }
                else {
                  Serial.println("Error: Unexpeceted Decimal Check");
                }
              }
            }
            adder += input;
        }
        else if(i == 1) {
          input = SerialUSB.read();
          adder += input * 10;
        }
        else if(i == 2) {
          input = SerialUSB.read();
          adder += input * 100;
        }
        else {
          Serial.println("Error: Unexpected Size Input");
        }
      }      
      //Serial.print("input: ");
      current_state[packetcount] = adder;
      //Serial.println(current_state[packetcount]);
      packetcount++;
      
    }
  }
  packetcount = 0;
  count = 0;  
  //Serial.println("Done reading"); 
  //Serial.println("Is it a new packet");
  for(i = 0; i < 6; i++) {
    if(current_state[i] != prev_state[i]) {
      //Serial.print("current: ");
      //Serial.print(current_state[i]);
      //Serial.print(" vs ");
      //Serial.print("previous: ");
      //Serial.println(prev_state[i]);
      if(i == 0) {
        c_pwm1 = true;
        c_i = false; 
      }
      if(i == 1) {
        c_att1 = true;
        c_i = false;
      }
      if(i == 2) {
        c_pwm2 = true;
        c_i = false;
      }
      if(i == 3) {
        c_att2 = true;
        c_i = false;
      }
      if(i == 4) {
        c_pwm3 = true;
        c_i = false;
      }
      if(i == 5) {
        c_att3 = true;
        c_i = false;
      }
      }
    }
    if(c_i == false) {
      //Serial.println("New State Change");
      prev_state[0] = current_state[0];     
      prev_state[1] = current_state[1];
      prev_state[2] = current_state[2];
      prev_state[3] = current_state[3];
      prev_state[4] = current_state[4];
      prev_state[5] = current_state[5];
    }
    
  if(c_i == false) {
    //Serial.println("Start Modify");
    if(c_pwm1 == true) {
      //Serial.println("New PWM1");
      //Serial.println(prev_state[0]);
      analogWrite(pwm1, prev_state[0]);
      c_pwm1 = false;
    }
    if(c_pwm2 == true) {
      //Serial.println("New PWM2");
      //Serial.println(prev_state[2]);
      analogWrite(pwm2, prev_state[2]);
      c_pwm2 = false;
    }
    if(c_pwm3 == true) {
      //Serial.println("New PWM3");
      //Serial.println(prev_state[4]);
      analogWrite(pwm3, prev_state[4]);
      c_pwm3 = false;
    }
    if(c_att1 == true) {
      //Serial.println("New Att1");
      int adjust;
      adjust = prev_state[1];
      if(att1_decimal == true) {
        adjust -= 5;
        adjust /= 10;
        adjust = adjust << 1;
        adjust = adjust | 1;
      }
      else if (att1_decimal = false){
        adjust /= 10;
        adjust = adjust << 1;
      }
      //Serial.print("adjust: ");
      //Serial.println(adjust, BIN);
      attenuation((byte)adjust);
      delay(1000);
      toggle_antenna_we(1);
      c_att1 = false;
    }
    if(c_att2 == true) {
      //Serial.println("New Att2");
      int adjust;
      adjust = prev_state[3];
      if(att2_decimal == true) {
        adjust -= 5;
        adjust /= 10;
        adjust = adjust << 1;
        adjust = adjust | 1;
      }
      else if (att2_decimal = false){
        adjust /= 10;
        adjust = adjust << 1;
      }
      //Serial.print("adjust: ");
      //Serial.println(adjust, BIN);
      attenuation((byte)adjust);
      delay(1000);
      toggle_antenna_we(2);
      c_att2 = false;
    }
    if(c_att3 == true) {
      //Serial.println("New Att3");
      int adjust;
      adjust = prev_state[5];
      if(att3_decimal == true) {
        adjust -= 5;
        adjust /= 10;
        adjust = adjust << 1;
        adjust = adjust | 1;
      }
      else if (att3_decimal = false){
        adjust /= 10;
        adjust = adjust << 1;
      }
      //Serial.print("adjust: ");
      //Serial.println(adjust,BIN);
      attenuation((byte)adjust);
      delay(1000);
      toggle_antenna_we(3);
      c_att3 = false;
    }
    c_i = true;
  }
  else{
    //Serial.println("No Modifiy Needed");
  }
}

void toggle_antenna_we(int value) {
  if(value == 1) {
    digitalWrite(antenna1, HIGH);
    delay(500);
    digitalWrite(antenna1, LOW);
  }
  else if(value == 2) {
    digitalWrite(antenna2, HIGH);
    delay(500);
    digitalWrite(antenna2, LOW);
  }
  else if(value == 3) {
    digitalWrite(antenna3, HIGH);
    delay(500);
    digitalWrite(antenna3, LOW);
  }
  else {
    
  }
}


void attenuation(byte value) {
  digitalWrite(latchPin, 0);
  shiftOut(value);
  digitalWrite(latchPin, 1); 
}

void shiftOut(byte myDataOut) {
  int i = 0;
  int pinState; 
  
  //initially write 0 to the data and clock pin
  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);
  
  for (i = 7; i >=0; i--) { //iterate 8 bits of a byte data
    digitalWrite(clockPin, 0); //falling clock edges
    
    if (myDataOut & (1 << i)) { //find if i-th bit of a byte data is low or high
      pinState = 1;
    }
    else {
      pinState = 0;
    }
    //Shift a bit per clock-pin cycle    
    digitalWrite(dataPin, pinState);
    digitalWrite(clockPin, 1); //rising clock edge
    digitalWrite(dataPin, 0);
  }
  //Stop shifting
  digitalWrite(clockPin, 0);
}
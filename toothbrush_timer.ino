/*
  Arduino Starter Kit example
 Project 11  - Crystal Ball

 This sketch is written to accompany Project 11 in the
 Arduino Starter Kit

 Parts required:
 220 ohm resistor
 10 kilohm resistor
 10 kilohm potentiometer
 16x2 LCD screen
 tilt switch


 Created 13 September 2012
 by Scott Fitzgerald

 http://www.arduino.cc/starterKit

 This example code is part of the public domain
 */

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// set up a constant for the tilt switchPin
const int switchPin = 6;

byte button_status = 0; //LSB is used to store the button status on the last loop
byte total_duration = 0;
unsigned long next_interval = 0;

//https://omerk.github.io/lcdchargen/
byte gylph_table[][8] = {
  { //0 : incisor
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111
  },
  // 1 : canine 
  {
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,  
    0b01110,
    0b00100
  },
  // 2 : molar
  {
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b11111,  
    0b11011,
    0b00000,
    0b00000
  },
  // 3 : plan view of inscisor
  {
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b00000,  
    0b00000,
    0b00000,
    0b00000
  }  
};

//the lcd can only have 8 custom characters - we invert each character :. we are limited to four
byte glyph_code_pages[3][4] = {
  {0,  1,  2, -1},    //code page 0
  {3, -1, -1, -1}     //code page 1
};

//glyph map indexes elements in the glyph codepage which in turn
//references the glyph table to draw a row of teeth
//the rotateGlyph function is used to draw the mirror image
//the first element is used to index the glyph code page
byte glyph_map[3][9] = {
  {0, 2,2,1,0,0,1,2,2},  //set 0 : anterior surfaces
  {1, 0,0,0,0,0,0,0,0},  //set 1 : biting surfaces
  {0, 2,2,1,0,0,1,2,2}   //set 2 : interior surfaces
};

typedef struct {
    byte  duration;         //duration of instructions
    byte  top_glyph_mask;   //bitmask to flash top glyphset
    byte  bottom_glyph_mask;//bitmask to flash bottom glyphset
    byte  glyph_set;        //set of custom chars to use
} brushing_direction;

//brushing directions
const byte brushing_instruction_count = 15;
brushing_direction brushing_directions[brushing_instruction_count] = {
    //outer surfaces
    {10, 0, B00000111, 0}, //bottom right 
    {10, 0, B00011000, 0}, //bottom center
    {10, 0, B11100000, 0}, //bottom left
    {10, B11100000, 0, 0}, //top left    
    {10, B00011000, 0, 0}, //top center
    {10, B00000111, 0, 0}, //top right
    //biting surfaces
    {10, 0, B11100111, 1}, //bottom teeth
    {10, B11100111, 0, 1}, //top teeth
    //inner surfaces
    {10, B00000111, 0, 2}, //top right
    {10, B00011000, 0, 2}, //top center
    {10, B11100000, 0, 2}, //top left   
    {10, 0, B11100000, 2}, //bottom left         
    {10, 0, B00011000, 2}, //bottom center    
    {10, 0, B00000111, 2}, //bottom right     
    //front
    {10, B00011000, B00011000, 0} //front teeth
};

byte instruction = 0;
byte glyph_map_index = -1;

byte* rotateGlyph(byte pIn[], byte pOut[]){

  for(int i = 0; i < 8; i++){
    pOut[7 - i] = pIn[i];
  }

  return pOut;
}

void setup() {

  Serial.begin(9600);
  
  // set up the switch pin as an input
  pinMode(switchPin, INPUT);  

  //calculate the total duration of the brushing instructions
  for( int i = 0; i < brushing_instruction_count; i++) {
    total_duration += (int)(brushing_directions[i].duration);
  }
  
  // set up the number of columns and rows on the LCD
  lcd.begin(16, 2);  
  lcd.setCursor(0, 0);

  //         ----------------
  lcd.print("Toothbrush Timer");

  // move the cursor to the second line
  lcd.setCursor(0, 1);
  //         ----------------
  lcd.print("Press start");        
}

void loop() {
/*  
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
  // print out the value you read:
  Serial.println(voltage);
*/
  //isolate the LSB in button_status and XOR it with the current digital button status
  if( (button_status & 1) ^ digitalRead(switchPin) ){
      //if we are here then the button state has changed     
      button_status++;

      if( 1 == button_status ) {
          next_interval = millis() + 1000;
          lcd.clear();          
          printChoppers();          
      }
  }

  if( next_interval && millis() >= next_interval ) {
    //seconds--;
    next_interval = millis() + (1000 - (millis() - next_interval));

    lcd.setCursor(0,0);  
    //         ----------------
    //lcd.print(seconds);   
    lcd.print(" ");
  }
}

void printChoppers(){

  //get the current instruction
  brushing_direction *p = &brushing_directions[instruction];

  //is this a new glyph map? 
  if( glyph_map_index != (*p).glyph_set ) {

    //then load new glyphs
    int code_page = glyph_map[ (*p).glyph_set ][0];
    byte temp[8];         
    
    //load page into custom char
    for(int i = 0; i < 4 && -1 != glyph_code_pages[code_page][i]; i++){

        int glyph_table_index = glyph_code_pages[code_page][i];
        
        //then load it
        lcd.createChar(i, gylph_table[ glyph_table_index  ]); 
        
        //rotate 180 for bottom row of teeth
        lcd.createChar(i + 4, rotateGlyph(gylph_table[ glyph_table_index ], temp));       
    }
  }

  lcd.setCursor(8,0);
  //the first item in the glyph map points to the glyph table to use :.
  //we start our loop at index 1
  for(int i = 1; i < 9; i++) {
    lcd.write((byte)glyph_map[ (*p).glyph_set ][i]);
  }

  lcd.setCursor(8,1);
  //the first item in the glyph map points to the glyph table to use :.
  //we start our loop at index 1  
  for(int i = 1; i < 9; i++) {
    lcd.write((byte)glyph_map[ (*p).glyph_set ][i] + 4);
  }
  
  
/*  lcd.write((byte)MOLAR);
  lcd.write((byte)MOLAR);
  lcd.write((byte)CANINE);
  lcd.write((byte)INCISOR);
  lcd.write((byte)INCISOR);
  lcd.write((byte)CANINE);  
  lcd.write((byte)MOLAR);
  lcd.write((byte)MOLAR);

  lcd.setCursor(8,1);
  lcd.write((byte)MOLAR + 3);
  lcd.write((byte)MOLAR + 3);
  lcd.write((byte)CANINE + 3);
  lcd.write((byte)INCISOR + 3);
  lcd.write((byte)INCISOR + 3);
  lcd.write((byte)CANINE + 3);  
  lcd.write((byte)MOLAR + 3);
  lcd.write((byte)MOLAR + 3);
*/  
}


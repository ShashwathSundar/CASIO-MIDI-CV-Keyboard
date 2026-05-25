#include <Keypad.h>
#include <MIDI.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <EEPROM.h>

#define eepromAddr 0
#define defaultCalib 863

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MCP4725 dac;
#define gate 13
const byte ROWS = 5; //four rows
const byte COLS = 8; //three columns
const char keys[ROWS][COLS] = {
{17,18,19,20,21,22,23,24},
{25,26,27,28,29,30,31,32},
{33,34,35,36,37,38,39,40},
{41,42,43,44,45,46,47,48},
{3 , 2, 1, 4, 5, 0, 0, 0}   //buttons for octave and channel increment and decrement
};
//char key list for white keys to get the channel number:
const char keyChannelMap[17]={17,19,21,23,24,
                              26,28,29,31,
                              33,35,36,38,
                              40,41,43,45};
                           
const byte rowPins[ROWS] = {10,11,12,A0,A1}; //connect to the row pinouts of the kpd
const byte colPins[COLS] = {2,3,4,5,6,7,8,9}; //connect to the column pinouts of the kpd
byte octShift=0;
byte channelread=1;
byte octval[17]={0};
bool channelselect=false;
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
int octCalib=863;
bool calibNotSaved=false;
byte btnList[LIST_MAX+1]={0};
byte newPressPtr=0;
MIDI_CREATE_DEFAULT_INSTANCE();
void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);              // Launch MIDI with default option
 Serial.begin(115200);
MIDI.setHandleNoteOn(midiOn);
MIDI.setHandleNoteOff(midiOff);
 dac.begin(0x60);
 octCalib=getCalib();
 pinMode(gate,OUTPUT);
 digitalWrite(gate,LOW);
 display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
 updateScreen();
  }
void loop() {

  octShift=octval[channelread]*12;
  MIDI.read();
     if (kpd.getKeys())
    {for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
         {
              if(kpd.key[i].kchar>=17 and channelselect ==false){
                    
                  if(channelread>0){
                    switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                      case PRESSED:
                      MIDI.sendNoteOn(kpd.key[i].kchar+octShift,127,channelread);
                      break;
                      case RELEASED:
                      MIDI.sendNoteOff(kpd.key[i].kchar+octShift,0,channelread);
                      break;
                      }
                      }
                  if(channelread==0){
                    switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                      case PRESSED:
                      CV_Press(kpd.key[i].kchar,true);
                      break;
                      case RELEASED:
                      CV_Press(kpd.key[i].kchar,false);
                      break;
                      }
                      outputCV();
                  }
              }
              
              if(kpd.key[i].kchar>=17 and channelselect ==true and kpd.key[i].kstate==PRESSED)
              {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                byte channelval=channelread;
                      for(byte x=0;x<17;x++) //check button character corresponds to which channel
                      {
                        if(kpd.key[i].kchar== keyChannelMap[x]){ 
                        channelval=x;
                        break;
                        }
                      }
                        
                        if(channelval != channelread)
                        {channelread=channelval;
                          if(channelread==0)
                            octCalib=getCalib();
                        
                        updateScreen();}
                        //Serial.println(channelval);
                    
                         
                 }
              if(kpd.key[i].kchar==1 and kpd.key[i].kstate==PRESSED and channelselect==false){// increment octave
                if(channelread==0){
                  if(octval[channelread]<2)
                    {switchkeys(0);
                      octval[channelread]+=1;
                      octShift=octval[channelread]*12;
                      switchkeys(1);
                      updateScreen();}
                  }
                else{
                  if(octval[channelread]<6)
                    {switchkeys(0);
                      octval[channelread]+=1;
                      octShift=octval[channelread]*12;
                      switchkeys(1);
                      updateScreen();}
                  }
              }
              if(kpd.key[i].kchar==2 and kpd.key[i].kstate==PRESSED and channelselect==false){// decrement octave
                if(octval[channelread]>0)
                  {switchkeys(0);
                    octval[channelread]-=1;
                    octShift=octval[channelread]*12;
                    switchkeys(1);
                    updateScreen();}
              }
              if(kpd.key[i].kchar==3){// enable channel select
                
                switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case PRESSED:
                      if(channelread==0)
                      resetCV();
                      else
                      switchkeys(0);
                      
                      channelselect=true;
                      break;
                    case RELEASED:
                      channelselect=false;
                      break;
                      }

              }
              if(kpd.key[i].kchar==4 and kpd.key[i].kstate==PRESSED and channelselect==false and channelread==0){// decrement Caliberation
                if(octCalib>0)
                  {octCalib-=1;
                    outputCV();
                    calibNotSaved=true;
                    updateScreen();}
              }
              if(kpd.key[i].kchar==5 and kpd.key[i].kstate==PRESSED and channelselect==false and channelread==0){// increment Caliberation
                if(octCalib<4095)
                  {octCalib+=1;
                    outputCV();
                    calibNotSaved=true;
                    updateScreen();}
              }
              
              if(kpd.key[i].kstate==PRESSED and channelselect==true and kpd.key[i].kchar==5){// save caliberation to EEPROM
                    storeCalib();
                    calibNotSaved=false;
                    updateScreen();
              }
              if(kpd.key[i].kstate==PRESSED and channelselect==true and kpd.key[i].kchar==4){// undo caliberation to EEPROM
                    octCalib=getCalib();
                    calibNotSaved=false;
                    updateScreen();
              }
              
              
            }//key state change check end
          }//key list for loop end
   }//if key pressed check end
          

} //loop end

void switchkeys(bool noteOn){
  for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
      { if(kpd.key[i].kchar>=17){
            if(channelread!=0){
                if(kpd.key[i].kstate == PRESSED or kpd.key[i].kstate == HOLD){
                  if(noteOn)
                  MIDI.sendNoteOn(kpd.key[i].kchar+octShift,127,channelread);
                  else
                  MIDI.sendNoteOff(kpd.key[i].kchar+octShift,0,channelread);
                }
            }
            
        }
      }
}
void midiOn(byte chn,byte note,byte vel){
  CV_Press(note,true);
  outputCV();
  }
void midiOff(byte chn,byte note,byte vel){
   CV_Press(note,false);
  outputCV();
  }
void CV_Press(byte btn, bool pres){
  if(pres){
    for(byte i=0;i<newPressPtr;i++){
      if(btn==btnList[i])
        return;
       }
        
      btnList[newPressPtr]=btn;
      newPressPtr+=1;
    }
  else{
    for(byte i=0;i<newPressPtr;i++)
     {if(btn==btnList[i])
       {btnList[i]=0;
        newPressPtr-=1;
          for(byte j=i;j<newPressPtr;j++)
              { btnList[j]=btnList[j+1];
                btnList[j+1]=0;
                }
        break;
       }
     }
      
   }
}
void outputCV(){
  if(newPressPtr==0){
    digitalWrite(gate,LOW);
    }
    else{
      byte shift=octShift;
      if(channelread>0)
        shift=0;
  int value=btnList[newPressPtr-1]-17+shift;
  //Serial.println(value);
  value=constrain(value, 0, 55);
  value= map(value,0, 12, 0, octCalib);//map octave notes to 1 volt output 858 is the calibrated value to get 1 volt
      dac.setVoltage(value,false);
      digitalWrite(gate,HIGH);
      
    }
}
void resetCV(){
  digitalWrite(gate,LOW);
  for(byte i=0;i<newPressPtr;i++)
  btnList[i]=0;
  newPressPtr=0;
  }

void updateScreen(){
  display.clearDisplay();
 display.setTextSize(1);     // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(display.height()/4,4);
  display.print("Chn.");
  display.setCursor(SCREEN_HEIGHT/4+(SCREEN_WIDTH/3)+1,4);
  display.print("Oct.");
  display.setCursor(SCREEN_HEIGHT/4+(SCREEN_WIDTH*2/3)+1,4);
  display.print("Cal.");
  
 display.setTextSize(2);     // Draw 2X-scale text
  display.setCursor(0,14);
  char buffer[10];
  sprintf(buffer," %02d  %d",channelread,octval[channelread]);
  display.print(buffer);
  if(calibNotSaved)
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);//invert text
  display.setCursor((SCREEN_WIDTH*2/3)+5,14);
  display.print(octCalib);
  if(calibNotSaved)
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);//invert text back
  
  display.drawRoundRect(0, 0,42,SCREEN_HEIGHT,display.height()/4, SSD1306_WHITE);
  display.drawRoundRect((SCREEN_WIDTH*2/3)+1,0,42,SCREEN_HEIGHT,display.height()/4, SSD1306_WHITE);
  display.drawRoundRect((SCREEN_WIDTH/3)+1,0,42,SCREEN_HEIGHT,display.height()/4, SSD1306_WHITE);
  display.display();
  }
void storeCalib(){
byte val=octCalib & 0x00ff;
EEPROM.update(eepromAddr,val); //store LSB
val=octCalib>>8 & 0x00ff;
EEPROM.update(eepromAddr+1,val); //store MSB
  }
int getCalib(){
  int val= EEPROM.read(eepromAddr) + (EEPROM.read(eepromAddr+1)<<8);
  if(val<=0)
  return defaultCalib;
  return val;
  }

#include <odroid_go.h>                                       // M5STACK
#include <sys/time.h>
extern "C" void sp_init();
extern "C" void start_spectemu();

timeval shouldbetv;

extern "C" void print(char *s) {
 GO.lcd.println(s);
}
// set display window, 192x256 mid screen...
extern "C" void setwindow() {
   GO.lcd.setAddrWindow(32,24,287,215);
}

extern "C" void writepixel(int16_t *buffer) {
   SPI.writeBytes((byte *)buffer, 2);
}

extern "C" void writepixels(int16_t *buffer) {
   SPI.writeBytes((byte *)buffer, 512);
}

// draw a scanline
extern "C" void line(int y, uint16_t *buffer) {
   GO.lcd.pushRect(32,y,256,1,buffer);
}

// draw 1st third of a scanline
extern "C" void line1(int y, uint16_t *buffer) {
  
     GO.lcd.pushRect(32,y,85,1,buffer); // was 128...
}

// draw 2nd third of a scanline
extern "C" void line2(int y, uint16_t *buffer) {
    GO.lcd.pushRect(117,y,85,1,&buffer[85]);  // was 128....
}

// draw 3rd third of a scanline
extern "C" void line3(int y, uint16_t *buffer) {
    GO.lcd.pushRect(202,y,86,1,&buffer[170]);  // was 128....
}



extern "C" void update_border(int colour) {
 
   int i,c;
   // for some reason the darFastHLine colours are now RGB...
   int bcolour[8]={
     0b0000000000000000, // black
     0b0000000000011110, // blue
     0b1111000000000000, // red
     0b1111000000011110, // magenta
     0b0000011111000000, // green
     0b0000011111011110, // cyan
     0b1111011111000000, // yellow
     0b1111011111011110, // white
   };

   c=bcolour[colour];
   for (i=0; i<24; i++)  GO.lcd.drawFastHLine(0,i,320,c);
   for (i=24; i<216; i++) {
     GO.lcd.drawFastHLine(0,i,32,c);
     GO.lcd.drawFastHLine(288,i,32,c);
   }
   for (i=216; i<240; i++)  GO.lcd.drawFastHLine(0,i,320,c);
}

extern "C" void play_sample(byte data) {
  int i;

  i=(int)data; i-=128; i/=8; i+=128;
  data=(byte)i;
   
  GO.Speaker.write(data); // keep quiet for now....
}

extern "C"  void key_update() {GO.update();}
extern "C" int key_start() {return(GO.BtnStart.isPressed());}
extern "C" int key_a() {return(GO.BtnA.isPressed());}
extern "C" int key_left() {return(GO.JOY_X.isAxisPressed()==2);}
extern "C" int key_right() {return(GO.JOY_X.isAxisPressed()==1);}


void setup() {
  // put your setup code here, to run once:
  GO.begin();                   // M5STACK INITIALIZE
  GO.lcd.setBrightness(200);    // BRIGHTNESS = MAX 255
  GO.lcd.fillScreen(BLACK);     // CLEAR SCREEN
  //GO.Speaker.setVolume(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  sp_init();
  start_spectemu();
}

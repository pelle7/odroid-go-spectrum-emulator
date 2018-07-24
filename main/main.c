#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_spiffs.h"
#include "driver/rtc_io.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"


#include "../components/odroid/odroid_settings.h"
#include "../components/odroid/odroid_input.h"
#include "../components/odroid/odroid_display.h"
#include "../components/odroid/odroid_audio.h"
#include "../components/odroid/odroid_system.h"

#include "../components/spectrum/compr.c"
#include "../components/spectrum/interf.c"
#include "../components/spectrum/keynames.c"
#include "../components/spectrum/loadim.c"
#include "../components/spectrum/misc.c"
#include "../components/spectrum/rom_imag.c"
#include "../components/spectrum/snapshot.c"
#include "../components/spectrum/spconf.c"
#include "../components/spectrum/spect.c"
#include "../components/spectrum/spectkey.c"
#include "../components/spectrum/spkey.c"
#include "../components/spectrum/spmain.c"
#include "../components/spectrum/spperif.c"
#include "../components/spectrum/spscr.c"
#include "../components/spectrum/spsound.c"
#include "../components/spectrum/sptape.c"
#include "../components/spectrum/sptiming.c"
#include "../components/spectrum/stubs.c"
#include "../components/spectrum/tapefile.c"
#include "../components/spectrum/vgakey.c"
#include "../components/spectrum/vgascr.c"
#include "../components/spectrum/z80.c"
#include "../components/spectrum/z80_op1.c"
#include "../components/spectrum/z80_op2.c"
#include "../components/spectrum/z80_op3.c"
#include "../components/spectrum/z80_op4.c"
#include "../components/spectrum/z80_op5.c"
#include "../components/spectrum/z80_op6.c"
#include "../components/spectrum/z80optab.c"
#include "../components/spectrum/z80_step.c"

#include <string.h>
#include <dirent.h>

extern int debug_trace;

int keyboard=0; // on-screen keyboard active?
int b_up=6,b_down=5,b_left=4,b_right=7;
int b_a=9,b_b=8,b_select=39,b_start=29; // 8 button maps

char key[40][6]={
"1","2","3","4","5","6","7","8","9","0",
"q","w","e","r","t","y","u","i","o","p",
"a","s","d","f","g","h","j","k","l","enter",
"caps","z","x","c","v","b","n","m","symbl","space"
};

odroid_gamepad_state joystick;

odroid_battery_state battery_state;

#define AUDIO_SAMPLE_RATE (15600) // 624 samples/frame at 25fps

int BatteryPercent = 100;

uint16_t* menuFramebuffer = 0;

unsigned short *buffer;  // screen buffer.
char path[256]="/sd/roms/spectrum",file[256]="/sd/roms/spectrum/basic.sna";

//----------------------------------------------------------------
void print(short x, short y, char *s)
{
    int rompos,i,n,k,a,len,idx;
    char c;
    extern unsigned short  *buffer; // screen buffer, 16 bit pixels
    
    len=strlen(s);
    for (k=0; k<len; k++) {
      c=s[k];
      rompos=15616; // start of character set 
      rompos+=c*8-256;
      
      for (i=0; i<8; i++) {
        a=rom_imag[rompos++];
	for (n=7,idx=i*len*8+k*8+7; n>=0; n--, idx--) {
	  if (a%2==1) buffer[idx]=65535; else buffer[idx]=0;
	  a=a>>1;
	}
      }   
    }
    ili9341_write_frame_rectangle(x*8,y*8,len*8,8,buffer);
}

//----------------------------------------------------------------
void printx2(short x, short y, char *s)
{
    int rompos,i,n,k,a,len,idx;
    char c;
    extern unsigned short  *buffer; // screen buffer, 16 bit pixels
    
    len=strlen(s);
    for (k=0; k<len; k++) {
      c=s[k];
      rompos=15616; // start of character set 
      rompos+=c*8-256;
      
      for (i=0; i<8; i++) {
        a=rom_imag[rompos++];
	for (n=7,idx=i*len*32+k*16+14; n>=0; n--, idx-=2) {
	  if (a%2==1) {buffer[idx]=-1; buffer[idx+1]=-1; buffer[idx+len*16]=-1; buffer[idx+len*16+1]=-1; }
	  else {buffer[idx]=0; buffer[idx+1]=0; buffer[idx+len*16]=0; buffer[idx+len*16+1]=0;}
	  a=a>>1;
	}
      }   
    }
    ili9341_write_frame_rectangle(x*8,y*8,len*16,16,buffer);
}

//----------------------------------------------------------------
void debounce(int key)
{
  while (joystick.values[key]) 
         odroid_input_gamepad_read(&joystick);
}

//---------------------------------------------------------------

int choose_file() 
{
  FILE *fp;
  struct dirent *de; 
  int y=0,count,redraw=0,len;
  DIR *dr;
  
  while(1) { // stay in file chooser until something loaded or menu button pressed
    redraw=0; count=0;
    ili9341_clear(0); // clear screen
    printx2(5,0,"Load Snapshot");
    print(0,3,path);
    dr = opendir(path);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory
      {
        printf("Could not open current directory" );
        return 0;
      }
      count=0;
      while ((de = readdir(dr)) != NULL) {
            if (count/20==y/20) {
	      if (strlen(de->d_name)>35) de->d_name[35]=0;
	      print(1,(count%20)+5, de->d_name);
	      if (de->d_type==DT_DIR) print(1+strlen(de->d_name),(count%20)+5,"/");
	    }
	    count++;
      } 
      closedir(dr);
      print(0,(y%20)+5,">");
      
      // process key presses...
      while(!joystick.values[ODROID_INPUT_A] && 
           !joystick.values[ODROID_INPUT_B] &&
           !redraw)
      {
        odroid_input_gamepad_read(&joystick);
	if (joystick.values[ODROID_INPUT_UP] && y>0) {
	  print(0,(y%20)+5," "); y--;
	  print (0,(y%20)+5,">");
	  debounce(ODROID_INPUT_UP);
	}
	if (joystick.values[ODROID_INPUT_DOWN] && y<count-1) {
	  print(0,(y%20)+5," "); y++;
	  print (0,(y%20)+5,">");
	  debounce(ODROID_INPUT_DOWN);
	  if (y%20==0) redraw=1;
	}
	if (joystick.values[ODROID_INPUT_LEFT] && y>19) {
	  debounce(ODROID_INPUT_LEFT); y-=20; redraw=1;
	}
	if (joystick.values[ODROID_INPUT_RIGHT] && y<count-21) {	
	  debounce(ODROID_INPUT_RIGHT); y+=20; redraw=1;
	}			
        if (joystick.values[ODROID_INPUT_MENU]) {  // just exit menu...
          debounce(ODROID_INPUT_MENU);
           return(0);
        }
      }
      // OK, either 'A' pressed or screen re-draw needed....
      if (joystick.values[ODROID_INPUT_A] && count==0) // empty directory
            debounce(ODROID_INPUT_A); else 
      if (joystick.values[ODROID_INPUT_A]) {
        debounce(ODROID_INPUT_A);
	dr = opendir(path);
	de = readdir(dr);
	for (count=0; count<y; count++) de = readdir(dr);
	
	if (de->d_type == DT_DIR) { // go into directory...
	  len=strlen(path); path[len]='/';
	  strcat(&path[len+1],de->d_name);
	  y=0; // go to first option
	}
	if (de->d_type == DT_REG) { // file....	
	  file[0]=0; strcat(file,path);
	  strcat(file,"/");
	  strcat(file,de->d_name);
	  load_snapshot_file_type(file,-1);
	  if ((fp=fopen("/sd/roms/spectrum/resume.txt","w"))) {
	    fprintf(fp,"%s\n",path); fprintf(fp,"%s\n",file);
	    fprintf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
	     b_up,b_down,b_left,b_right,b_a,b_b,b_select,b_start);	    
	    fclose(fp);
	  }
	  ili9341_clear(0); // clear screen
	  return(0);	  
        }
      }
      if (joystick.values[ODROID_INPUT_B]) { // up a directory?
        debounce(ODROID_INPUT_B);
	y=0;  // back to first item in list
        if (strlen(path)>3) {  // safe to go up a directory...
          while(path[strlen(path)-1]!='/') path[strlen(path)-1]=0;
 	  path[strlen(path)-1]=0;
        } 
      }
  }
}

//----------------------------------------------------------------
void save()
{
  printx2(12,22,"SAVING");
  save_snapshot_file(file);
  printx2(12,22,"SAVED."); sleep(1);
}

//----------------------------------------------------------------
void draw_keyboard()
{
   print(0,24,"      1  2  3  4  5  6  7  8  9  0   ");
   print(0,25,"      q  w  e  r  t  y  u  i  o  p   ");
   print(0,26,"      a  s  d  f  g  h  j  k  l ent  ");
   print(0,27,"     cap z  x  c  v  b  n  m sym _   ");
   print(0,28,"                                     ");
   kb_set(); // add cursor
}
//---------------------------------------------------------------
void kb_blank()
{
  if (kbpos==29 || kbpos==30 || kbpos==38) {
    print(4+(kbpos%10)*3,24+(kbpos/10)," ");
    print(8+(kbpos%10)*3,24+(kbpos/10)," ");
  } else {
    print(5+(kbpos%10)*3,24+(kbpos/10)," ");
    print(7+(kbpos%10)*3,24+(kbpos/10)," ");
  } 
}

void kb_set() 
{
   if (kbpos==29 || kbpos==30 || kbpos==38) {
    print(4+(kbpos%10)*3,24+(kbpos/10),">");
    print(8+(kbpos%10)*3,24+(kbpos/10),"<");
  } else {
    print(5+(kbpos%10)*3,24+(kbpos/10),">");
    print(7+(kbpos%10)*3,24+(kbpos/10),"<");
  } 
}

int get_key(int current)
{
  draw_keyboard();
  while(1) {
    odroid_input_gamepad_read(&joystick);
    
    if (joystick.values[ODROID_INPUT_UP]) {
      kb_blank(); kbpos-=10; if (kbpos<0) kbpos+=40;
      kb_set();
      debounce(ODROID_INPUT_UP);   
    }
    if (joystick.values[ODROID_INPUT_DOWN]) {
      kb_blank(); kbpos+=10; if (kbpos>39) kbpos-=40; 
      kb_set();
      debounce(ODROID_INPUT_DOWN);     
    }
    
    if (joystick.values[ODROID_INPUT_LEFT]) {
      kb_blank(); kbpos--; if (kbpos%10==9 || kbpos==-1) kbpos+=10;
      kb_set();
      debounce(ODROID_INPUT_LEFT);   
    }
    if (joystick.values[ODROID_INPUT_RIGHT]) {
      kb_blank(); kbpos++; if (kbpos%10==0) kbpos-=10;
      kb_set();
      debounce(ODROID_INPUT_RIGHT);   
    }
    if (joystick.values[ODROID_INPUT_A]) {     
      debounce(ODROID_INPUT_A);
      return(kbpos);
    }
    if (joystick.values[ODROID_INPUT_B]) {
     debounce(ODROID_INPUT_B);
     return(current);
    }
    if (joystick.values[ODROID_INPUT_MENU]) {
     debounce(ODROID_INPUT_MENU);
     return(current);
    }
  }
 
}
//-----------------------------------------------------------------
// screen for choosing joystick setups like Cursor or Sinclair.
int load_set()
{
  int posn=3;
  
  ili9341_clear(0); // clear screen
  printx2(0,0,"Choose Button Set");
  printx2(4,3,"Cursor/Protek");
  printx2(4,6,"q,a,o,p,m");
  printx2(4,9,"Sinclair");
  printx2(4,12,"Go Back");
  printx2(0,posn,">");
  while(1) {
      odroid_input_gamepad_read(&joystick);
      if (joystick.values[ODROID_INPUT_DOWN]) {
        printx2(0,posn," "); posn+=3; if (posn>12) posn=3;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_DOWN);     
      }
      if (joystick.values[ODROID_INPUT_UP]) {
        printx2(0,posn," "); posn-=3; if (posn<3) posn=12;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_UP);   
      }
      if (joystick.values[ODROID_INPUT_A]) {     
        debounce(ODROID_INPUT_A);
        if (posn==3 ) {b_up=6; b_down=5;b_left=4;b_right=7; b_a=9; return(0);}
        if (posn==6 ) {b_up=10; b_down=20;b_left=18;b_right=19; b_a=37; return(0);}
        if (posn==9 ) {b_up=8; b_down=7;b_left=5;b_right=6; b_a=9; return(0);}	
        if (posn==12) return(0);  
      }
      if (joystick.values[ODROID_INPUT_MENU]) {
       debounce(ODROID_INPUT_MENU);
       return(0);
      }
  }     
}

//-----------------------------------------------------------------
int setup_buttons()
{
  int posn=0,redraw=0;
  
  while(1) {
    ili9341_clear(0); // clear screen
   
    printx2(4, 0,"Up     = "); printx2(22,0,key[b_up]);
    printx2(4, 3,"Down   = "); printx2(22,3,key[b_down]);
    printx2(4, 6,"Left   = "); printx2(22,6,key[b_left]);
    printx2(4, 9,"Right  = "); printx2(22,9,key[b_right]);
    printx2(4,12,"A      = "); printx2(22,12,key[b_a]);
    printx2(4,15,"B      = "); printx2(22,15,key[b_b]);    
    printx2(4,18,"Select = "); printx2(22,18,key[b_select]);
    printx2(4,21,"Start  = "); printx2(22,21,key[b_start]);
    printx2(4,24,"Choose a Set");
    printx2(4,27,"Done.");
    printx2(0,posn,">");
    redraw=0;
    while(!redraw) {
      odroid_input_gamepad_read(&joystick);
      if (joystick.values[ODROID_INPUT_DOWN]) {
        printx2(0,posn," "); posn+=3;  if (posn>27) posn=0;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_DOWN);     
      }
      if (joystick.values[ODROID_INPUT_UP]) {
        printx2(0,posn," "); posn-=3;  if (posn<0) posn=27;
        printx2(0,posn,">");
        debounce(ODROID_INPUT_UP);   
      }
      if (joystick.values[ODROID_INPUT_A]) {     
        debounce(ODROID_INPUT_A);
        if (posn==0 ) {b_up=get_key(b_up); redraw=1;}
        if (posn==3 ) {b_down=get_key(b_down); redraw=1;}
        if (posn==6 ) {b_left=get_key(b_left); redraw=1;}
        if (posn==9 ) {b_right=get_key(b_right); redraw=1;}
	if (posn==12 ) {b_a=get_key(b_a); redraw=1;}
	if (posn==15 ) {b_b=get_key(b_b); redraw=1;}
	if (posn==18) {b_select=get_key(b_select); redraw=1;}
	if (posn==21) {b_start=get_key(b_start); redraw=1;}
	if (posn==24) {load_set(); redraw=1;}
        if (posn==27) return(0);  
      }
      if (joystick.values[ODROID_INPUT_MENU]) {
       debounce(ODROID_INPUT_MENU);
       return(0);
      }
    }    
  }
}

//----------------------------------------------------------------
int menu()
{
  int posn;
  odroid_volume_level level;
  FILE *fp;

  level=odroid_audio_volume_get();
  odroid_audio_volume_set(0); // turn off sound when in menu...
  process_sound();
  
  lastborder=100; // force border re-draw after menu exit....
  
  //debounce inital menu button press first....
  odroid_input_gamepad_read(&joystick);
  debounce(ODROID_INPUT_MENU);
  keyboard=0; // make sure virtual keyboard swtiched off now
  ili9341_clear(0); // clear screen
  printx2(0,0,"ZX Spectrum Emulator");
  printx2(4,6,"Keyboard");
  printx2(4,9,"Load Snapshot");
  printx2(4,12,"Save Snapshot");
  printx2(4,15,"Setup Buttons");
  printx2(4,18,"Back To Emulator");
  printx2(4,21,"Exit Emulator");
  
  posn=6;
  printx2(0,posn,">");
  while(1) {
    odroid_input_gamepad_read(&joystick);
    if (joystick.values[ODROID_INPUT_DOWN]) {
      printx2(0,posn," "); posn+=3; if (posn>21) posn=6;
      printx2(0,posn,">");
      debounce(ODROID_INPUT_DOWN);     
    }
    if (joystick.values[ODROID_INPUT_UP]) {
      printx2(0,posn," "); posn-=3; if (posn<6) posn=21;
      printx2(0,posn,">");
      debounce(ODROID_INPUT_UP);   
    }
    if (joystick.values[ODROID_INPUT_A]) {     
      debounce(ODROID_INPUT_A);
      if (posn==6) {keyboard=1; draw_keyboard();}
      if (posn==9) choose_file(); 
      if (posn==12) save();
      if (posn==15) { // set up buttons, save back new settings
         setup_buttons();
	 if ((fp=fopen("/sd/roms/spectrum/resume.txt","w"))) {
	    fprintf(fp,"%s\n",path); fprintf(fp,"%s\n",file);
	    fprintf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
	     b_up,b_down,b_left,b_right,b_a,b_b,b_select,b_start);	    
	    fclose(fp);
	  }
      }
      if (posn==21) {
        odroid_system_application_set(0); // set menu slot 
	esp_restart(); // reboot!
      }
      
      odroid_audio_volume_set(level);  // restore sound...
      return(0);
    }
    if (joystick.values[ODROID_INPUT_MENU]) {
     debounce(ODROID_INPUT_MENU);
     odroid_audio_volume_set(level); // restore sound...
     return(0);
    }
  }
}

//----------------------------------------------------------------
void app_main(void)
{
    FILE *fp;
    
    printf("odroid start.\n");
  
    nvs_flash_init();
    odroid_system_init();
    odroid_input_gamepad_init();

    // Boot state overrides
    bool forceConsoleReset = false;

    switch (esp_sleep_get_wakeup_cause())
    {
        case ESP_SLEEP_WAKEUP_EXT0:
        {
            printf("app_main: ESP_SLEEP_WAKEUP_EXT0 deep sleep wake\n");
            break;
        }

        case ESP_SLEEP_WAKEUP_EXT1:
        case ESP_SLEEP_WAKEUP_TIMER:
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
        case ESP_SLEEP_WAKEUP_ULP:
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        {
            printf("app_main: Non deep sleep startup\n");

            odroid_gamepad_state bootState = odroid_input_read_raw();

            if (bootState.values[ODROID_INPUT_MENU])
            {
                // Force return to factory app to recover from
                // ROM loading crashes

                // Set factory app
                const esp_partition_t* partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
                if (partition == NULL)
                {
                    abort();
                }

                esp_err_t err = esp_ota_set_boot_partition(partition);
                if (err != ESP_OK)
                {
                    abort();
                }

                // Reset
                esp_restart();
            }

            if (bootState.values[ODROID_INPUT_START])
            {
                // Reset emulator if button held at startup to
                // override save state
                forceConsoleReset = true;
            }

            break;
        }
        default:
            printf("app_main: Not a deep sleep reset\n");
            break;
    }  
   
    // allocate a screen buffer...
    buffer=(unsigned short *)malloc(16384);
   
    // Display
    ili9341_prepare();
    ili9341_init();
    
    odroid_sdcard_open("/sd");   // map SD card.
    // see if there is a 'resume.txt' file, use it if so...
    
    if ((fp=fopen("/sd/roms/spectrum/resume.txt","r"))) {
      fgets(path,sizeof path,fp); path[strlen(path)-1]=0;
      fgets(file, sizeof file,fp); file[strlen(file)-1]=0; 
      fscanf(fp,"%i,%i,%i,%i,%i,%i,%i,%i\n",
	     &b_up,&b_down,&b_left,&b_right,&b_a,&b_b,&b_select,&b_start);
      fclose(fp);
      printf("resume=%s\n",file);
    }

    // Audio hardware
    odroid_audio_init(AUDIO_SAMPLE_RATE);
  
    // Start Spectrum emulation here.......
    sp_init();
    load_snapshot_file_type(file,-1);
    start_spectemu();
}

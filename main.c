#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

#include "usb_descriptors.h"
#include <stdio.h>

#include "pico/stdlib.h"

#include "hardware/gpio.h"

#include "hardware/i2c.h"
#include "pico/binary_info.h"

#include "rtc.h"
#include "f_util.h"
#include "ff.h"
#include "hw_config.h"

#include "Skylander.h"

#include "util.h"
//////////MENU
int sd_skylander_count = 0;
int selected_skylander = -1; // If this is -1, do not read a skylander
//////////ENDOFMENU
//////////PORTAL
#define MSG_SIZE 32
#define BLOCK_SIZE 16
#define MAX_SKYLANDER_COUNT 4
FIL *loaded_skylanders[MAX_SKYLANDER_COUNT] = {0};
bool shutthefuckup = false;
bool send_data_now = false;
char sense_counter = 0;
//////////END OF PORTAL
//////////BUTTON STUFF
#define BUTTON_SELECT 21
#define BUTTON_RIGHT 20
#define BUTTON_LEFT 19
#define BUTTON_SLOT_RIGHT 15
#define BUTTON_SLOT_LEFT 14
#define BUTTON_START 18
//////////END BUTTON
//////////LED STUFF
#define LED1 6
#define LED2 7
#define LED3 8
#define LED4 9
//////////END OF LED
//////////LCD STUFF
// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const int LCD_ENTRYSHIFTINCREMENT = 0x01;
const int LCD_ENTRYLEFT = 0x02;

// flags for display and cursor control
const int LCD_BLINKON = 0x01;
const int LCD_CURSORON = 0x02;
const int LCD_DISPLAYON = 0x04;

// flags for display and cursor shift
const int LCD_MOVERIGHT = 0x04;
const int LCD_DISPLAYMOVE = 0x08;

// flags for function set
const int LCD_5x10DOTS = 0x04;
const int LCD_2LINE = 0x08;
const int LCD_8BITMODE = 0x10;

// flag for backlight control
const int LCD_BACKLIGHT = 0x08;

const int LCD_ENABLE_BIT = 0x04;

// By default these LCD display drivers are on bus address 0x27
static int addr = 0x27;

// Modes for lcd_send_byte
#define LCD_CHARACTER 1
#define LCD_COMMAND 0

#define MAX_LINES 2
#define MAX_CHARS 16
void SDSkylanders();
int SkylanderEndsWith(char *, char *);

/* Quick helper function for single byte transfers */
void i2c_write_byte(uint8_t val)
{
#ifdef i2c_default
  i2c_write_blocking(i2c_default, addr, &val, 1, false);
#endif
}

void lcd_toggle_enable(uint8_t val)
{
  // Toggle enable pin on LCD display
  // We cannot do this too quickly or things don't work
#define DELAY_US 600
  sleep_us(DELAY_US);
  i2c_write_byte(val | LCD_ENABLE_BIT);
  sleep_us(DELAY_US);
  i2c_write_byte(val & ~LCD_ENABLE_BIT);
  sleep_us(DELAY_US);
}

// The display is sent a byte as two separate nibble transfers
void lcd_send_byte(uint8_t val, int mode)
{
  uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
  uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

  i2c_write_byte(high);
  lcd_toggle_enable(high);
  i2c_write_byte(low);
  lcd_toggle_enable(low);
}

void lcd_clear(void)
{
  lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

// go to location on LCD
void lcd_set_cursor(int line, int position)
{
  int val = (line == 0) ? 0x80 + position : 0xC0 + position;
  lcd_send_byte(val, LCD_COMMAND);
}

static void inline lcd_char(char val)
{
  lcd_send_byte(val, LCD_CHARACTER);
}

void lcd_string(const char *s)
{
  while (*s)
  {
    lcd_char(*s++);
  }
}

void lcd_init()
{
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x03, LCD_COMMAND);
  lcd_send_byte(0x02, LCD_COMMAND);

  lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
  lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
  lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
  lcd_clear();
}
/////////END OF LCD STUFF

void hid_task(void);

size_t f_listfiles(char *names[], size_t max)
{
  FRESULT res;
  DIR dir;
  FILINFO fno;
  bool skylander = false;
  size_t count = -1;
  f_opendir(&dir, "/"); // Open Root
  do
  {
    skylander = false;
    f_readdir(&dir, &fno);
    if (fno.fname[0] != 0)
    {

      if (SkylanderEndsWith(fno.fname, "bin") == 1 || SkylanderEndsWith(fno.fname, "sky") == 1 || SkylanderEndsWith(fno.fname, "dmp") == 1 || SkylanderEndsWith(fno.fname, "dump") == 1)
      {
        printf("String to alloc: %s | String length = %d\n", fno.fname, strlen(fno.fname) + 1);
        sleep_ms(100);
        count++;
        printf("Writing to FileNames[%d]\n", count);
        names[count] = malloc(strlen(fno.fname) + 1);
        sleep_ms(100);
        strcpy(names[count], fno.fname);
        sleep_ms(100);
      }
    }
  } while (fno.fname[0] != 0);

  f_closedir(&dir);
  return count;
}

int main()
{
  bool start_emu = false;
  // INIT SERIAL DEBUG
  printf("new");
  stdio_init_all();
  printf("INIT STDIO DONE\n");
  sleep_ms(200);
  printf("-------------------------------------\n");
  printf(" KAOS - a Raspbery Pi Pico Skylander \n");
  printf("      Portal of Power Emulator       \n");
  printf("-------------------------------------\n");
  printf("    Made by NicoAICP and redcubie    \n");
  printf("-------------------------------------\n");
  sleep_ms(400);
  // INIT GPIO for buttons
  gpio_init(BUTTON_SELECT);
  gpio_set_dir(BUTTON_SELECT, GPIO_IN);
  gpio_pull_up(BUTTON_SELECT);

  gpio_init(BUTTON_LEFT);
  gpio_set_dir(BUTTON_LEFT, GPIO_IN);
  gpio_pull_up(BUTTON_LEFT);

  gpio_init(BUTTON_RIGHT);
  gpio_set_dir(BUTTON_RIGHT, GPIO_IN);
  gpio_pull_up(BUTTON_RIGHT);

  gpio_init(BUTTON_START);
  gpio_set_dir(BUTTON_START, GPIO_IN);
  gpio_pull_up(BUTTON_START);

  gpio_init(BUTTON_SLOT_LEFT);
  gpio_set_dir(BUTTON_SLOT_LEFT, GPIO_IN);
  gpio_pull_up(BUTTON_SLOT_LEFT);

  gpio_init(BUTTON_SLOT_RIGHT);
  gpio_set_dir(BUTTON_SLOT_RIGHT, GPIO_IN);
  gpio_pull_up(BUTTON_SLOT_RIGHT);

  // INIT i2c FOR LCD
  i2c_init(i2c_default, 100 * 1000);
  gpio_set_function(4, GPIO_FUNC_I2C);
  gpio_set_function(5, GPIO_FUNC_I2C);
  gpio_pull_up(4);
  gpio_pull_up(5);
  // Make the I2C pins available to picotool
  bi_decl(bi_2pins_with_func(4, 5, GPIO_FUNC_I2C));
  lcd_init();
  printf("LCD INIT done\n");
  sleep_ms(200);

  // SEND MESSAGE TO LCD
  lcd_clear();
  lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("    Starting    ") / 2);
  lcd_string("    Starting    ");
  lcd_set_cursor(1, (MAX_CHARS / 2) - strlen(" Initialization ") / 2);
  lcd_string(" Initialization ");
  sleep_ms(500);

  // SDCARD INIT
  sd_card_t *pSD = sd_get_by_num(0);
  FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
  if (FR_OK != fr)
  {
    printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    sleep_ms(200);

    lcd_clear();
    lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("  Cannot Mount  ") / 2);
    lcd_string("  Cannot Mount  ");
    lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("     SDCard     ") / 2);
    lcd_string("     SDCard     ");

    while (true)
      ;
  }
  SDSkylanders(); // Gets how many skylanders we have

  char *skyFiles[sd_skylander_count];
  size_t count = f_listfiles(skyFiles, 255);

  // INIT TINYUSB
  board_init();
  // tusb_init();
  tud_init(0);
  printf("INIT TinyUSB done\n");
  sleep_ms(200);
  lcd_clear();
  lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("    Finished    ") / 2);
  lcd_string("    Finished    ");
  lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("  Initializing  ") / 2);
  lcd_string("  Initializing  ");
  sleep_ms(500);

  lcd_clear();
  lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("Skylander Portal") / 2);
  lcd_string("Skylander Portal");
  lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("    Emulator    ") / 2);
  lcd_string("    Emulator    ");
  sleep_ms(500);

  char *nbuffer;
  nbuffer = calloc(MSG_SIZE, 1);
  nbuffer[0] = 0x53;
  nbuffer[6] = 0x01;
  bool send_data = false;
  while (true)
  {
    if (!gpio_get(BUTTON_SELECT))
    {
      if (selected_skylander == -1)
      {
        lcd_set_cursor(0, (MAX_CHARS / 2) - strlen(" Please  select ") / 2);
        lcd_string(" Please  select ");
        lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("  a  Skylander  ") / 2);
        lcd_string("  a  Skylander  ");
        sleep_ms(500);
      }
      else
      {
        // TODO: Check if slot already has a file, if yes remove it

        FIL *newfile = calloc(1, sizeof(FIL));
        FRESULT fr = f_open(newfile, skyFiles[selected_skylander], FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (fr != FR_OK && fr != FR_EXIST)
        {
          printf("f_open(%s) error (Probably because file is already loaded): %s (%d)\n", skyFiles[selected_skylander], FRESULT_str(fr), fr);
          lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("  File already  ") / 2);
          lcd_string("  File already  ");
          lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("     loaded     ") / 2);
          lcd_string("     loaded     ");
          sleep_ms(500);
        }
        else
        {
          if (fd_in_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT) == 0)
          {
            add_fd_to_array(newfile, loaded_skylanders, MAX_SKYLANDER_COUNT);

            // Read skylander data
            // char *databuffer;
            // uint actual_len = 0;
            // databuffer = calloc(1024, 1);

            // f_lseek(newfile, 0);
            // f_read(newfile, midbuffer, BLOCK_SIZE, &actual_len);

            // if (actual_len != BLOCK_SIZE)
            // {
            //  printf("Read data length is %i not %i", actual_len, BLOCK_SIZE);
            //   return;
            // }

            // char *skyName = calloc(16, sizeof(char));
            // GetName()
            // IF Skylander nickname is null
            printf("File %s loaded", skyFiles[selected_skylander]);
            // ELSE
            // printf("File %s loaded", skyName);
            lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("  File  loaded  ") / 2);
            lcd_string("  File  loaded  ");
            sleep_ms(100);
            // Needs to be called for skylander to be read
            nbuffer = calloc(MSG_SIZE, 1);
            nbuffer[0] = 0x53;
            nbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
            nbuffer[5] = sense_counter++;
            nbuffer[6] = 0x01;
            tud_hid_report(0, nbuffer, MSG_SIZE);
          }
          else
          {
            lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("  File already  ") / 2);
            lcd_string("  File already  ");
            lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("     loaded     ") / 2);
            lcd_string("     loaded     ");
            sleep_ms(500);
          }
        }
      }
      lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("Skylander Portal") / 2);
      lcd_string("Skylander Portal");
      lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("    Emulator    ") / 2);
      lcd_string("    Emulator    ");
      sleep_ms(500);
    }

    if (!gpio_get(BUTTON_LEFT))
    {
      if (selected_skylander == -1)
      {
        selected_skylander = 0;
      }
      else if (selected_skylander > 0)
      {
        selected_skylander--;
      }

      printf("Selected Skylander #%d\n", selected_skylander);
      sleep_ms(100);
      lcd_clear();
      lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("  File already  ") / 2);
      lcd_string(" Selected  File ");
      lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("     loaded     ") / 2);
      lcd_string(skyFiles[selected_skylander]);
      sleep_ms(500);
      lcd_clear();
      lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("Skylander Portal") / 2);
      lcd_string("Skylander Portal");
      lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("    Emulator    ") / 2);
      lcd_string("    Emulator    ");
      sleep_ms(500);
    }

    if (!gpio_get(BUTTON_RIGHT))
    {
      if (selected_skylander < sd_skylander_count - 1)
      {
        selected_skylander++;
      }
      printf("Selected Skylander #%d\n", selected_skylander);
      sleep_ms(100);
      lcd_clear();
      lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("  File already  ") / 2);
      lcd_string(" Selected  File ");
      lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("     loaded     ") / 2);
      lcd_string(skyFiles[selected_skylander]);
      sleep_ms(500);
      lcd_clear();
      lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("Skylander Portal") / 2);
      lcd_string("Skylander Portal");
      lcd_set_cursor(1, (MAX_CHARS / 2) - strlen("    Emulator    ") / 2);
      lcd_string("    Emulator    ");
      sleep_ms(500);
    }

    if (!gpio_get(BUTTON_START))
    {
      nbuffer = calloc(MSG_SIZE, 1);
      nbuffer[0] = 0x53;
      nbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
      nbuffer[5] = sense_counter++;
      nbuffer[6] = 0x01;
      tud_hid_report(0, nbuffer, MSG_SIZE);
      free(nbuffer);
      sleep_ms(20);
    }
    tud_task();

    // tinyusb device task
    // sleep_ms(1000);
  }
  return 0;
}

void SDSkylanders()
{
  FRESULT res;
  DIR dir;
  FILINFO fno;
  bool skylander = false;
  f_opendir(&dir, "/"); // Open Root
  do
  {
    skylander = false;
    f_readdir(&dir, &fno);
    if (fno.fname[0] != 0)
    {

      if (SkylanderEndsWith(fno.fname, "bin") == 1 || SkylanderEndsWith(fno.fname, "sky") == 1 || SkylanderEndsWith(fno.fname, "dmp") == 1 || SkylanderEndsWith(fno.fname, "dump") == 1)
      {
        printf("Skylander found: %s\n", fno.fname);
        sleep_ms(100);
        // skylanderFileNames[sd_skylander_count] = fno.fname;
        sd_skylander_count++;
      }
      else
      {
        printf("File is not a Skylander: %s\n", fno.fname);
        sleep_ms(100);
      }
    }

  } while (fno.fname[0] != 0);
  printf("Found %d Skylanders\n", sd_skylander_count);
  sleep_ms(100);
  f_closedir(&dir);
}

int SkylanderEndsWith(char *str, char *suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return (str_len >= suffix_len) &&
         (0 == strcmp(str + (str_len - suffix_len), suffix));
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint8_t len)
{
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  printf("GET %X", report_type);
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) // Console/ Computer sends data (Query, Shutdown etc.)
{
  printf("%X", report_type);
  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // char paddedbuffer[MSG_SIZE] = {0};
    char *outbuffer;
    char *midbuffer;
    FIL *curfile;
    uint actual_len = 0;

    switch (buffer[0]) // Commands we can recieve from the host
    {
    case 'R': // 0x52 Reboot/Shutdown Portal
      printf("Recieved reboot\n");
      shutthefuckup = true;
      outbuffer = calloc(MSG_SIZE, 1);
      /* outbuffer[0] = 0x52;
       outbuffer[1] = 0x02;
       outbuffer[2] = 0x0a;
       outbuffer[3] = 0x05;
       outbuffer[4] = 0x08;*/
      outbuffer[0] = 0x52;
      outbuffer[1] = 0x02;
      outbuffer[2] = 0x1b;
      tud_hid_report(0, outbuffer, MSG_SIZE);

      // free(outbuffer);
      break;
    case 'J':
      printf("I do not know what it does but something related to sound");
      outbuffer = calloc(MSG_SIZE, 1);
      outbuffer[0] = buffer[0];
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;
    case 'M':
      printf("Activate / Deactivate speaker");
      outbuffer = calloc(MSG_SIZE, 1);
      outbuffer[0] = buffer[0]; // We only send the command back without any return value, since we do not have a speaker
      tud_hid_report(0, outbuffer, MSG_SIZE);
      break;
    case 'A': // 0x41 Activate Portal

      printf("Recieved activate\n");
      outbuffer = calloc(MSG_SIZE, 1);
      outbuffer[0] = 0x41;
      outbuffer[1] = buffer[1];
      outbuffer[2] = 0xff;
      outbuffer[3] = 0x77;
      tud_hid_report(0, outbuffer, MSG_SIZE);
      if (buffer[1] == 0x01)
        shutthefuckup = false;
      // free(outbuffer);
      break;
    case 'S': // 0x53 Sense how many Skylanders are on the Portal
      printf("Recieved sense\n");
      outbuffer = calloc(MSG_SIZE, 1);
      outbuffer[0] = 0x53;
      outbuffer[1] = create_sense_bitmask(loaded_skylanders, MAX_SKYLANDER_COUNT);
      // pseudo: outbuffer[2:5] = {0x00} (len = 3)
      outbuffer[5] = sense_counter++;
      outbuffer[6] = 0x01;
      tud_hid_report(0, outbuffer, MSG_SIZE);
      // free(outbuffer);
      break;
    case 'Q': // 0x51 Read Blocks (16 Bytes) From Skylander
      printf("Recieved query\n");
      outbuffer = calloc(MSG_SIZE, 1);
      outbuffer[0] = 0x51;
      outbuffer[1] = 0x10;
      if (buffer[1] == 0x11 || buffer[1] == 0x21)
      {
        outbuffer[1] = 0x11;
      }
      outbuffer[2] = buffer[2];

      midbuffer = calloc(BLOCK_SIZE, 1);
      // if buffer[1] == 0x20 loaded_skylanders[0] else [1]
      curfile = loaded_skylanders[0]; // read skylander #1
      if (buffer[1] == 0x21 || buffer[1] == 0x11)
      {
        curfile = loaded_skylanders[1]; // read skylander #2
      }
      f_lseek(curfile, buffer[2] * BLOCK_SIZE);
      f_read(curfile, midbuffer, BLOCK_SIZE, &actual_len);

      if (actual_len != BLOCK_SIZE)
      {
        printf("Read data length is %i not %i", actual_len, BLOCK_SIZE);
        return;
      }

      memcpy(outbuffer + 3, midbuffer, BLOCK_SIZE);
      printf("midbuffer: %16x", *midbuffer);
      free(midbuffer);
      tud_hid_report(0, outbuffer, MSG_SIZE);
      // free(outbuffer);
      break;
    case 'W': // 0x57 Write Blocks (16 Bytes) To Skylander
      printf("Recieved write\n");
      // Add writing to file
      midbuffer = calloc(BLOCK_SIZE, 1);

      // if buffer[1] == 0x20 loaded_skylanders[0] else [1]

      curfile = loaded_skylanders[0]; // read skylander #1
      if (buffer[1] == 0x21 || buffer[1] == 0x11)
      {
        curfile = loaded_skylanders[1]; // read skylander #2
      }
      f_lseek(curfile, buffer[2] * BLOCK_SIZE);

      // FILL MIDBUFFER
      midbuffer[0] = buffer[3];
      midbuffer[1] = buffer[4];
      midbuffer[2] = buffer[5];
      midbuffer[3] = buffer[6];
      midbuffer[4] = buffer[7];
      midbuffer[5] = buffer[8];
      midbuffer[6] = buffer[9];
      midbuffer[7] = buffer[10];
      midbuffer[8] = buffer[11];
      midbuffer[9] = buffer[12];
      midbuffer[10] = buffer[13];
      midbuffer[11] = buffer[14];
      midbuffer[12] = buffer[15];
      midbuffer[13] = buffer[16];
      midbuffer[14] = buffer[17];
      midbuffer[15] = buffer[18];

      f_write(curfile, midbuffer, BLOCK_SIZE, &actual_len);

      if (actual_len != BLOCK_SIZE)
      {
        // lcd_set_cursor(0, (MAX_CHARS / 2) - strlen("    Starting    ") / 2);
        // lcd_string(" Error  writing ");
        printf("Read data length is %i not %i", actual_len, BLOCK_SIZE);
        return;
      }

      printf("midbuffer: %16x", *midbuffer);
      free(midbuffer);
      // If skylander #1 -> buffer[1] = 0x10, else buffer[1] = 0x11
      outbuffer = calloc(MSG_SIZE, 1);

      // can probably be done easier but fuck it
      outbuffer[0] = buffer[0];
      outbuffer[1] = buffer[1];
      outbuffer[2] = buffer[2];
      outbuffer[3] = buffer[3];
      outbuffer[4] = buffer[4];
      outbuffer[5] = buffer[5];
      outbuffer[6] = buffer[6];
      outbuffer[7] = buffer[7];
      outbuffer[8] = buffer[8];
      outbuffer[9] = buffer[9];
      outbuffer[10] = buffer[10];
      outbuffer[11] = buffer[11];
      outbuffer[12] = buffer[12];
      outbuffer[13] = buffer[13];
      outbuffer[14] = buffer[14];
      outbuffer[15] = buffer[15];
      outbuffer[16] = buffer[16];
      outbuffer[17] = buffer[17];
      outbuffer[18] = buffer[18];

      outbuffer[1] = 0x10;
      if (buffer[1] == 0x11)
      {
        outbuffer[1] = 0x11;
      }
      tud_hid_report(0, outbuffer, bufsize);
      break;
    case 'C': // 0x42 Skylander Portal Color
      printf("Color ");
      tud_hid_report(0, buffer, bufsize);
      return;
      // tud_hid_report(0, buffer, bufsize);
      break;
    default:
      return;
    }

    printf("Sent: ");
    for (char *printptr = outbuffer; *printptr != '\0'; printptr++)
    {
      printf("%x", *printptr);
    }
    printf("\n");
    free(outbuffer);
  }
}

#include "io.h"
#include "bt.h"
#include "fb.h"

#define MAX_MSG_LEN    50
#define MAX_READ_RUN   100

unsigned char data_buf[MAX_MSG_LEN];
unsigned int data_len;
unsigned int messages_received = 0;
unsigned int poll_state = 0;

enum {
    LE_EVENT_CODE             = 0x3e,
    LE_CONNECT_CODE           = 0x01,
    LE_ADREPORT_CODE          = 0x02,
    HCI_ACL_PKT               = 0x02,
    HCI_EVENT_PKT             = 0x04
};

unsigned int got_echo_sid = 0;
unsigned int got_echo_name = 0;
unsigned char echo_addr[6];

unsigned int connected = 0;
unsigned int connection_handle = 0;

unsigned char dir = 50;

int memcmp(const char *str1, const char *str2, int count) {
    const char *s1 = (const char*)str1;
    const char *s2 = (const char*)str2;

    while (count-- > 0) {
       if (*s1++ != *s2++) return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

void hci_poll2(unsigned char byte)
{
    switch (poll_state) {
       case 0:
	  if (byte != HCI_EVENT_PKT) poll_state = 0;
	  else poll_state = 1;
	  break;
       case 1:
	  if (byte != LE_EVENT_CODE) poll_state = 0;
	  else poll_state = 2;
	  break;
       case 2:
	  if (byte > MAX_MSG_LEN) poll_state = 0;
	  else {
	     poll_state = 3;
	     data_len = byte;
	  }
	  break;
       default:
	  data_buf[poll_state - 3] = byte;
	  if (poll_state == data_len + 3 - 1) {
	     messages_received++;
             poll_state = 0;
	  } else poll_state++;
   }
}

unsigned char *hci_poll()
{
    unsigned int goal = messages_received + 1;

    if (bt_isReadByteReady()) {
       unsigned int run = 0;

       while (run < MAX_READ_RUN && messages_received < goal && bt_isReadByteReady()) {
          unsigned char byte = bt_readByte(); 
	  hci_poll2(byte);
	  run++;
       }
       if (run == MAX_READ_RUN) return 0;
       else return data_buf;
    }
    return 0;
}

void bt_search(void) {
    unsigned char *buf;

    while ( (buf = hci_poll()) ) {
       if (data_len >= 2) {
          if (buf[0] == LE_ADREPORT_CODE) {
             if (buf[1] == 1) { // num_reports
                if (buf[2] == 0) { // event_type
                   int bufindex = 0;
		   unsigned char ad_len = buf[11];

                   for (int c=9;c>=4;c--) echo_addr[9-c] = buf[bufindex + c]; // save the mac address
                   bufindex += 11;

                   got_echo_sid = 0; got_echo_name = 0; // Reset the search state machine
                   do {
                      ad_len = buf[bufindex];
                      unsigned char ad_type = buf[bufindex + 1];
                      bufindex += 2;

                      if (ad_len >= 2) {
                         if (ad_type == 0x03) {
			    unsigned int sid = buf[bufindex] | (buf[bufindex + 1] << 8);
			    if (sid == 0xEC00) {
			       got_echo_sid = 1;
			       debugstr("got sid... ");
			    }
                         } else if (ad_type == 0x09) {
                            char remote_name[ad_len - 1];
		            unsigned int d=0;

		            while (d<ad_len - 1) {
			       remote_name[d] = buf[bufindex + d];
		               d++;
		            }
			    if (!memcmp(remote_name,"echo",4)) {
			       got_echo_name = 1;
			       debugstr("got name... ");
			    }
                         }
                      }

                      bufindex += ad_len - 1;
                   } while (bufindex < data_len);
                }
             }
          }
       }
    }
}

void bt_conn()
{
    unsigned char *buf;

    while ( (buf = hci_poll()) ) {
       if (!connected && data_len >= 2 && buf[0] == LE_CONNECT_CODE) {
          connected = !*(buf+1);
	  debughex(connected); debugstr(" ");
	  connection_handle = *(buf+2) | (*(buf+3) << 8);
	  debughex(connection_handle); debugstr(" ");

	  if (connection_handle == 0) wait_msec(0x186A);
       }
    }
}

// The screen
#define WIDTH         1920
#define HEIGHT        1080
#define MARGIN        30
#define VIRTWIDTH     (WIDTH-(2*MARGIN))
#define FONT_BPG      8

// For the bricks
#define ROWS          5
#define COLS          10

// Gameplay
#define NUM_LIVES     3

// OBJECT TRACKING

struct Object
{
    unsigned int type;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned char alive;
};

enum {
    OBJ_NONE   = 0,
    OBJ_BRICK  = 1,
    OBJ_PADDLE = 2,
    OBJ_BALL   = 3
};

unsigned int numobjs = 0;
struct Object *objects = (struct Object *)SAFE_ADDRESS;
struct Object *ball;
struct Object *paddle;
const int paddlewidth = 80;
const int ballradius = 15;

void removeObject(struct Object *object)
{
    if (object->type == OBJ_BALL) {
       drawCircle(object->x + ballradius, object->y + ballradius, ballradius, 0, 1);
    } else {
       drawRect(object->x, object->y, object->x + object->width, object->y + object->height, 0, 1);
    }
    object->alive = 0;
}

void moveObjectAbs(struct Object *object, int x, int y)
{
    if (object->type == OBJ_BALL) {
       drawCircle(object->x + ballradius, object->y + ballradius, ballradius, 0, 1);
       drawCircle(x + ballradius, y + ballradius, ballradius, 0x55, 1);
    } else {
       moveRectAbs(object->x, object->y, object->width, object->height, x, y, 0x00);
    }
    object->x = x;
    object->y = y;
}

struct Object *detectCollision(struct Object *with, int xoff, int yoff)
{
    for (int i=0; i<numobjs;i++) {
	if (&objects[i] != with && objects[i].alive == 1) {
	   if (with->x + xoff > objects[i].x + objects[i].width || objects[i].x > with->x + xoff + with->width) {
              // with is too far left or right to collide
	   } else if (with->y + yoff > objects[i].y + objects[i].height || objects[i].y > with->y + yoff + with->height) {
              // with is too far up or down to collide
	   } else {
	      // Collision!
	      return &objects[i];
	   }
        }
    }
    return 0;
}

// OBJECT INITIALISERS

void initBricks()
{
    int brickwidth = 32;
    int brickheight = 8;
    int brickspacer = 20;
    const int brickcols[] = { 0x11, 0x22, 0xEE, 0x44, 0x66 };

    int ybrick = MARGIN + brickheight;

    for (int i=0; i<ROWS; i++) {
       int xbrick = MARGIN + (VIRTWIDTH/COLS/2) - (brickwidth/2);
        
       for (int j = 0; j<COLS; j++) {
          drawRect(xbrick, ybrick, xbrick+brickwidth, ybrick+brickheight, brickcols[i], 1);

	  objects[numobjs].type = OBJ_BRICK;
	  objects[numobjs].x = xbrick;
	  objects[numobjs].y = ybrick;
	  objects[numobjs].width = brickwidth;
	  objects[numobjs].height = brickheight;
	  objects[numobjs].alive = 1;
	  numobjs++;

          xbrick += (VIRTWIDTH/COLS);
        }
        ybrick = ybrick + brickspacer;
     }
}

void initBall()
{
    drawCircle(WIDTH/2, HEIGHT/2, ballradius, 0x55, 1);

    objects[numobjs].type = OBJ_BALL;
    objects[numobjs].x = (WIDTH/2) - ballradius;
    objects[numobjs].y = (HEIGHT/2) - ballradius;
    objects[numobjs].width = ballradius * 2;
    objects[numobjs].height = ballradius * 2;
    objects[numobjs].alive = 1;
    ball = &objects[numobjs];
    numobjs++;
}

void initPaddle()
{
    int paddleheight = 20;
    int startx = MARGIN + (dir * ((VIRTWIDTH - paddlewidth + MARGIN)/100));

    drawRect(startx, (HEIGHT-MARGIN-paddleheight), startx + paddlewidth, (HEIGHT-MARGIN), 0x11, 1);

    objects[numobjs].type = OBJ_PADDLE;
    objects[numobjs].x = startx;
    objects[numobjs].y = (HEIGHT-MARGIN-paddleheight);
    objects[numobjs].width = paddlewidth;
    objects[numobjs].height = paddleheight;
    objects[numobjs].alive = 1;
    paddle = &objects[numobjs];
    numobjs++;
}

void drawScoreboard(int score, int lives)
{
    char tens = score / 10; score -= (10 * tens);
    char ones = score;

    drawString((WIDTH/2)-252, MARGIN-25, "Score: 0     Lives:  ", 0x0f, 3);
    drawChar(tens + 0x30, (WIDTH/2)-252 + (8*8*3), MARGIN-25, 0x0f, 3);
    drawChar(ones + 0x30, (WIDTH/2)-252 + (8*9*3), MARGIN-25, 0x0f, 3);
    drawChar((char)lives + 0x30, (WIDTH/2)-252 + (8*20*3), MARGIN-25, 0x0f, 3);
}

void acl_poll()
{
    while (bt_isReadByteReady()) {
       unsigned char byte = bt_readByte(); 

       if (byte == HCI_EVENT_PKT) {
	  bt_readByte(); // opcode
	  unsigned char length = bt_readByte();
	  for (int i=0;i<length;i++) bt_readByte();
       } else if (byte == HCI_ACL_PKT) {
	  unsigned char h1 = bt_readByte(); // handle1
	  unsigned char h2 = bt_readByte(); // handle2
          unsigned char thandle = h1 | (h2 << 8);

	  unsigned char d1 = bt_readByte();
	  unsigned char d2 = bt_readByte();

	  unsigned int dlen = d1 | (d2 << 8);
	  unsigned char data[dlen];

	  if (dlen) {
	     for (int i=0;i<dlen;i++) data[i] = bt_readByte();

	     unsigned int length = data[0] | (data[1] << 8);
	     unsigned int channel = data[2] | (data[3] << 8);
	     unsigned char opcode = data[4];

	     if (thandle == connection_handle && length == 4 && opcode == 0x1b) {
	        if (channel == 4 && data[5] == 0x1b && data[6] == 0x00) {
	      	   dir = data[7];
                   moveObjectAbs(paddle, MARGIN + (dir * ((VIRTWIDTH - paddlewidth + MARGIN)/100)), paddle->y);
                }
	     }
          }
       }
    }
}

void breakout()
{
    struct Object *foundObject;

    int bricks = ROWS * COLS;
    int lives = NUM_LIVES;
    int points = 0;
    
    int velocity_x = 1;
    int velocity_y = 3;

    initBricks();
    initBall();
    initPaddle();
    drawScoreboard(points, lives);

    while (lives > 0 && bricks > 0) {
       acl_poll();

       // Are we going to hit anything?
       foundObject = detectCollision(ball, velocity_x, velocity_y);

       if (foundObject) {
          if (foundObject == paddle) {
             velocity_y = -velocity_y;
	     // Are we going to hit the side of the paddle
	     if (ball->x + ball->width + velocity_x == paddle->x || ball->x + velocity_x == paddle->x + paddle->width) velocity_x = -velocity_x;
          } else if (foundObject->type == OBJ_BRICK) {
             removeObject(foundObject);
             velocity_y = -velocity_y;
             bricks--;
             points++;
             drawScoreboard(points, lives);
          }
       }

       wait_msec(0x186A);
       moveObjectAbs(ball, ball->x + velocity_x, ball->y + velocity_y);

       // Check we're in the game arena still
       if (ball->x + ball->width >= WIDTH-MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->x <= MARGIN) {
          velocity_x = -velocity_x;
       } else if (ball->y + ball->height >= HEIGHT-MARGIN) {
          lives--;

	  removeObject(ball);
	  removeObject(paddle);

          drawScoreboard(points, lives);
	  if (lives) {
             initBall();
             initPaddle();
	  }
       } else if (ball->y <= MARGIN) {
          velocity_y = -velocity_y;
       }
    }

    int zoom = WIDTH/192;
    int strwidth = 10 * FONT_BPG * zoom;
    int strheight = FONT_BPG * zoom;

    if (bricks == 0) drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Well done!", 0x02, zoom);
    else drawString((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), "Game over!", 0x04, zoom);

    wait_msec(0x500000); // Wait 5 seconds

    drawRect((WIDTH/2)-(strwidth/2), (HEIGHT/2)-(strheight/2), (WIDTH/2)+(strwidth/2), (HEIGHT/2)+(strheight/2), 0, 1);
    numobjs = 0;
}

void main()
{
    fb_init();
    uart_init();
    bt_init();

    debugstr("Initialising Bluetooth: ");
    debugstr(">> reset: ");
    bt_reset();
    debugstr(">> firmware load: ");
    bt_loadfirmware();
    debugstr(">> set baud: ");
    bt_setbaud();
    debugstr(">> set bdaddr: ");
    bt_setbdaddr();

    // Print the BD_ADDR
    unsigned char local_addr[6];
    bt_getbdaddr(local_addr);
    for (int c=5;c>=0;c--) debugch(local_addr[c]);
    debugcrlf();

    // Start scanning
    debugstr("Setting event mask... ");
    setLEeventmask(0xff);
    debugstr("Starting scanning... ");
    startActiveScanning();

    // Search for the echo
    debugstr("Waiting...");
    debugcrlf();
    while (!(got_echo_sid && got_echo_name)) bt_search();
    stopScanning();
    for (int c=0;c<=5;c++) debugch(echo_addr[c]);
    debugcrlf();

    // Connecting to echo
    debugstr("Connecting to echo: ");
    connect(echo_addr);
    while (!(connected && connection_handle)) bt_conn();
    debugstr("Connected!");
    debugcrlf();

    // Subscribe to updates
    debugstr("Sending read request: ");
    debughex(connection_handle); debugcrlf();
    sendACLsubscribe(connection_handle);

    // Begin the game
    debugstr("Let the game commence...\n");
    wait_msec(0x100000); // Wait a second

    while (1) breakout();
}

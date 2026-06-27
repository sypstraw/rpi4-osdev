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

void acl_poll()
{
    while (bt_isReadByteReady()) {
       unsigned char byte = bt_readByte(); 

       if (byte == HCI_EVENT_PKT) {
	  bt_waitReadByte(); // opcode
	  unsigned char length = bt_waitReadByte();
	  for (int i=0;i<length;i++) bt_waitReadByte();
       } else if (byte == HCI_ACL_PKT) {
	  unsigned char h1 = bt_waitReadByte(); // handle1
	  unsigned char h2 = bt_waitReadByte(); // handle2
          unsigned char thandle = h1 | (h2 << 8);

	  unsigned char d1 = bt_waitReadByte();
	  unsigned char d2 = bt_waitReadByte();

	  unsigned int dlen = d1 | (d2 << 8);
	  unsigned char data[dlen];

	  if (dlen) {
	     for (int i=0;i<dlen;i++) data[i] = bt_waitReadByte();

	     unsigned int length = data[0] | (data[1] << 8);
	     unsigned int channel = data[2] | (data[3] << 8);
	     unsigned char opcode = data[4];

             if (thandle == connection_handle && length == 4 && opcode == 0x1b) {
	        if (channel == 4 && data[5] == 0x1b && data[6] == 0x00) {
                   debugcrlf();
                   debugstr("Got ACL packet... ");
                   debugch(data[7]); 
                }
	     }
          }
       }
    }
}

void run_search(void) {
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
    while (!connected) bt_conn();
    debugstr("Connected!");
    debugcrlf();

    // Get the characteristic value
    debugstr("Sending read request: ");
    debughex(connection_handle); debugcrlf();
    sendACLsubscribe(connection_handle);

    // Enter an infinite loop
    debugstr("Going loopy...");
    while (1) {
       acl_poll();
       uart_update();
    }
}

void run_eddystone(void) {
    // Start advertising
    debugstr("Setting event mask... ");
    setLEeventmask(0xff);
    debugstr("Starting advertising... ");
    startActiveAdvertising();

    // Enter an infinite loop
    debugstr("Going loopy...");
    while (1) {
       uart_update();
    }
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

    // Test out the advertising
    // run_eddystone();

    // Test out the scanning
    run_search();
}

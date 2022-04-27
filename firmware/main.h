#ifndef _MAIN_H_
#define _MAIN_H_

#define ON  1
#define OFF 0

#define PIN_SCK 18
#define PIN_MOSI 19
#define PIN_MISO 16
#define PIN_CS 17
#define PIN_RST 20

/* PROJECT SPECIFIC */
#define ALERT_LED               14      // Alert LED
#define E_SWITCH                13      // Emergency Switch 
#define LINK_STS_LED            25      // Onboard LED is used to indicate  DHCP Success/Fail


#define BIT0		(1UL << 0)


#define BRD_SOCKET          1         // W5100S provides four independent SOCKETs
#define BRD_CLIENT_PORT     49153     // There are 65,535 ports available for communication between devices


void send_normal_data(void);
void send_alert_data(void);


#endif
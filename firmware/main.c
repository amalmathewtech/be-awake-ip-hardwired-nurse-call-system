#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#include "main.h"
#include "w5100s.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "dhcp.h"

#define SPI_PORT spi0

wiz_NetInfo gWIZNETINFO = { .mac = {0x00,0x08,0xdc,0xff,0xff,0x89},
                            .ip = {192,168,0,5},
                            .sn = {255, 255, 255, 0},
                            .gw = {192, 168, 0, 1},
                            .dns = {168, 126, 63, 1},
//                          .dhcp = NETINFO_STATIC};
                            .dhcp = NETINFO_DHCP};


/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */

bool toggle_val = 0;
uint on_off_flag = 0;

uint8_t SourceIP[4];
uint8_t LANBuffer[1472];

uint8_t alert_msg[] = "0200";
uint8_t normal_msg[] = "0201";

uint8_t BroadCastIP[4] = {192,168,0,102};   // Replace with  Your Destination  IP Address
uint16_t DestinationPort = 49153;           // Port Number 

bool periodic_timer_callback(struct repeating_timer *t)
{
    if (on_off_flag == 1)
    {
        toggle_val = !toggle_val;
        gpio_put(ALERT_LED, toggle_val);
        send_alert_data();
    }
    else{

        send_normal_data();
    }
    return true;
}

void dhcp_assign(void)
{
    getIPfromDHCP(gWIZNETINFO.ip);
    getGWfromDHCP(gWIZNETINFO.gw);
    getSNfromDHCP(gWIZNETINFO.sn);
    getDNSfromDHCP(gWIZNETINFO.dns);

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
}

/**
 * @brief  The call back function of ip update.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_update(void)
{
    ;
}

/**
 * @brief  The call back function of ip conflict.
 * @note
 * @param  None
 * @retval None
 */
void dhcp_conflict(void)
{
    ;
}

static void wizchip_reset(void)
{
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);
    bi_decl(bi_1pin_with_name(PIN_RST, "W5x00 RESET"));
}

static void wizchip_initialize(void)
{
        /* Deselect the FLASH : chip select high */
    wizchip_cs_deselect();

    /* CS function register */
    reg_wizchip_cs_cbfunc(0, 0);

    /* SPI function register */
    reg_wizchip_spi_cbfunc(0, 0);
#ifdef USE_SPI_DMA
    reg_wizchip_spiburst_cbfunc(wizchip_read_burst, wizchip_write_burst);
#endif
    //reg_wizchip_cris_cbfunc(wizchip_critical_section_lock, wizchip_critical_section_unlock);

    /* W5x00 initialize */
    uint8_t temp;
    uint8_t memsize[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};

    if (ctlwizchip(CW_INIT_WIZCHIP, (void *)memsize) == -1)
    {
        printf(" W5x00 initialized fail\n");

        return;
    }

    /* Check PHY link status */
    do
    {
        if (ctlwizchip(CW_GET_PHYLINK, (void *)&temp) == -1)
        {
            printf(" Unknown PHY link status\n");

            return;
        }
    } while (temp == PHY_LINK_OFF);
}

static void wizchip_check(void)
{
    /* Read version register */
    if (getVER() != 0x51) // W5100S
    {
        printf(" ACCESS ERR : VERSIONR != 0x51, read value = 0x%02x\n", getVER());

        while (1)
            ;
    }
}

void print_network_information(void)
{
    wizchip_getnetinfo(&gWIZNETINFO);
    printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n\r",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
    printf("IP address : %d.%d.%d.%d\n\r",gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
    printf("SM Mask    : %d.%d.%d.%d\n\r",gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
    printf("Gate way   : %d.%d.%d.%d\n\r",gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
    printf("DNS Server : %d.%d.%d.%d\n\r",gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
}

int main()
{
    uint8_t tmp1, tmp2;
    uint32_t ret;
    uint8_t dhcp_retry = 0;
    gpio_init(ALERT_LED);
    gpio_init(LINK_STS_LED);
    gpio_set_dir(LINK_STS_LED, GPIO_OUT);
    gpio_set_dir(ALERT_LED, GPIO_OUT);
    gpio_set_dir(E_SWITCH, GPIO_IN);
    gpio_set_input_enabled(E_SWITCH, true);
    struct repeating_timer timer;
    add_repeating_timer_ms(150, periodic_timer_callback, NULL, &timer);
    // Initialize chosen serial port
    stdio_init_all();

    // spi_initilize();
    //  this example will use SPI0 at 5MHz
    spi_init(SPI_PORT, 5000 * 1000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    // make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));
    // chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // make the SPI pins available to picotool
    bi_decl(bi_1pin_with_name(PIN_CS, "W5x00 CHIP SELECT"))
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();
    uint8_t LinkStatus = 0;
    LinkStatus = WIZCHIP_READ(VERR);
    /* Wait Phy Link */
    while(1){
        ctlwizchip(CW_GET_PHYLINK, &tmp1 );
        ctlwizchip(CW_GET_PHYLINK, &tmp2 );
        if(tmp1==PHY_LINK_ON && tmp2==PHY_LINK_ON) break;
    }
    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
    DHCP_init(0, LANBuffer);
    reg_dhcp_cbfunc(dhcp_assign, dhcp_update, dhcp_conflict);

    if (gWIZNETINFO.dhcp == NETINFO_DHCP) {       // CHECK DHCP
        printf("Start DHCP\r\n");
        while (1) {
            ret = DHCP_run();
            if (ret == DHCP_IP_LEASED) {
                printf("DHCP Success\r\n");
                gpio_put(LINK_STS_LED, ON);
                break;
            }
                else if (ret == DHCP_FAILED) {
                dhcp_retry++;
            }

            if (dhcp_retry > 3) {
                printf("DHCP Fail\r\n");
                gpio_put(LINK_STS_LED, OFF);
                break;
            }
        }
    }

    ctlnetwork(CN_SET_NETINFO, (void*) &gWIZNETINFO);
    printf("Register value after W5100S initialize!\r\n");
    print_network_information();

    if(getSn_SR(BRD_SOCKET) != SOCK_UDP){
	    socket(BRD_SOCKET, Sn_MR_UDP, BRD_CLIENT_PORT, 0x00);
	}

    for (;;)
    {
        if (gpio_get(E_SWITCH) == 1)
        {
            sleep_ms(250);
            while (gpio_get(E_SWITCH) == 1)
                ;
            on_off_flag = !on_off_flag;
        }
        if (on_off_flag == 0)
        {
            gpio_put(ALERT_LED, OFF);
        }
    }
    return 0;
}


void send_alert_data(void)
{
    sendto(BRD_SOCKET, (uint8_t *)&alert_msg, sizeof(alert_msg), BroadCastIP, DestinationPort);  
}

void send_normal_data(void)
{
    sendto(BRD_SOCKET, (uint8_t *)&normal_msg, sizeof(normal_msg), BroadCastIP, DestinationPort);
}
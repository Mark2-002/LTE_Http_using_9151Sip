

#include <stdio.h>
#include <ncs_version.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include "http.h"
#include <modem/nrf_modem_lib.h>
#include <nrf_modem.h>
#include <nrf_socket.h> 
bool net_connected = false;
int server_connection;
bool ser_flag = 0;
int ser_retry = 0;
uint8_t counter = 0;
static K_SEM_DEFINE(lte_connected, 0, 1);
LOG_MODULE_REGISTER(main_c, LOG_LEVEL_INF);
uint16_t data[32];
char text_data[] = "Hello This is testing of the text sending via HTTP POST ";
void set_manual_dns(void)
{
	int err;
	struct nrf_in6_addr dns_addr = {.s6_addr = {0x20, 0x01, 0x06, 0x7c, 0x02, 0xb0, 0x00, 0x00,
												0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04}};

	err = nrf_setdnsaddr(NRF_AF_INET6, &dns_addr, sizeof(dns_addr));
	if (err)
	{
		printk("Failed to set DNS: %d\n", err);
	}
	else
	{
		printk("Manual NAT64 DNS configured successfully!\n");
	}
}
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type)
	{
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING))
		{
			break;
		}
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		dk_set_led_on(DK_LED2);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		net_connected = 1;
		LOG_INF("RRC mode: %s,%d", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle", net_connected);
		break;
	default:
		break;
	}
}
static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");
	err = nrf_modem_lib_init();
	if (err)
	{
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}
	set_manual_dns();
	k_msleep(100);
	LOG_INF("Connecting to LTE network");
	err = lte_lc_connect_async(lte_handler);
	if (err)
	{
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
		return err;
	}
	return 0;
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & DK_BTN1_MSK && button_state & DK_BTN1_MSK)
	{
		if (server_connect() >= 0)
		{
			client_http_post_json(data);
			counter++;
		}
	}
	else if (has_changed & DK_BTN2_MSK && button_state & DK_BTN2_MSK)
	{

		if (server_connect() >= 0)
		{
			client_http_post_text(text_data);
			counter++;
		}
	}
}

void lte_Connection()
{
	int err;
	LOG_INF("Initializing the modem....");
	if (dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LEDs Library");
	}
	else
	{
		LOG_INF("Initialized LEDs");
	}
	err = modem_configure();
	if (err)
	{
		LOG_ERR("Failed to configure the modem");
		return 0;
	}
	else
	{
		LOG_INF("Modem configuration succesful.....");
	}

	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE network");
}

int main(void)
{

	while (1)
	{

		if (net_connected)
		{
			for (int i = 0; i < 20; i++)
			{
				printk("\r%d", i);
				k_msleep(1000);
			}
			if (server_resolve() != 0)
			{
				LOG_ERR("Failed to resolve server name");
			}

			LOG_INF("Connecting to %s:%s", CONFIG_HTTP_SAMPLE_HOSTNAME, CONFIG_HTTP_SAMPLE_PORT);
			if (server_connect() != 0)
			{
				LOG_ERR("Failed to connect to server");
			}
			else
			{
				printk("Server conected");
				dk_set_led_on(DK_LED1);
				break;
			}
		}
		else
		{
			k_msleep(10000);
		}
		LOG_INF("net not connected");
	}
	LOG_INF("Outside network loop");
	data[0] = 1;
	data[1] = 2;
	data[2] = 3;
	data[3] = 4;
	data[4] = 5;
	data[5] = 6;
	for (int j = 0; j < 3; j++)
	{
		client_http_post_json(data);
		k_msleep(1000);
		server_connect();
		client_http_post_text(text_data);
		k_msleep(1000);
		server_connect();
		client_http_put();
		k_msleep(1000);
		server_connect();
		client_http_get();
		k_msleep(1000);
		server_connect();
	}
	return 0;
}

K_THREAD_DEFINE(connection_tread, 4192, lte_Connection, NULL, NULL, NULL, 7, 0, 0);
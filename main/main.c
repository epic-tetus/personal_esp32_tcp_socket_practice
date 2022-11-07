#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static const char * TAG = "TCP_SOCKET_PRACTICE";

#define WIFI_SSID "iptime"
#define WIFI_PASS "01234567"
#define WIFI_RETRY 10
#define AUTH_MODE WIFI_AUTH_OPEN

EventGroupHandle_t wifi_event_group;

/**
 * @brief 이벤트 그룹은 각 이벤트에 여러 비트값들을 제공하는데, 여기서는 아래의 2개의 비트값만 알면 된다
 * BIT0: AP에 연결이 됨
 * BIT1: 재시도 횟수 만큼 계속 연결을 시도 해봤지만 결국 실패함
 */
#define SUCCESS_BIT BIT0
#define FAIL_BIT BIT1

uint32_t retry_count = 0;

void event_handler(void * arg, esp_event_base_t event_base, int32_t event_id, void * event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (retry_count < WIFI_RETRY)
		{
			esp_wifi_connect();
			retry_count++;
			ESP_LOGI(TAG, "Retry connecting...");
		}
		else
		{
			xEventGroupSetBits(wifi_event_group, FAIL_BIT);
		}
		ESP_LOGI(TAG, "Connect FAIL!!");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t * event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&event->ip_info.ip));
		retry_count = 0;
		xEventGroupSetBits(wifi_event_group, SUCCESS_BIT);
	}
}

void wifi_init_sta(void)
{
	wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&event_handler,
														NULL,
														&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
														IP_EVENT_STA_GOT_IP,
														&event_handler,
														NULL,
														&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASS,
			.threshold.authmode = AUTH_MODE,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	EventBits_t bits = xEventGroupWaitBits(wifi_event_group, SUCCESS_BIT | FAIL_BIT,
										   pdFALSE, pdFALSE, portMAX_DELAY);

	if (bits & SUCCESS_BIT)
	{
		ESP_LOGI(TAG, "CONNECTING SUCCESS!!\tSSID:%s PASS:%s", WIFI_SSID, WIFI_PASS);
	}
	else if (bits & FAIL_BIT)
	{
		ESP_LOGI(TAG, "CONNECTING FAIL!!\tSSID:%s, PASS:%s", WIFI_SSID, WIFI_PASS);
	}
	else
	{
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}
}

// 원본 코드
// https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station

void app_main(void)
{
	esp_err_t ret = nvs_flash_init();
	// ESP_ERROR_CHECK(nvs_flash_init());
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	wifi_init_sta();
}

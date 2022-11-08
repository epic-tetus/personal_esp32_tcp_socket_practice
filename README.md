# ESP32 TCP Socket Practice

기본 제공 tcp_client 예제에는 ```example_connect()``` 라는 함수와 sdkconfig 파일에 지정된 정보를 통해서 WIFI 설정 값들과 서버 주소 등을 설정하게 되어있는데,

다른 프로젝트에 적용하기 용이하지 않아서 연습삼아 만든 프로젝트

## WIFI 연결 예제
* https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station

### 실제 WIFI 연결하는 부분
```c
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
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));

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
```

## TCP 소켓 클라이언트 예제
* https://github.com/espressif/esp-idf/tree/master/examples/protocols/sockets/tcp_client

### TCP 태스크
```c
void tcp_client_task(void * pvParameters)
{
	uint8_t rx_buffer[128];
	char host_ip[] = SERVER_IP;
	int addr_family = 0;
	int ip_protocol = 0;

	while (1)
	{
		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = inet_addr(host_ip);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(SERVER_PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;

		sock = socket(addr_family, SOCK_STREAM, ip_protocol);
		if (sock < 0)
		{
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, SERVER_PORT);

		int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
		if (err != 0)
		{
			ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Successfully connected");

		while (1)
		{
			err = send(sock, packet, strlen(packet), 0);
			if (err < 0)
			{
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
				break;
			}

			int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
			if (len < 0)
			{
				ESP_LOGE(TAG, "recv failed: errno %d", errno);
				break;
			}
			else
			{
				rx_buffer[len] = 0;
				ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
				ESP_LOGI(TAG, "%s", rx_buffer);
			}
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}

		if (sock != -1)
		{
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(NULL);
}
```


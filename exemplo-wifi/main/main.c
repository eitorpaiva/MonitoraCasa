#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_websocket_client.h"
#include "mqtt_client.h"
#include "dht.h"

#define WIFI_SSID "IFMS-CX-LAB"
#define WIFI_PASSWORD "Coxim@@22"
#define WS_URI "ws://iot-ifms-2022.herokuapp.com/chat/websocket"

#define MQTT_CLIENT_ID "100"
#define MQTT_BROKER_URI "ws://iot-mqtt-ifms-2022.herokuapp.com"
#define MQTT_TOPIC_SUBSCRIBE "eitor/casa/sala/lampada"
#define MQTT_TOPIC_PUBLISH_TEMP "eitor/casa/sala/temperatura"
#define MQTT_TOPIC_PUBLISH_UMI "eitor/casa/sala/umidade"

esp_websocket_client_handle_t client;

void task_ler_dht(void* mqtt_client){
	while(1){
		float temperatura;
		float umidade;

		if(dht_read_float_data(DHT_TYPE_AM2301, GPIO_NUM_4, &umidade, &temperatura) == ESP_OK){
			printf("Temperatura: %.1fC Umidadade: %.1f%%\n", temperatura, umidade);
			if(mqtt_client){
				char temperaturaStr[5];
				sprintf(temperaturaStr, "%.1f", temperatura);

				char umidadeStr[5];
				sprintf(umidadeStr, "%.1f", umidade);

				esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_PUBLISH_TEMP, temperaturaStr, 5, 1, 0);
				esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_PUBLISH_UMI, umidadeStr, 5, 1, 0);
			}
		}else{
			printf("Falha ao ler o sensor\n");
		}
		vTaskDelay(2000 / portTICK_RATE_MS);
	}
}

void manipulador_mqtt(void* args, esp_event_base_t base, int32_t event_id, void *dados){
	esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)dados;
	if(event_id == MQTT_EVENT_CONNECTED){
		esp_mqtt_client_handle_t mqtt_client = event->client;

		printf("CONECTADO AO BROKER MQTT\n");

		xTaskCreatePinnedToCore(task_ler_dht, "ler_dht", 4*1024, mqtt_client, 10, NULL, 1);

		esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_SUBSCRIBE, 0);
	}
	if(event_id == MQTT_EVENT_SUBSCRIBED){
		printf("SUBACK RECEBIDO\n");
	}

	if(event_id == MQTT_EVENT_DATA){
		printf("PUBLISH RECEBIDO\n");
		printf("Tópico: %.*s.\n", event->topic_len, event->topic);
		printf("Mensagem: %.*s.\n", event->data_len, event->data);

		char valor[6];
		strncpy(valor, &event->data[0], 5);
		valor[5] = '\0';

		if(strcmp(valor, "ligar") == 0){
			gpio_set_level(GPIO_NUM_2, 1);
		}

		if(strcmp(valor, "desli") == 0){
			gpio_set_level(GPIO_NUM_2, 0);
		}


	}
}


void mqtt_init(){
	const esp_mqtt_client_config_t mqtt_config = {
			.uri = MQTT_BROKER_URI,
			.client_id = MQTT_CLIENT_ID
	};
	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
	esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, &manipulador_mqtt, mqtt_client);
	esp_mqtt_client_start(mqtt_client);
}

void manipulador_websocket(void *args, esp_event_base_t event_base, int32_t event_id, void *dados){
	if(event_id == WEBSOCKET_EVENT_CONNECTED){
		printf("Conectado ao servidor Websocket\n");
	}

	if(event_id == WEBSOCKET_EVENT_DISCONNECTED){

	}

	if(event_id == WEBSOCKET_EVENT_DATA){
		esp_websocket_event_data_t *msg = (esp_websocket_event_data_t *) dados;
		if(msg->op_code == 1){
			printf("Tamanho: %d\n", msg->data_len);
			printf("Mensagem: %.*s\n", msg->data_len, (char*) msg->data_ptr);

			char check[6];
			strncpy(check, &msg->data_ptr[9], 5);
			check[5] = '\0';
			printf("Check: %s\n",check);

			if(strcmp(check, "eitor") == 0){
				char valor[6];
				strncpy(valor, &msg->data_ptr[25], 5);
				printf("Valor: %s\n", valor);

				if(strcmp(valor, "ligar") == 0){
					gpio_set_level(GPIO_NUM_2, 1);
				}

				if(strcmp(valor, "desli") == 0){
					gpio_set_level(GPIO_NUM_2, 0);
				}
			}
		}
	}
}

void iniciar_websocket(){
	esp_websocket_client_config_t ws_config = {
			.uri = WS_URI
	};
	client = esp_websocket_client_init(&ws_config);
	esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, &manipulador_websocket, (void *)client);
	esp_websocket_client_start(client);
}

void manipulador_de_eventos_wifi(void *arg, esp_event_base_t event_base, int32_t event_id, void *dados){
	printf("Entrou no manipulador de eventos\n");
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
		printf("Modo Station de Wifi Iniciado\n");
		esp_wifi_connect();
	}

	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED){
		printf("Conexao com a rede estabelecida\n");
	}

	if(event_base == IP_EVENT  && event_id == IP_EVENT_STA_GOT_IP){
		printf("Recebeu um endereco IP\n");
		//iniciar_websocket();
		mqtt_init();
	}
}

void app_main(void)
{
	nvs_flash_init(); // inicialização modulo flash
	esp_netif_init(); // inicialização do gerenciador de rede
	esp_event_loop_create_default(); // inicializa um event loop

	esp_netif_create_default_wifi_sta(); // configura o esp como station
	esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, &manipulador_de_eventos_wifi, NULL);

	wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT(); // parametros de inicialização do modulo de wifi

	wifi_config_t wifi_config = { // parametros da rede
			.sta = {
					.ssid = WIFI_SSID,
					.password = WIFI_PASSWORD
			}
	};

	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	esp_wifi_init(&config);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();
}

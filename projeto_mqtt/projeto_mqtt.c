#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"
#include "ws2812.pio.h"
#include "ssd1306.h"
#include "font.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Configurações Wi-Fi e MQTT
#define WIFI_SSID "Galaxy S20 FE 5G"
#define WIFI_PASSWORD "abcd9700"
#define MQTT_SERVER "172.27.48.241"
#define MQTT_USERNAME "joao_pedro"
#define MQTT_PASSWORD "1234"

// Definições dos pinos
#define BOTAO_A 5
#define BOTAO_B 6
#define AZUL 12
#define VERDE 11
#define VERMELHO 13
#define VERTICALY 26
#define HORIZONTALX 27
#define SELEC 22
#define PWM_WRAP 4095
#define PWM_CLK_DIV 30.52f
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_I2C 0x3C
#define buzzer1 10
#define buzzer2 21
#define IS_RGBW false 
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define DEADZONE 300

// Definições de configuração do display
#define WIDTH 128
#define HEIGHT 64

// Definições MQTT
#define TEMP_WORKER_TIME_S 3
#define MQTT_KEEP_ALIVE_S 60
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1
#define MQTT_DEVICE_NAME "pico"
#define MQTT_UNIQUE_TOPIC 0
#define MQTT_TOPIC_LEN 100
#define MQTT_OUTPUT_RINGBUF_SIZE 256

// Variáveis globais
ssd1306_t ssd;
absolute_time_t last_interrupt_time = 0;
float temperatura = 25.0f;
float umidade = 50.0f;
bool temp1 = false;
bool temp2 = false;
bool temp3 = false;
bool temp4 = false;
bool global1 = false;
bool global2 = false;
bool global3 = false;
bool global4 = false;
bool ventilador_ligado = false;
bool umidificador_ligado = false;
bool desumidificador_ligado = false;
bool aquecedor_ligado = false;
absolute_time_t ultimo_bip;
bool estado_buzzer = false;
bool buzzer_ativo = false;
int ciclos_buzzer = 0;
// Status do sistema
char status_sistema[30] = "Sistema OK";
// Estrutura de dados do cliente MQTT
typedef struct {
    mqtt_client_t* mqtt_client_inst;
    struct mqtt_connect_client_info_t mqtt_client_info;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
} MQTT_CLIENT_DATA_T;

// Protótipos de funções
static void pub_request_cb(__unused void *arg, err_t err);
static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name);
static void sub_request_cb(void *arg, err_t err);
static void unsub_request_cb(void *arg, err_t err);
static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void temperature_worker_fn(async_context_t *context, async_at_time_worker_t *worker);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void start_client(MQTT_CLIENT_DATA_T *state);
static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
static void publish_all_states(MQTT_CLIENT_DATA_T *state);

static async_at_time_worker_t temperature_worker = { .do_work = temperature_worker_fn };

// Desenhos para LEDs
double desenho0[25] = {
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2
};

double desenho1[25] = {
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.2, 0.2, 0.2, 0.2, 0.2,
    0.2, 0.2, 0.2, 0.0, 0.2,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.2, 0.0, 0.0, 0.0, 0.2
};

double desenho2[25] = {
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0
};

double desenho3[25] = {
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0,
    0.2, 0.0, 0.2, 0.0, 0.2,
    0.0, 0.2, 0.2, 0.2, 0.0,
    0.0, 0.0, 0.2, 0.0, 0.0
};

// Função para controlar LEDs WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void num0(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);  
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho0[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num1(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);  
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho1[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num2(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);  
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho2[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

void num3(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);  
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (desenho3[i]) {
            put_pixel(color);
        } else {
            put_pixel(0);
        }
    }
}

// Configuração do PWM
void pwm_init_gpio(uint gpio, uint wrap, float clkdiv) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, wrap);
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_init(slice_num, &config, true);
}

// Função para atualizar temperatura e umidade com joystick
void adc_incremental(float *temperatura, float *umidade) {
    adc_select_input(0); // eixo Y
    uint16_t valor_y = adc_read();
    adc_select_input(1); // eixo X
    uint16_t valor_x = adc_read();

    int16_t diff_y = 2048 - (int16_t)valor_y;
    int16_t diff_x = (int16_t)valor_x - 2048;

    if (abs(diff_y) > DEADZONE) {
        *temperatura += (diff_y > 0) ? -2.0f : 2.0f;
        if (*temperatura < -20.0f) *temperatura = -20.0f;
        if (*temperatura > 100.0f) *temperatura = 100.0f;
    }

    if (abs(diff_x) > DEADZONE) {
        *umidade += (diff_x > 0) ? 2.0f : -2.0f;
        if (*umidade < 0.0f) *umidade = 0.0f;
        if (*umidade > 100.0f) *umidade = 100.0f;
    }
}

// Função para atualizar buzzer
void atualiza_buzzer() {
    if (!buzzer_ativo) return;

    if (absolute_time_diff_us(ultimo_bip, get_absolute_time()) >= 250000) { // 250ms
        ultimo_bip = get_absolute_time();
        estado_buzzer = !estado_buzzer;
        pwm_set_gpio_level(buzzer1, estado_buzzer ? 250 : 0);
        pwm_set_gpio_level(buzzer2, estado_buzzer ? 250 : 0);

        ciclos_buzzer++;
        if (ciclos_buzzer >= 8) {  // 4 ciclos ON/OFF = 1 segundo
            buzzer_ativo = false;
            pwm_set_gpio_level(buzzer1, 0);
            pwm_set_gpio_level(buzzer2, 0);
            ciclos_buzzer = 0;
        }
    }
}

// Função de análise de condições ambientais
void analise() {
    if (temperatura > 55.0f) {
        global1 = !global1;
        if (temp1 == false && global1 == true) {
            printf("Temperatura elevada, por favor ligue o ventilador\n");    
        }
        num1(255, 0, 0);
        strcpy(status_sistema, "Temp. Elevada");  // Novo
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
    } else if (temperatura < 5.0f) {
        temp4 = !temp4;
        printf("Sistema operando em temperaturas críticas, ligue o aquecedor\n");
        num0(0, 0, 255);
        strcpy(status_sistema, "Temp. Critica");  // Novo
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
    } else if (umidade > 70.0f) {
        temp3 = !temp3;
        printf("Umidade muito elevada, ligue o desumidificador\n");
        num3(0, 255, 255);
        strcpy(status_sistema, "Umid. Elevada");  // Novo
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
    } else if (umidade < 30.0f) {
        global2 = !global2;
        if (temp2 == false && global2 == true) {
            printf("Umidade em situação crítica, ligue o umidificador\n");
        }
        num2(255, 165, 0);
        strcpy(status_sistema, "Umid. Critica");  // Novo
        if (!buzzer_ativo) {
            buzzer_ativo = true;
            ultimo_bip = get_absolute_time();
            ciclos_buzzer = 0;
        }
    } else {
        strcpy(status_sistema, "Sistema OK");  // Novo
        num0(0, 0, 0);
        num1(0, 0, 0);
        num2(0, 0, 0);
        num3(0, 0, 0);
    }
}

// Callback para interrupções dos botões
void gpio_callback(uint gpio, uint32_t events) {
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_interrupt_time, now) < 250000) return;
    last_interrupt_time = now;

    if (gpio == BOTAO_A) {
        temp1 = !temp1;
        ventilador_ligado = !ventilador_ligado;
        printf("Ventilador: %s\n", ventilador_ligado ? "LIGADO" : "DESLIGADO");
    }
    if (gpio == BOTAO_B) {
        temp2 = !temp2;
        umidificador_ligado = !umidificador_ligado;
        printf("Umidificador: %s\n", umidificador_ligado ? "LIGADO" : "DESLIGADO");
    }
}

// Inicialização dos pinos e periféricos
void inicia_pinos() {
    // Inicialização básica
    stdio_init_all();
    printf("Iniciando sistema...\n");
    sleep_ms(1000);
    
    // Inicialização do I2C para o display
    printf("Inicializando I2C...\n");
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicialização do display SSD1306
    printf("Inicializando display SSD1306...\n");
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_I2C, I2C_PORT);
    printf("Display SSD1306 inicializado\n");
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando...", 10, 10);
    ssd1306_send_data(&ssd);
    
    // Inicialização da matriz de LEDs WS2812
    printf("Inicializando LEDs WS2812...\n");
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    
    // Inicialização do ADC
    printf("Inicializando ADC...\n");
    adc_init();
    adc_gpio_init(VERTICALY);
    adc_gpio_init(HORIZONTALX);
    adc_set_temp_sensor_enabled(true);
    
    // Inicialização do PWM para buzzers
    printf("Inicializando PWM...\n");
    pwm_init_gpio(buzzer1, PWM_WRAP, PWM_CLK_DIV);
    pwm_set_gpio_level(buzzer1, 0);
    pwm_init_gpio(buzzer2, PWM_WRAP, PWM_CLK_DIV);
    pwm_set_gpio_level(buzzer2, 0);
    
    // Inicialização dos LEDs de status
    printf("Inicializando LEDs de status...\n");
    gpio_init(VERDE); 
    gpio_set_dir(VERDE, GPIO_OUT);
    gpio_put(VERDE, false);
    
    gpio_init(AZUL);
    gpio_set_dir(AZUL, GPIO_OUT);
    gpio_put(AZUL, false);
    
    gpio_init(VERMELHO);
    gpio_set_dir(VERMELHO, GPIO_OUT);
    gpio_put(VERMELHO, false);
    
    // Inicialização dos botões
    printf("Inicializando botões...\n");
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    
    gpio_init(SELEC);
    gpio_set_dir(SELEC, GPIO_IN);
    gpio_pull_up(SELEC);
    
    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    

}

// Função para controlar atuadores
void liga() {
    if (ventilador_ligado == true) {
        gpio_put(AZUL, true);
        if (temperatura >= 31.0f) {
            printf("Ventilador ligado!\n");
            temperatura -= 0.5f;
        } else {
            ventilador_ligado = false;
            gpio_put(AZUL, false);
            temp1 = false;
        }
    } else {
        gpio_put(AZUL, false);
    }

    if (umidificador_ligado == true) {
        gpio_put(VERDE, true);
        if (umidade <= 35.0f) {
            umidade += 0.5f;
            printf("Umidificador ligado!\n");
        } else {
            umidificador_ligado = false;
            gpio_put(VERDE, false);
            temp2 = false;
        }
    } else if (!desumidificador_ligado) {
        gpio_put(VERDE, false);
    }
    
    if (aquecedor_ligado == true) {
        gpio_put(VERMELHO, true);
        if (temperatura <= 25.0f) {
            temperatura += 0.5f;
            printf("Aquecedor ligado!\n");
        } else {
            aquecedor_ligado = false;
            gpio_put(VERMELHO, false);
            temp4 = false;
        }
    } else if (!desumidificador_ligado) {
        gpio_put(VERMELHO, false);
    }
    
    if (desumidificador_ligado == true) {
        gpio_put(VERDE, true);
        gpio_put(VERMELHO, true);
        if (umidade >= 60.0f) {
            umidade -= 0.5f;
            printf("Desumidificador ligado!\n");
        } else {
            desumidificador_ligado = false;
            gpio_put(VERDE, false);
            gpio_put(VERMELHO, false);
            temp3 = false;
        }
    }
}

// Funções MQTT (implementações simplificadas para compilação)
static void pub_request_cb(__unused void *arg, err_t err) {
    if (err != 0) {
        printf("pub_request_cb failed %d\n", err);
    }
}

static const char *full_topic(MQTT_CLIENT_DATA_T *state, const char *name) {
#if MQTT_UNIQUE_TOPIC
    static char full_topic_buf[MQTT_TOPIC_LEN];
    snprintf(full_topic_buf, sizeof(full_topic_buf), "/%s%s", state->mqtt_client_info.client_id, name);
    return full_topic_buf;
#else
    return name;
#endif
}

static void publish_all_states(MQTT_CLIENT_DATA_T *state) {
    char buffer[20];


    snprintf(buffer, sizeof(buffer), "%.2f", temperatura);
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/temperatura/state"), buffer, strlen(buffer), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

    snprintf(buffer, sizeof(buffer), "%.2f", umidade);
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/umidade/state"), buffer, strlen(buffer), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/ventilador/state"), ventilador_ligado ? "ON" : "OFF", ventilador_ligado ? 2 : 3, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/umidificador/state"), umidificador_ligado ? "ON" : "OFF", umidificador_ligado ? 2 : 3, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/aquecedor/state"), aquecedor_ligado ? "ON" : "OFF", aquecedor_ligado ? 2 : 3, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/desumidificador/state"), desumidificador_ligado ? "ON" : "OFF", desumidificador_ligado ? 2 : 3, MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    
    mqtt_publish(state->mqtt_client_inst, full_topic(state, "/status/state"), status_sistema, strlen(status_sistema), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    
    printf("MQTT: Status publicados.\n");
}

static void temperature_worker_fn(async_context_t *context, async_at_time_worker_t *worker) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)worker->user_data;
    publish_all_states(state); 
    async_context_add_at_time_worker_in_ms(context, worker, TEMP_WORKER_TIME_S * 1000);
}


// Implementações simplificadas das funções MQTT restantes
static void sub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        printf("subscribe request failed %d\n", err);
        return;
    }
    state->subscribe_count++;
}

static void unsub_request_cb(void *arg, err_t err) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (err != 0) {
        printf("unsubscribe request failed %d\n", err);
        return;
    }
    state->subscribe_count--;
    if (state->subscribe_count <= 0 && state->stop_client) {
        mqtt_disconnect(state->mqtt_client_inst);
    }
}

static void sub_unsub_topics(MQTT_CLIENT_DATA_T* state, bool sub) {
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;
    
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/ventilador/set"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/umidificador/set"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/aquecedor/set"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
    mqtt_sub_unsub(state->mqtt_client_inst, full_topic(state, "/desumidificador/set"), MQTT_SUBSCRIBE_QOS, cb, state, sub);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
#if MQTT_UNIQUE_TOPIC
    const char *basic_topic = state->topic + strlen(state->mqtt_client_info.client_id) + 1;
#else
    const char *basic_topic = state->topic;
#endif
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    printf("Topic: %s, Message: %s\n", state->topic, state->data);

    bool command_on = (lwip_stricmp(state->data, "ON") == 0 || strcmp(state->data, "1") == 0);

    if (strcmp(basic_topic, "/ventilador/set") == 0) {
        ventilador_ligado = command_on;
        printf("Comando MQTT: Ventilador %s\n", command_on ? "LIGADO" : "DESLIGADO");
    } else if (strcmp(basic_topic, "/umidificador/set") == 0) {
        umidificador_ligado = command_on;
        printf("Comando MQTT: Umidificador %s\n", command_on ? "LIGADO" : "DESLIGADO");
    } else if (strcmp(basic_topic, "/aquecedor/set") == 0) {
        aquecedor_ligado = command_on;
        printf("Comando MQTT: Aquecedor %s\n", command_on ? "LIGADO" : "DESLIGADO");
    } else if (strcmp(basic_topic, "/desumidificador/set") == 0) {
        desumidificador_ligado = command_on;
        printf("Comando MQTT: Desumidificador %s\n", command_on ? "LIGADO" : "DESLIGADO");
    }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    strncpy(state->topic, topic, sizeof(state->topic));
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    MQTT_CLIENT_DATA_T* state = (MQTT_CLIENT_DATA_T*)arg;
    if (status == MQTT_CONNECT_ACCEPTED) {
        state->connect_done = true;
        sub_unsub_topics(state, true);

        if (state->mqtt_client_info.will_topic) {
            mqtt_publish(state->mqtt_client_inst, state->mqtt_client_info.will_topic, "1", 1, MQTT_WILL_QOS, true, pub_request_cb, state);
        }

        temperature_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &temperature_worker, 0);
    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (!state->connect_done) {
            printf("Failed to connect to mqtt server\n");
        }
    }
}

// Continuação do código a partir de onde parou

static void start_client(MQTT_CLIENT_DATA_T *state) {
    const int port = MQTT_PORT;
    
    state->mqtt_client_inst = mqtt_client_new();
    if (state->mqtt_client_inst == NULL) {
        printf("Failed to create mqtt client\n");
        return;
    }

    state->mqtt_client_info.client_id = MQTT_DEVICE_NAME;
    state->mqtt_client_info.client_user = MQTT_USERNAME;
    state->mqtt_client_info.client_pass = MQTT_PASSWORD;
    state->mqtt_client_info.keep_alive = MQTT_KEEP_ALIVE_S;
    state->mqtt_client_info.will_topic = MQTT_WILL_TOPIC;
    state->mqtt_client_info.will_msg = MQTT_WILL_MSG;
    state->mqtt_client_info.will_qos = MQTT_WILL_QOS;
    state->mqtt_client_info.will_retain = 1;

    mqtt_set_inpub_callback(state->mqtt_client_inst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);

    err_t err = mqtt_client_connect(state->mqtt_client_inst, &state->mqtt_server_address, port, mqtt_connection_cb, state, &state->mqtt_client_info);
    if (err != ERR_OK) {
        printf("mqtt_connect return: %d\n", err);
    }
}

static void dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    MQTT_CLIENT_DATA_T *state = (MQTT_CLIENT_DATA_T*)arg;
    if (ipaddr) {
        state->mqtt_server_address = *ipaddr;
        printf("DNS resolved %s -> %s\n", hostname, ip4addr_ntoa(ipaddr));
        start_client(state);
    } else {
        printf("DNS failed to resolve %s\n", hostname);
    }
}

void atualiza_display() {
    char temp_str[20];
    char umid_str[20];
    bool cor = true;
    
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 5, 5, 123, 59, cor, false);
    ssd1306_line(&ssd, 5, 20, 123, 20, cor);
    ssd1306_line(&ssd, 5, 40, 123, 40, cor);
    ssd1306_line(&ssd, 64, 5, 64, 40, cor);
    
    ssd1306_draw_string(&ssd, "Temp.", 10, 8);
    ssd1306_draw_string(&ssd, "Umid.", 67, 8);
    
    sprintf(temp_str, "%.1f", temperatura);
    sprintf(umid_str, "%.1f", umidade);
    ssd1306_draw_string(&ssd, temp_str, 10, 31);
    ssd1306_draw_string(&ssd, umid_str, 67, 31);
    
    ssd1306_draw_string(&ssd, status_sistema, 10, 50);
    
    ssd1306_send_data(&ssd);
}
// Função principal
int main() {
    // Inicialização dos pinos e periféricos
    inicia_pinos();
    
    // Inicialização do Wi-Fi
    printf("Inicializando Wi-Fi...\n");
    if (cyw43_arch_init()) {
        printf("ERRO: Falha ao inicializar Wi-Fi\n");
        return -1;
    }
    
    cyw43_arch_enable_sta_mode();
    
    printf("Conectando ao Wi-Fi '%s'...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("ERRO: Falha ao conectar ao Wi-Fi\n");
        return -1;
    }
    
    printf("Wi-Fi conectado com sucesso!\n");
    printf("IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    
    // Configuração do cliente MQTT
    static MQTT_CLIENT_DATA_T mqtt_state = {0};
    
    // Resolução DNS do servidor MQTT
    printf("Resolvendo DNS do servidor MQTT...\n");
    err_t err = dns_gethostbyname(MQTT_SERVER, &mqtt_state.mqtt_server_address, dns_found, &mqtt_state);
    if (err == ERR_OK) {
        dns_found(MQTT_SERVER, &mqtt_state.mqtt_server_address, &mqtt_state);
    } else if (err != ERR_INPROGRESS) {
        printf("ERRO: Falha na resolução DNS\n");
        return -1;
    }
    
    // Loop principal
    printf("Iniciando loop principal...\n");
    absolute_time_t ultimo_update_display = get_absolute_time();
    
    while (true) {
        // Atualização dos valores com joystick
        adc_incremental(&temperatura, &umidade);
        
        // Análise das condições ambientais
        analise();
        
        // Controle dos atuadores
        liga();
        
        // Atualização do buzzer
        atualiza_buzzer();
        
        // Atualização do display (a cada 100ms)
        if (absolute_time_diff_us(ultimo_update_display, get_absolute_time()) >= 100000) {
            atualiza_display();
            ultimo_update_display = get_absolute_time();
        }
        
        // Processa eventos de rede
        cyw43_arch_poll();
        
        // Pequeno delay para não sobrecarregar o sistema
        sleep_ms(20);
    }
    
    // Cleanup (nunca alcançado no loop infinito)
    cyw43_arch_deinit();
    return 0;
}
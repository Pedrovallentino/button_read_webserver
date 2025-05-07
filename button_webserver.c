// Bibliotecas padrão da Raspberry Pi Pico e da pilha TCP/IP lwIP
#include "pico/stdlib.h"            // GPIO, UART, delays, etc.
#include "hardware/adc.h"           // Conversor analógico-digital
#include "pico/cyw43_arch.h"        // Controle do módulo Wi-Fi integrado (Pico W)
#include <stdio.h>                  // Entrada e saída padrão
#include <string.h>                 // Manipulação de strings
#include <stdlib.h>                 // Funções utilitárias como malloc e free
#include "lwip/pbuf.h"              // Manipulação de buffers de pacotes
#include "lwip/tcp.h"               // Funções do protocolo TCP
#include "lwip/netif.h"             // Interface de rede (ex: IP da placa)

// Definições da rede Wi-Fi
#define WIFI_SSID "REDEWIFI"
#define WIFI_PASSWORD "SENHADAREDE"

// GPIO dos botões
#define BTA 5
#define BTB 6

// Função chamada ao receber dados em uma conexão TCP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        // Fecha a conexão se o buffer de pacote estiver vazio (cliente desconectou)
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Copia o conteúdo recebido para uma string com terminação nula
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';
    printf("Request: %s\n", request);  // Imprime a requisição

    // Lê a temperatura interna do chip usando o ADC
    adc_select_input(4);                 // Entrada 4 é o sensor interno
    uint16_t raw_value = adc_read();     // Lê o valor bruto do ADC
    const float conversion_factor = 3.3f / (1 << 12); // Converte valor ADC para tensão
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;

    // Lê os estados dos botões (pressionado = 0)
    int button_a = gpio_get(BTA) == 0;
    int button_b = gpio_get(BTB) == 0;

    // Gera uma resposta HTML com cores dinâmicas e temperatura
    char html[1024];
    snprintf(html, sizeof(html),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Button and Temperature Read</title>\n"
        "<style>\n"
        "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
        "h1 { font-size: 48px; margin-bottom: 20px; }\n"
        "button { font-size: 32px; padding: 20px 40px; border-radius: 10px; color: white; }\n"
        "#btnA { background-color: %s; }\n"
        "#btnB { background-color: %s; }\n"
        ".temperature { font-size: 36px; margin-top: 30px; color: #333; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Button and Temperature real time monitoring</h1>\n"
        "<button id='btnA'>Button A</button>\n"
        "<button id='btnB'>Button B</button>\n"
        "<p class='temperature'>Temperatura: %.2f &deg;C</p>\n"
        "</body>\n"
        "</html>\n",
        button_a ? "green" : "red",       // Botão A verde se pressionado
        button_b ? "green" : "red",       // Botão B verde se pressionado
        temperature);

    // Envia a resposta HTML pela conexão TCP
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);  // Garante envio imediato

    // Libera memória e buffer de pacotes
    free(request);
    pbuf_free(p);

    return ERR_OK;
}

// Função chamada quando uma nova conexão é aceita no servidor TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv); // Define a função de recebimento de dados
    return ERR_OK;
}

int main() {
    stdio_init_all(); // Inicializa UART e printf

    // Configura os GPIOs dos botões como entrada com pull-up
    gpio_init(BTA); 
    gpio_set_dir(BTA, GPIO_IN); 
    gpio_pull_up(BTA);

    gpio_init(BTB); 
    gpio_set_dir(BTB, GPIO_IN); 
    gpio_pull_up(BTB);

    // Inicializa o Wi-Fi
    while (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();  // Coloca o Wi-Fi no modo estação (cliente)

    // Tenta conectar ao Wi-Fi
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    printf("Conectado ao Wi-Fi\n");

    // Exibe o IP obtido
    if (netif_default) {
        printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Cria e configura o servidor TCP na porta 80
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    server = tcp_listen(server);                   // Coloca em modo escuta
    tcp_accept(server, tcp_server_accept);         // Define a função de aceitação
    printf("Servidor ouvindo na porta 80\n");

    // Inicializa o ADC para leitura da temperatura
    adc_init();
    adc_set_temp_sensor_enabled(true);

    // Loop principal
    while (true) {
        cyw43_arch_poll();  // Atualiza eventos do Wi-Fi
    }

    cyw43_arch_deinit();  // Encerra o módulo Wi-Fi (nunca será alcançado)
    return 0;
}

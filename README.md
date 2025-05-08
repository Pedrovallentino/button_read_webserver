## 🧩 1. Inclusão de bibliotecas e definições

**Essas bibliotecas controlam GPIO, ADC, Wi-Fi, e protocolo TCP/IP:**
```c
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
```

**Definições da rede e dos pinos dos botões:**
```c
#define WIFI_SSID "REDEWIFI"
#define WIFI_PASSWORD "SENHADAREDE"
#define BTA 5
#define BTB 6
```

## 📶 2. Conexão ao Wi-Fi


**Inicializa a UART e configura os GPIOs como entradas com pull-up:**
```c
stdio_init_all();
gpio_init(BTA); gpio_set_dir(BTA, GPIO_IN); gpio_pull_up(BTA);
gpio_init(BTB); gpio_set_dir(BTB, GPIO_IN); gpio_pull_up(BTB);
Inicializa e conecta ao Wi-Fi:


while (cyw43_arch_init()) {
    printf("Falha ao inicializar Wi-Fi\n");
    sleep_ms(100);
    return -1;
}

cyw43_arch_enable_sta_mode();

printf("Conectando ao Wi-Fi...\n");
while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
    printf("Falha ao conectar ao Wi-Fi\n");
    sleep_ms(100);
    return -1;
}
```

**Após conectar, imprime o IP:**
```c
if (netif_default) {
    printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
}
```

## 🌐 3. Criação do servidor TCP


**Cria um servidor escutando na porta 80:**
```c
struct tcp_pcb *server = tcp_new();
if (!server) {
    printf("Falha ao criar servidor TCP\n");
    return -1;
}

if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
    printf("Falha ao associar servidor TCP à porta 80\n");
    return -1;
}

server = tcp_listen(server);
tcp_accept(server, tcp_server_accept);
printf("Servidor ouvindo na porta 80\n");
```

## 🔁 4. Tratamento de requisição HTTP


**Quando o navegador acessa o IP da placa, essa função é chamada:**

```c
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
```
**Verifica se o cliente desconectou:**

```c
if (!p) {
    tcp_close(tpcb);
    tcp_recv(tpcb, NULL);
    return ERR_OK;
}
```
**Copia os dados recebidos (como a string "GET /"):**

```c
char *request = (char *)malloc(p->len + 1);
memcpy(request, p->payload, p->len);
request[p->len] = '\0';
printf("Request: %s\n", request);
```

## 🌡️ 5. Leitura da temperatura interna


**Seleciona o sensor de temperatura (canal 4) e lê o ADC:**

```c
adc_select_input(4);
uint16_t raw_value = adc_read();
const float conversion_factor = 3.3f / (1 << 12);
float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
```

## 🟢🔴 6. Leitura dos botões


**Estado dos botões (pressionado = 0):**

```c
int button_a = gpio_get(BTA) == 0;
int button_b = gpio_get(BTB) == 0;
```

🧾 7. Geração da página HTML


**Cria uma resposta HTML com as cores dos botões e a temperatura:**

```c
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
    button_a ? "green" : "red",
    button_b ? "green" : "red",
    temperature);
```
    
## 📤 8. Envio da resposta ao navegador


**Envia a página gerada de volta ao cliente:**

```c
tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
tcp_output(tpcb);
free(request);
pbuf_free(p);
```

## 🔄 9. Loop principal


**Mantém a execução do servidor atualizando os eventos do Wi-Fi:**

```c
while (true) {
    cyw43_arch_poll();


(...)
}

```

**Loop infinito para manter o servidor ativo.**

**cyw43_arch_poll() atualiza a pilha de rede e trata eventos de Wi-Fi e TCP.**

- **Inicia UART e GPIOs dos botões.**

- **Inicializa o Wi-Fi e conecta à rede.**

- **Cria um servidor HTTP na porta 80.**

- **Ativa o sensor de temperatura.**

- **Entra num loop infinito para manter o sistema ativo e funcional.**

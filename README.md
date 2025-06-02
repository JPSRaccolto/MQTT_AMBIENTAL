![image](https://github.com/user-attachments/assets/f2a5c9b8-6208-4723-8f46-1d74be421827)

# 👥 Projeto: Sistema de Monitoramento Ambiental com MQTT e Raspberry Pi Pico W

## 📑 Sumário
- [🎯 Objetivos](#-objetivos)
- [📋 Descrição do Projeto](#-descrição-do-projeto)
- [⚙️ Funcionalidades Implementadas](#️-funcionalidades-implementadas)
- [🛠️ Requisitos do Projeto](#️-requisitos-do-projeto)
- [📂 Estrutura do Código](#-estrutura-do-código)
- [🖥️ Como Compilar](#️-como-compilar)
- [🧑‍💻 Autor](#-autor)
- [📝 Descrição Detalhada](#-descrição-detalhada)
- [🤝 Contribuições](#-contribuições)
- [📽️ Demonstração em Vídeo](#️-demonstração-em-vídeo)
- [💡 Considerações Finais](#-considerações-finais)

## 🎯 Objetivos
- Desenvolver um sistema de monitoramento ambiental utilizando o Raspberry Pi Pico W.
- Integrar atuadores (LEDs, Buzzer, Matriz de LEDs) e sensores simulados (Joystick).
- Implementar comunicação via Wi-Fi e MQTT para controle e monitoramento remoto.
- Apresentar dados em tempo real através de um display OLED e alertas visuais/sonoros.

## 📋 Descrição do Projeto
Este projeto transforma o Raspberry Pi Pico W em uma central de monitoramento ambiental. Ele simula a leitura de temperatura e umidade através de um joystick analógico e exibe esses dados em um display OLED. O sistema se conecta a um broker MQTT para enviar os dados e receber comandos de controle para atuadores como ventilador, aquecedor, umidificador e desumidificador, cujos estados são representados por LEDs. Alertas críticos de temperatura e umidade acionam um buzzer e padrões visuais em uma matriz de LEDs WS2812.

## ⚙️ Funcionalidades Implementadas
1.  **Monitoramento e Controle via MQTT:**
    * Conecta-se a uma rede Wi-Fi e a um servidor MQTT especificados.
    * Publica o estado de temperatura, umidade e atuadores nos tópicos `/temperatura/state`, `/umidade/state`, etc..
    * Subscreve-se a tópicos de comando (ex: `/ventilador/set`) para ligar/desligar atuadores remotamente.
    * Ajuste manual dos valores de temperatura e umidade através de um joystick.

2.  **Sistema de Alertas Visuais:**
    * **LEDs de Status:** LEDs individuais (Vermelho, Verde, Azul) indicam o estado dos atuadores.
    * **Matriz de LED WS2812:** Exibe padrões visuais distintos para sinalizar condições críticas como "Temperatura Elevada", "Umidade Crítica", etc..
    * **Display OLED:** Exibe em tempo real a temperatura, umidade e o status geral do sistema (`Sistema OK`, `Temp. Elevada`).

3.  **Sistema de Alertas Sonoros:**
    * Um buzzer é ativado com um som intermitente quando as leituras de temperatura ou umidade ultrapassam os limites de segurança predefinidos.

4.  **Controle Local:**
    * Botões físicos permitem ao usuário ligar e desligar manualmente o ventilador e o umidificador.

## 🛠️ Requisitos do Projeto
- **Pico SDK Libraries:** `pico_stdlib`, `pico_cyw43_arch_lwip_threadsafe_background`.
- **Comunicação:** `hardware_i2c` para o display OLED e `pico_lwip_mqtt` para comunicação MQTT.
- **Hardware Control:** `hardware_adc` para o joystick, `hardware_pwm` para o buzzer.
- **Segurança:** `pico_mbedtls` para conexões TLS (Transport Layer Security).
- **PIO (Programmable I/O):** Utilizado para o controle preciso da matriz de LEDs WS2812.

## 📂 Estrutura do Repositório
├── projeto_mqtt.c        # Código principal da aplicação, lógica de controle e MQTT
├── CMakeLists.txt        # Arquivo de configuração de compilação do projeto
└── lib/
├── ssd1306.c         # Driver para o controle do display OLED
├── ssd1306.h         # Header do driver do display
├── font.h            # Dados da fonte para exibição de caracteres no display
├── ws2812.pio.h      # Programa PIO para a matriz de LEDs WS2812
└── ...               # Demais arquivos de configuração de rede (lwipopts, mbedtls)


## 🖥️ Como Compilar
1.  Clone o repositório:
    ```bash
    git clone https://github.com/JPSRaccolto/MQTT_AMBIENTAL.git
    ```
2.  Navegue até o diretório do projeto:
    ```bash
    cd MQTT_AMBIENTAL/projeto_mqtt
    ```
3.  Configure seu ambiente de desenvolvimento para o Raspberry Pi Pico (RP2040).
4.  Compile o projeto. Um arquivo `projeto_mqtt.uf2` será gerado no diretório `build`.
5.  Pressione e segure o botão BOOTSEL na sua Pico W e conecte-a ao computador.
6.  Arraste o arquivo `.uf2` gerado para o drive `RPI-RP2` que aparece no seu computador.

## 🧑‍💻 Autor
**João Pedro Soares Raccolto**

## 📝 Descrição Detalhada
Este é um sistema de Internet das Coisas (IoT) para monitoramento ambiental construído sobre a plataforma Raspberry Pi Pico W. Ele integra múltiplos periféricos para fornecer uma solução completa de sensoriamento (simulado) e atuação. Os dados de temperatura e umidade, ajustáveis localmente por um joystick, são exibidos em um display OLED e publicados em um broker MQTT. Isso permite que os dados sejam visualizados e que o sistema seja controlado remotamente através de qualquer cliente MQTT. O projeto também inclui um sistema de feedback local robusto com LEDs de status, uma matriz de LEDs para alertas visuais detalhados e um buzzer para notificações sonoras, garantindo que o usuário seja informado sobre as condições ambientais críticas tanto local quanto remotamente.

## 🤝 Contribuições
Este projeto foi desenvolvido por **João Pedro Soares Raccolto**.
Contribuições são bem-vindas! Siga os passos abaixo para contribuir:

1.  Fork este repositório.
2.  Crie uma nova branch:
    ```bash
    git checkout -b minha-feature
    ```
3.  Faça suas modificações e commit:
    ```bash
    git commit -m 'Minha nova feature'
    ```
4.  Envie suas alterações:
    ```bash
    git push origin minha-feature
    ```
5.  Abra um Pull Request.

## 📽️ Demonstração em Vídeo
- O vídeo de demonstração do projeto pode ser visualizado aqui: [Link para o vídeo](https://drive.google.com/file/d/1CtTBlMcizYix0AwHNDmm700G4hOCUKI9/view?usp=sharing)

## 💡 Considerações Finais
Este projeto serve como uma excelente base para o desenvolvimento de dispositivos IoT mais complexos. A arquitetura pode ser expandida para incluir sensores reais (como BME280 ou DHT22). O uso de MQTT o torna modular e facilmente integrável a sistemas de automação residencial existentes. A implementação atual utiliza um loop de polling para gerenciar tarefas, o que é eficiente para esta aplicação, mas poderia ser migrado para um sistema operacional de tempo real (RTOS) para projetos com maior complexidade de tarefas concorrentes.

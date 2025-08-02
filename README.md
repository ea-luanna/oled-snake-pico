# Snake Game para BitDogLab

## Sumário

- [1. Descrição](#1-descrição)
- [2. Funcionalidades](#2-funcionalidades)
- [3. Requisitos de Hardware](#3-requisitos-de-hardware)
- [4. Configuração e Execução](#4-configuração-e-execução)
  - [4.1 Requisitos de Software](#41-requisitos-de-software)
  - [4.2 Etapas de Execução](#42-etapas-de-execução)
- [5. Contato e Contribuições](#5-contato-e-contribuições)

---


## 1. Descrição

Este projeto apresenta o desenvolvimento do jogo Snake (popularmente conhecido como “Jogo da Cobrinha”) utilizando a linguagem C, com execução voltada para a placa **BitDogLab**, que integra o microcontrolador **Raspberry Pi Pico W**.

A aplicação proposta consiste na implementação do jogo Snake com controle por joystick analógico e exibição gráfica no display OLED.

---

## 2. Funcionalidades

O projeto tem as funcionalidades:

- **Controle por Joystick Analógico**  
  O movimento da cobrinha é controlado por um joystick analógico embutido na BitDogLab. Os sinais analógicos dos eixos X e Y são lidos por canais ADC, permitindo o controle direcional em tempo real.

- **Exibição Gráfica via Display OLED (128x64)**  
  O jogo é exibido em um display OLED monocromático de 0.96” com resolução de 128x64 pixels, utilizando o protocolo I²C. A cobrinha, comida e demais elementos são representados com blocos simples (pixel art).

- **Geração Aleatória de Comida**  
  A cada coleta de comida, uma nova aparece em posição aleatória válida, alinhada ao grid da tela.

- **Crescimento Progressivo da Cobrinha**  
  A cobrinha aumenta de tamanho conforme consome a comida, com atualização contínua do vetor de posições do corpo.

- **Detecção de Colisões (Game Over)**  
  O sistema detecta colisões contra bordas da tela e contra o próprio corpo da cobrinha. Em caso de colisão, a partida é encerrada e exibida a mensagem "GAME OVER", em seguida retorna ao menu inicial.

---

## 3. Requisitos de Hardware

Para a execução adequada do projeto, é necessário os seguintes itens:
- **Placa BitDogLab**
- **Alimentação via USB** ou bateria Li-Ion 3.7V integrada à BitDogLab

### Alternativamente:

| Componente        | Especificações BitDogLab                                  |
|-------------------|------------------------------------------------------------|
| Microcontrolador  | Raspberry Pi Pico W (Dual-core ARM Cortex-M0+, 133 MHz)    |
| Display OLED      | 0.96" - 128x64 pixels via I²C (SSD1306)                    |
| Entrada de Dados  | Joystick analógico (Eixos X e Y conectados aos ADCs)       |
| Outros Recursos   | Botões tácteis, buzzer, alimentação por bateria, GPIOs     |

---

## 4. Configuração e Execução

### 4.1 Requisitos de Software

- Visual Studio Code com a extensão oficial do Raspberry Pi Pico
- Raspberry Pi Pico SDK instalado e configurado
- Ambiente de build CMake corretamente configurado
- Placa BitDogLab conectada via USB

### 4.2 Etapas de Execução

1. Crie um novo projeto no VS Code utilizando a extensão oficial do Raspberry Pi Pico, com o suporte a I²C ativado.
2. Copie os seguintes arquivos para a pasta do projeto:
   - `main.c` (lógica do jogo)
   - Driver leve para o display
   - `CMakeLists.txt` (arquivo de build do projeto)
3. Compile o projeto clicando no botão **Build** da extensão.
4. Pressione e segure o botão **BOOTSEL** na BitDogLab, conecte-a ao computador via cabo USB e solte o botão.
5. Um novo dispositivo aparecerá como unidade de armazenamento. Copie o arquivo `.uf2` da pasta `build` para essa unidade.
6. O jogo será executado automaticamente após o carregamento.

---

## 5. Contato e Contribuições

Para dúvidas, sugestões ou contribuições, entre em contato:

Desenvolvido por **Lu ♡**  
Email: [contatoludev@gmail.com](mailto:contatoludev@gmail.com)  
GitHub: [@ea-luanna](https://github.com/ea-luanna)

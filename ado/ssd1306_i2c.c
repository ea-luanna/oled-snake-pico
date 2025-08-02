#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h> // Para toupper
#include "pico/stdlib.h"
#include "pico/binary_info.h" // Pode ser removido se não usar bi_decl
#include "hardware/i2c.h"
#include "ssd1306_font.h"
#include "ssd1306_i2c.h"

// Macro para contar elementos em um array (útil para ssd1306_init)
#ifndef count_of
#define count_of(a) (sizeof(a) / sizeof((a)[0]))
#endif

// Global display buffer
uint8_t ssd1306_buffer[ssd1306_buffer_length];

// Global variables for display configuration
static i2c_inst_t *ssd1306_i2c_port = NULL;
static uint8_t ssd1306_i2c_addr = 0x3C;

// Calcular quanto do buffer será destinado à área de renderização
void calculate_render_area_buffer_length(struct render_area *area)
{
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

// Processo de escrita do i2c espera um byte de controle, seguido por dados
void ssd1306_send_command(uint8_t command)
{
    uint8_t buffer[2] = {0x80, command}; // 0x80 indica que é um comando
    i2c_write_blocking(ssd1306_i2c_port, ssd1306_i2c_addr, buffer, 2, false);
}

// Envia uma lista de comandos ao hardware
void ssd1306_send_command_list(uint8_t *commands, int number)
{
    for (int i = 0; i < number; i++)
    {
        ssd1306_send_command(commands[i]);
    }
}

// Copia buffer de referência num novo buffer, a fim de adicionar o byte de controle desde o início
void ssd1306_send_buffer(uint8_t ssd[], int buffer_length)
{
    // Aloca um buffer temporário maior para incluir o byte de controle (0x40 para dados)
    uint8_t *temp_buffer = (uint8_t *)malloc(buffer_length + 1);
    if (temp_buffer == NULL)
    {
        // Tratar erro de alocação de memória
        return;
    }

    temp_buffer[0] = 0x40;                       // 0x40 indica que os bytes seguintes são dados
    memcpy(temp_buffer + 1, ssd, buffer_length); // Copia os dados do buffer original

    i2c_write_blocking(ssd1306_i2c_port, ssd1306_i2c_addr, temp_buffer, buffer_length + 1, false);

    free(temp_buffer); // Libera a memória alocada
}

// Cria a lista de comandos (com base nos endereços definidos em ssd1306_i2c.h) para a inicialização do display
void ssd1306_init(i2c_inst_t *i2c, uint8_t address, uint8_t width, uint8_t height)
{
    // Store global configuration
    ssd1306_i2c_port = i2c;
    ssd1306_i2c_addr = address;
    
    // Clear the buffer
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
    
    uint8_t commands[] = {
        ssd1306_set_display | 0x00,                           // Desliga o display
        ssd1306_set_memory_mode, 0x00,                        // Modo de endereçamento horizontal
        ssd1306_set_display_start_line | 0x00,                // Linha de início do display (0x40 + 0)
        ssd1306_set_segment_remap | 0x01,                     // Remapeia colunas (0xA0 | 0x01 para A1)
        ssd1306_set_mux_ratio, (uint8_t)(height - 1),         // Multiplex ratio (altura - 1)
        ssd1306_set_common_output_direction | 0x08,           // COM scan direction (0xC0 para normal, 0xC8 para invertido)
        ssd1306_set_display_offset, 0x00,                     // Offset do display (0x00)
        ssd1306_set_common_pin_configuration,

#if (ssd1306_height == 32)
        0x02, // Configuração de pinos COM para 32 pixels de altura
#elif (ssd1306_height == 64)
        0x12, // Configuração de pinos COM para 64 pixels de altura
#else
        0x12, // Padrão para 64 pixels
#endif
        ssd1306_set_display_clock_divide_ratio, 0x80, // Divide ratio/oscillator frequency
        ssd1306_set_precharge, 0xF1,                  // Pre-charge period
        ssd1306_set_vcomh_deselect_level, 0x30,       // VCOMH deselect level
        ssd1306_set_contrast, 0xFF,                   // Contraste (0x00 a 0xFF)
        ssd1306_set_entire_on,                        // Display all pixels ON (0xA4 para seguir RAM, 0xA5 para todos ON)
        ssd1306_set_normal_display,                   // Normal display (0xA6 para normal, 0xA7 para invertido)
        ssd1306_set_charge_pump, 0x14,                // Charge pump setting (0x14 para habilitar, 0x10 para desabilitar)
        ssd1306_set_scroll | 0x00,                    // Desabilita o scroll
        ssd1306_set_display | 0x01,                   // Liga o display (0xAF)
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

// Cria a lista de comandos para configurar o scrolling
void ssd1306_scroll(bool set)
{
    uint8_t commands[] = {
        ssd1306_set_horizontal_scroll | 0x00, // Scroll horizontal à direita (0x26)
        0x00,                                 // Dummy byte
        0x00,                                 // Start page address
        0x00,                                 // Time interval (frames)
        0x03,                                 // End page address
        0x00,                                 // Dummy byte
        0xFF,                                 // Dummy byte
        ssd1306_set_scroll | (set ? 0x01 : 0) // Ativa/desativa o scroll (0x2F para desativar, 0x2E para ativar)
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

// Atualiza uma parte do display com uma área de renderização
void render_on_display(uint8_t *ssd, struct render_area *area)
{
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page};

    ssd1306_send_command_list(commands, count_of(commands));
    ssd1306_send_buffer(ssd, area->buffer_length);
}

// Determina o pixel a ser aceso (no display) de acordo com a coordenada fornecida
void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set)
{
    // Verifica se as coordenadas estão dentro dos limites do display
    if (x < 0 || x >= ssd1306_width || y < 0 || y >= ssd1306_height)
    {
        return; // Fora dos limites, não faz nada
    }

    const int bytes_per_row = ssd1306_width; // Cada coluna tem 1 byte (8 pixels verticais)

    // Calcula o índice do byte no buffer
    // (y / 8) determina a "página" (linha de 8 pixels)
    // x determina a coluna
    int byte_idx = (y / 8) * bytes_per_row + x;
    uint8_t byte = ssd[byte_idx];

    // Calcula a posição do bit dentro do byte (y % 8)
    if (set)
    {
        byte |= (1 << (y % 8)); // Liga o bit correspondente ao pixel
    }
    else
    {
        byte &= ~(1 << (y % 8)); // Desliga o bit correspondente ao pixel
    }

    ssd[byte_idx] = byte; // Atualiza o byte no buffer
}

// Algoritmo de Bresenham básico para desenhar uma linha
void ssd1306_draw_line(uint8_t *ssd, int x_0, int y_0, int x_1, int y_1, bool set)
{
    int dx = abs(x_1 - x_0);     // Deslocamento absoluto em X
    int dy = -abs(y_1 - y_0);    // Deslocamento absoluto em Y (negativo para o algoritmo)
    int sx = x_0 < x_1 ? 1 : -1; // Direção de avanço em X
    int sy = y_0 < y_1 ? 1 : -1; // Direção de avanço em Y
    int error = dx + dy;         // Erro acumulado inicial
    int error_2;

    while (true)
    {
        ssd1306_set_pixel(ssd, x_0, y_0, set); // Acende/apaga pixel no ponto atual

        if (x_0 == x_1 && y_0 == y_1)
        {
            break; // Verifica se o ponto final foi alcançado
        }

        error_2 = 2 * error; // Ajusta o erro acumulado

        if (error_2 >= dy)
        { // Se o erro for maior ou igual ao deslocamento em Y
            error += dy;
            x_0 += sx; // Avança na direção X
        }
        if (error_2 <= dx)
        { // Se o erro for menor ou igual ao deslocamento em X
            error += dx;
            y_0 += sy; // Avança na direção Y
        }
    }
}

// Adquire o índice do caractere na matriz de fontes
inline int ssd1306_get_font(uint8_t character)
{
    if (character >= 'A' && character <= 'Z')
    {
        return character - 'A' + 1; // 'A' está no índice 1
    }
    else if (character >= '0' && character <= '9')
    {
        return character - '0' + 27; // '0' está no índice 27
    }
    else if (character == ' ')
    {             // Espaço
        return 0; // Espaço está no índice 0
    }
    else
        return 0; // Caractere não reconhecido, retorna espaço
}

// Desenha um único caractere no display
void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character)
{
    // Verifica se o caractere está fora dos limites visíveis
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8 || x < -7 || y < -7)
    {
        return;
    }

    // A coordenada Y é dividida por 8 porque cada "página" tem 8 pixels de altura
    int page_y = y / 8;

    character = toupper(character);             // Converte para maiúscula para usar a fonte
    int font_idx = ssd1306_get_font(character); // Obtém o índice da fonte

    // Calcula o índice inicial no buffer do frame
    // (page_y * ssd1306_width) para ir para a página correta
    // + x para ir para a coluna correta
    int fb_idx = page_y * ssd1306_width + x;

    // Copia os 8 bytes do caractere da fonte para o buffer do display
    for (int i = 0; i < 8; i++)
    {
        // Verifica se a escrita está dentro dos limites do buffer
        if (x + i >= 0 && x + i < ssd1306_width && page_y >= 0 && page_y < ssd1306_n_pages)
        {
            ssd[fb_idx + i] = font[font_idx * 8 + i];
        }
    }
}

// Desenha uma string, chamando a função de desenhar caractere várias vezes
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string)
{
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8)
    {
        return;
    }

    while (*string)
    {
        ssd1306_draw_char(ssd, x, y, *string++);
        x += 8; // Avança 8 pixels para o próximo caractere
    }
}

// Comando de configuração com base na estrutura ssd1306_t (para bitmap)
void ssd1306_command(ssd1306_t *ssd, uint8_t command)
{
    ssd->port_buffer[1] = command;
    i2c_write_blocking(
        ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false);
}

// Função de configuração do display para o caso do bitmap
void ssd1306_config(ssd1306_t *ssd)
{
    ssd1306_command(ssd, ssd1306_set_display | 0x00);
    ssd1306_command(ssd, ssd1306_set_memory_mode);
    ssd1306_command(ssd, 0x01); // Vertical addressing mode
    ssd1306_command(ssd, ssd1306_set_display_start_line | 0x00);
    ssd1306_command(ssd, ssd1306_set_segment_remap | 0x01);
    ssd1306_command(ssd, ssd1306_set_mux_ratio);
    ssd1306_command(ssd, ssd1306_height - 1);
    ssd1306_command(ssd, ssd1306_set_common_output_direction | 0x08);
    ssd1306_command(ssd, ssd1306_set_display_offset);
    ssd1306_command(ssd, 0x00);
    ssd1306_command(ssd, ssd1306_set_common_pin_configuration);
    ssd1306_command(ssd, 0x12); // For 128x64 display
    ssd1306_command(ssd, ssd1306_set_display_clock_divide_ratio);
    ssd1306_command(ssd, 0x80);
    ssd1306_command(ssd, ssd1306_set_precharge);
    ssd1306_command(ssd, 0xF1);
    ssd1306_command(ssd, ssd1306_set_vcomh_deselect_level);
    ssd1306_command(ssd, 0x30);
    ssd1306_command(ssd, ssd1306_set_contrast);
    ssd1306_command(ssd, 0xFF);
    ssd1306_command(ssd, ssd1306_set_entire_on);
    ssd1306_command(ssd, ssd1306_set_normal_display);
    ssd1306_command(ssd, ssd1306_set_charge_pump);
    ssd1306_command(ssd, 0x14);
    ssd1306_command(ssd, ssd1306_set_display | 0x01);
}

// Inicializa o display para o caso de exibição de bitmap
void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c)
{
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8U;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->bufsize = ssd->pages * ssd->width + 1; // +1 para o byte de controle
    ssd->ram_buffer = (uint8_t *)calloc(ssd->bufsize, sizeof(uint8_t));
    if (ssd->ram_buffer == NULL)
    {
        // Tratar erro de alocação de memória
        return;
    }
    ssd->ram_buffer[0] = 0x40;  // Byte de controle para dados
    ssd->port_buffer[0] = 0x80; // Byte de controle para comandos
}

// Envia os dados ao display (para bitmap)
void ssd1306_send_data(ssd1306_t *ssd)
{
    ssd1306_command(ssd, ssd1306_set_column_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->width - 1);
    ssd1306_command(ssd, ssd1306_set_page_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->pages - 1);
    i2c_write_blocking(
        ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false);
}

// Desenha o bitmap (a ser fornecido em display_oled.c) no display
void ssd1306_draw_bitmap(ssd1306_t *ssd, const uint8_t *bitmap)
{
    // Copia o bitmap para o buffer do display (começando do índice 1, pois 0 é o byte de controle)
    for (int i = 0; i < ssd->bufsize - 1; i++)
    {
        ssd->ram_buffer[i + 1] = bitmap[i];
    }
    ssd1306_send_data(ssd); // Envia o buffer para o display
}

// Missing functions needed by snake_game.c

// Clear the display buffer
void ssd1306_clear()
{
    memset(ssd1306_buffer, 0, ssd1306_buffer_length);
}

// Send the buffer to the display
void ssd1306_show()
{
    // Set column address range
    ssd1306_send_command(ssd1306_set_column_address);
    ssd1306_send_command(0);
    ssd1306_send_command(ssd1306_width - 1);
    
    // Set page address range
    ssd1306_send_command(ssd1306_set_page_address);
    ssd1306_send_command(0);
    ssd1306_send_command(ssd1306_n_pages - 1);
    
    // Send the buffer
    ssd1306_send_buffer(ssd1306_buffer, ssd1306_buffer_length);
}

// Draw a filled rectangle
void ssd1306_draw_filled_rect(int x, int y, int width, int height, int color)
{
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            ssd1306_set_pixel(ssd1306_buffer, x + i, y + j, color);
        }
    }
}

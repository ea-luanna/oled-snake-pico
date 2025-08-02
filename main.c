//Este projejo, no momento, está imcompleto e sujeito a alterações. 
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ado/ssd1306.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define SEGMENT_SIZE 4 


#define SEGMENTS_WIDE (SCREEN_WIDTH / SEGMENT_SIZE) 
#define SEGMENTS_HIGH (SCREEN_HEIGHT / SEGMENT_SIZE)
#define OLED_SDA 14
#define OLED_SCL 15

#define JOY_X 26
#define JOY_Y 27
#define JOY_BUTTON 22


#define PYTHON_LOW_THRESHOLD 1024
#define PYTHON_HIGH_THRESHOLD 3072



typedef struct {
    int x;
    int y;
} Segment;



#define MAX_LENGTH 128 
Segment snake[MAX_LENGTH];
int snake_length; 
int dx = 1, dy = 0; 
int score = 0;
int high_score = 0;

Segment food;
bool game_over = false; 
bool in_menu = true;
int menu_selection = 0; // 0: game, 1: record

void draw_char(int x, int y, char c) {
    ssd1306_draw_char(ssd1306_buffer, x, y, c);
}

void draw_string(int x, int y, const char *str) {
    while (*str) {
        draw_char(x, y, *str++);
        x += 8;
    }
}

void draw_walls() {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        ssd1306_set_pixel(ssd1306_buffer, 0, y, 1);
        ssd1306_set_pixel(ssd1306_buffer, SCREEN_WIDTH - 1, y, 1);
    }
    
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        ssd1306_set_pixel(ssd1306_buffer, x, 0, 1);
        ssd1306_set_pixel(ssd1306_buffer, x, SCREEN_HEIGHT - 1, 1);
    }
}

void draw_score() {
    char score_str[16];
    sprintf(score_str, "Score: %d", score);
    draw_string(5, 3, score_str);
}

void draw_menu() {
    ssd1306_clear();
    
    draw_string(SCREEN_WIDTH/2 - 30, 10, "SNAKE GAME");
    
    int menu_y = 30;
    int menu_x = SCREEN_WIDTH/2 - 20;
    
    // Opção Game
    if (menu_selection == 0) {
        draw_char(menu_x - 10, menu_y, 'X'); // X para seleção
    } else {
        draw_char(menu_x - 10, menu_y, ' '); // Espaço quando não selecionado
    }
    draw_string(menu_x, menu_y, "game");
    
    menu_y += 10;
    
    // Opção Record
    if (menu_selection == 1) {
        draw_char(menu_x - 10, menu_y, 'X'); // X para seleção
    } else {
        draw_char(menu_x - 10, menu_y, ' '); // Espaço quando não selecionado
    }
    draw_string(menu_x, menu_y, "record");
    
    ssd1306_show();
}

void draw_segment(Segment s, int color) {
    ssd1306_draw_filled_rect(s.x * SEGMENT_SIZE, s.y * SEGMENT_SIZE, SEGMENT_SIZE, SEGMENT_SIZE, color);
}

void place_food() {
    bool valid = false;
    while (!valid) {
        food.x = 1 + rand() % (SEGMENTS_WIDE - 2);
        food.y = 1 + rand() % (SEGMENTS_HIGH - 2);
        valid = true;
        for (int i = 0; i < snake_length; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = false;
                break;
            }
        }
    }
}

void reset_game() {
    snake_length = 3;
    dx = 1;
    dy = 0;
    score = 0;
    for (int i = 0; i < snake_length; i++) {
        snake[i].x = 5 - i;
        snake[i].y = 5;
    }
    place_food();
    game_over = false;
}

void update_joystick(uint16_t vrx_raw, uint16_t vry_raw) {
    static bool joystick_pressed = false;
    static bool button_pressed = false;
    
    bool joystick_up = (vrx_raw > PYTHON_HIGH_THRESHOLD);
    bool joystick_down = (vrx_raw < PYTHON_LOW_THRESHOLD);
    bool joystick_left = (vry_raw < PYTHON_LOW_THRESHOLD);
    bool joystick_right = (vry_raw > PYTHON_HIGH_THRESHOLD);
    bool current_button = !gpio_get(JOY_BUTTON);

    if (in_menu) {
        if (joystick_up && !joystick_pressed) {
            menu_selection = 0;
            joystick_pressed = true;
        } 
        else if (joystick_down && !joystick_pressed) {
            menu_selection = 1;
            joystick_pressed = true;
        }
        else if (!joystick_up && !joystick_down) {
            joystick_pressed = false;
        }

        if (current_button && !button_pressed) {
            button_pressed = true;
            if (menu_selection == 0) {
                in_menu = false;
                reset_game();
            } else {
                ssd1306_clear();
                draw_string(SCREEN_WIDTH/2 - 25, 20, "RECORD");
                char high_score_str[32];
                sprintf(high_score_str, "Melhor: %d", high_score);
                draw_string(SCREEN_WIDTH/2 - 30, 40, high_score_str);
                ssd1306_show();
                sleep_ms(3000);
            }
        } else if (!current_button) {
            button_pressed = false;
        }
    } else {
        if (joystick_up && dy != 1) {
            dx = 0;
            dy = -1;
        } else if (joystick_down && dy != -1) {
            dx = 0;
            dy = 1;
        } else if (joystick_left && dx != 1) {
            dx = -1;
            dy = 0;
        } else if (joystick_right && dx != -1) {
            dx = 1;
            dy = 0;
        }
    }
}

bool check_collision(int x, int y) {
    if (x <= 0 || y <= 0 || x >= SEGMENTS_WIDE - 1 || y >= SEGMENTS_HIGH - 1)
        return true;
    
    for (int i = 1; i < snake_length; i++) {
        if (snake[i].x == x && snake[i].y == y)
            return true;
    }
    return false;
}

void move_snake() {
    Segment new_head = {snake[0].x + dx, snake[0].y + dy};

    if (check_collision(new_head.x, new_head.y)) {
        if (score > high_score) {
            high_score = score;
        }
        game_over = true;
        return;
    }

    for (int i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;

    if (new_head.x == food.x && new_head.y == food.y) {
        if (snake_length < MAX_LENGTH) {
            snake[snake_length] = snake[snake_length - 1];
            snake_length++;
            score += 1;
        }
        place_food();
    }
}

void draw_game() {
    ssd1306_clear();
    draw_walls();
    draw_score();
    draw_segment(food, 1);
    for (int i = 0; i < snake_length; i++)
        draw_segment(snake[i], 1);
    ssd1306_show();
}

void draw_game_over() {
    ssd1306_clear();
    draw_string(SCREEN_WIDTH/2 - 30, SCREEN_HEIGHT/2 - 5, "GAME OVER");
    ssd1306_show();
    sleep_ms(3000);
    in_menu = true;
}

int main() {
    stdio_init_all();
    sleep_ms(1000);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);

    ssd1306_init(i2c1, 0x3C, SCREEN_WIDTH, SCREEN_HEIGHT);

    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);

    gpio_init(JOY_BUTTON);
    gpio_set_dir(JOY_BUTTON, GPIO_IN);
    gpio_pull_up(JOY_BUTTON);

    while (1) {
        if (in_menu) {
            draw_menu();
            
            adc_select_input(0);
            uint16_t vrx_raw = adc_read();
            adc_select_input(1);
            uint16_t vry_raw = adc_read();
            
            update_joystick(vrx_raw, vry_raw);
        } 
        else {
            if (!game_over) {
                adc_select_input(0);
                uint16_t vrx_raw = adc_read();
                adc_select_input(1);
                uint16_t vry_raw = adc_read();
                
                update_joystick(vrx_raw, vry_raw);
                move_snake();
                draw_game();
                sleep_ms(100);
            } else {
                draw_game_over();
            }
        }
    }

    return 0;
}
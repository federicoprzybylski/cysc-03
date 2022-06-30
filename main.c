#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

/* Struct para los datos de temperatura */
typedef struct {
  float lm35;
  float pote;
  int16_t cd;
} temperature_data_t;

/* Queue para comunicar los dos nucleos */
queue_t queue;

/* Main para el core 1 */
void core1_main() {
  
    while(1) {
        /* Variable para recuperar el dato de la queue */
        temperature_data_t data;
        /* Espera a que esten los datos para recibir */
        queue_remove_blocking(&queue, &data);
        if (data.pote > data.lm35){
            pwm_set_gpio_level(8, 0);
            pwm_set_gpio_level(9, data.cd);
        }
        if (data.pote < data.lm35){
        pwm_set_gpio_level(9, 0);
        pwm_set_gpio_level(8, -data.cd);
        }
        sleep_ms(250);
    }
}

/* Main para el core 0 */
int main() {
    stdio_init_all();
    /* Inicializa la cola para enviar un unico dato */
    queue_init(&queue, sizeof(temperature_data_t), 1);
    /* Inicializa el core 1 */
    multicore_launch_core1(core1_main);
    float conversion_factor = 3.3 / (4095);
    stdio_init_all();
    adc_init();
    adc_gpio_init(27);
    adc_gpio_init(28);
    gpio_set_function(9, GPIO_FUNC_PWM);
    gpio_set_function(8, GPIO_FUNC_PWM);

  uint8_t slice_num = pwm_gpio_to_slice_num(9);
  pwm_config config = pwm_get_default_config();
  pwm_init(slice_num, &config, true);

    while(1) {
        /* Variable para enviar los datos */
            adc_select_input(1);
        uint16_t result = adc_read();
        float r = result * conversion_factor;
        int Ts = (r*100);
        adc_select_input(2);
        uint16_t result1 = adc_read();
        float r1 = result1 * conversion_factor;
        int Tp = (r1*35)/3.3;

        int16_t dif = Tp - Ts;
        if(dif > 10) {
            dif = 10;
        }
        else if(dif < -10) {
            dif = -10;
        }
        int16_t cd = dif * 0xffff / 10;

        printf("Temperatura: %d\n", Ts);
        printf("Pote: %d\n", Tp);
        temperature_data_t data;

        data.lm35 = Ts;
        data.pote = Tp;
        data.cd = cd;
        /* Cuando los datos estan listos, enviar por la queue */
        queue_add_blocking(&queue, &data);
    }
}
#include <stdio.h>
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/adc.h"

#define SPI_PORT spi0
#define PIN_MISO 0
#define PIN_CS   1
#define PIN_SCK  2
#define PIN_MOSI 3

#define ADC_PIN      26   // ADC0
#define ADC_CHANNEL  0
#define VEL_MAX      150000

static const uint32_t SPI_COMMAND_MAGIC = 0x7ABBCC7A;

typedef struct __attribute__((packed)) {
  int32_t velocity;
  int32_t velocity_setpoint;
  int16_t current_mA;
  int16_t current_setpoint_mA;
  int16_t duty_m;
  int8_t motor_direction;
  int8_t motor_coast;
} spi_status_t;

typedef struct __attribute__((packed)) {
  int32_t velocity;
  uint32_t _b1;
  uint32_t _b2;
  uint32_t magic;
} spi_command_t;

static_assert(sizeof(spi_status_t) == 16, "status size");
static_assert(sizeof(spi_command_t) == 16, "command size");

// Average 8 samples, map 0..4095 -> -VEL_MAX..+VEL_MAX
static int32_t read_velocity()
{
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) sum += adc_read();
    int32_t raw = sum / 8;
    return (int32_t)(((int64_t)raw * 2 * VEL_MAX) / 4095) - VEL_MAX;
}

int main()
{
    stdio_init_all();

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    spi_init(SPI_PORT, 200 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_CS,   GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);  // idle high

    spi_command_t cmd = {};
    cmd.magic = SPI_COMMAND_MAGIC;

    spi_status_t s;
    uint32_t n = 0;

    while (true) {
        cmd.velocity = read_velocity();

        gpio_put(PIN_CS, 0);
        spi_write_read_blocking(SPI_PORT, (const uint8_t *)&cmd,
                                (uint8_t *)&s, sizeof(cmd));
        sleep_us(400);
        gpio_put(PIN_CS, 1);

        printf("[%lu] vel=%ld vel_sp=%ld cur=%d mA cur_sp=%d mA duty=%d dir=%d coast=%d\n",
               (unsigned long)n++, (long)s.velocity, (long)s.velocity_setpoint,
               s.current_mA, s.current_setpoint_mA,
               s.duty_m, s.motor_direction, s.motor_coast);

        sleep_us(400);  // inter-frame gap so slave can re-arm
    }
}
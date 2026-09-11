#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdint.h>

/*
 * Software-UART TX:
 *
 * Nano D11
 * ATmega328P PB3
 * ICSP MOSI
 */
#define SOFT_TX_DDR  DDRB
#define SOFT_TX_PORT PORTB
#define SOFT_TX_BIT  PB3

#define BAUD_RATE 9600UL
#define BIT_TIME_US (1000000.0 / BAUD_RATE)

static void soft_uart_init(void)
{
    // PB3/D11 als Ausgang
    SOFT_TX_DDR |= (1 << SOFT_TX_BIT);

    // UART-Leitung ist im Ruhezustand HIGH
    SOFT_TX_PORT |= (1 << SOFT_TX_BIT);
}

static void soft_uart_write_byte(uint8_t value)
{
    /*
     * Während eines Zeichens unterbrechen wir keine Bitzeiten.
     * Den vorherigen Interrupt-Zustand merken wir uns.
     */
    uint8_t old_sreg = SREG;
    cli();

    // Startbit: LOW
    SOFT_TX_PORT &= ~(1 << SOFT_TX_BIT);
    _delay_us(BIT_TIME_US);

    // Acht Datenbits, niedrigstes Bit zuerst
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (value & 0x01) {
            SOFT_TX_PORT |= (1 << SOFT_TX_BIT);
        } else {
            SOFT_TX_PORT &= ~(1 << SOFT_TX_BIT);
        }

        _delay_us(BIT_TIME_US);
        value >>= 1;
    }

    // Stopbit: HIGH
    SOFT_TX_PORT |= (1 << SOFT_TX_BIT);
    _delay_us(BIT_TIME_US);

    // Vorherigen Interrupt-Zustand wiederherstellen
    SREG = old_sreg;
}

static void soft_uart_write_string(const char *text)
{
    while (*text != '\0') {
        soft_uart_write_byte((uint8_t)*text);
        text++;
    }
}

static void soft_uart_write_number(uint32_t number)
{
    char buffer[11];
    uint8_t position = 0;

    if (number == 0) {
        soft_uart_write_byte('0');
        return;
    }

    while (number > 0) {
        buffer[position++] = '0' + (number % 10);
        number /= 10;
    }

    while (position > 0) {
        soft_uart_write_byte(buffer[--position]);
    }
}

int main(void)
{
    soft_uart_init();

    _delay_ms(200);

    soft_uart_write_string("\r\n");
    soft_uart_write_string("Midnight-Worker ATmega328P\r\n");
    soft_uart_write_string("Bare-Metal avr-gcc gestartet!\r\n");

    uint32_t counter = 0;

    while (1) {
        soft_uart_write_string("Zaehler: ");
        soft_uart_write_number(counter);
        soft_uart_write_string("\r\n");

        counter++;

        _delay_ms(1000);
    }
}

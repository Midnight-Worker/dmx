#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define LCD_RS PD7    // Nano D7 -> LCD RS
#define LCD_E  PD6    // Nano D6 -> LCD E

void lcd_enable_pulse()
{
    PORTD |= (1 << LCD_E);
    _delay_us(1);

    PORTD &= ~(1 << LCD_E);
    _delay_us(50);
}

void lcd_send_nibble(uint8_t nibble)
{
    // Nano D2 bis D5 löschen
    PORTD &= ~((1 << PD2) |
               (1 << PD3) |
               (1 << PD4) |
               (1 << PD5));

    /*
        Nibble-Bit 0 -> Nano D5 -> LCD D4
        Nibble-Bit 1 -> Nano D4 -> LCD D5
        Nibble-Bit 2 -> Nano D3 -> LCD D6
        Nibble-Bit 3 -> Nano D2 -> LCD D7
    */

    if (nibble & 0x01)
        PORTD |= (1 << PD5);

    if (nibble & 0x02)
        PORTD |= (1 << PD4);

    if (nibble & 0x04)
        PORTD |= (1 << PD3);

    if (nibble & 0x08)
        PORTD |= (1 << PD2);

    lcd_enable_pulse();
}

void lcd_send_byte(uint8_t value, bool character)
{
    if (character)
        PORTD |= (1 << LCD_RS);
    else
        PORTD &= ~(1 << LCD_RS);

    lcd_send_nibble(value >> 4);
    lcd_send_nibble(value & 0x0F);
}

void lcd_command(uint8_t command)
{
    lcd_send_byte(command, false);

    if (command == 0x01 || command == 0x02)
        _delay_ms(2);
}

void lcd_write_char(char character)
{
    lcd_send_byte((uint8_t)character, true);
}

void lcd_write(const char* text)
{
    while (*text != '\0')
    {
        lcd_write_char(*text);
        text++;
    }
}

void lcd_clear()
{
    lcd_command(0x01);
}

void lcd_set_cursor(uint8_t column, uint8_t row)
{
    // Speicheradressen der vier Zeilen eines 20x4-LCDs
    const uint8_t row_address[4] =
    {
        0x00,    // Zeile 1
        0x40,    // Zeile 2
        0x14,    // Zeile 3
        0x54     // Zeile 4
    };

    if (row > 3)
        row = 3;

    if (column > 19)
        column = 19;

    lcd_command(0x80 | (row_address[row] + column));
}

void lcd_init()
{
    // Nano D2 bis D7 als Ausgang
    DDRD |= (1 << PD2) |
            (1 << PD3) |
            (1 << PD4) |
            (1 << PD5) |
            (1 << PD6) |
            (1 << PD7);

    PORTD &= ~((1 << LCD_RS) | (1 << LCD_E));

    _delay_ms(50);

    // HD44780 in den 4-Bit-Modus bringen
    lcd_send_nibble(0x03);
    _delay_ms(5);

    lcd_send_nibble(0x03);
    _delay_us(150);

    lcd_send_nibble(0x03);
    lcd_send_nibble(0x02);

    lcd_command(0x28);  // 4-Bit-Modus
    lcd_command(0x0C);  // Display an, Cursor aus
    lcd_command(0x06);  // Cursor nach rechts
    lcd_clear();
}

void setup()
{
    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_write("Hallo Maik!");

    lcd_set_cursor(0, 1);
    lcd_write("IR-Empfaenger");

    lcd_set_cursor(0, 2);
    lcd_write("Display: 20 x 4");

    lcd_set_cursor(0, 3);
    lcd_write("Controller gefunden!");
}

void loop()
{
    // Text bleibt dauerhaft stehen
    _delay_ms(1000);
}

int main()
{
    setup();

    while (true)
    {
        loop();
    }
}

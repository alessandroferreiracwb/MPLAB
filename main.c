/*
 * Controle básico do AD9850 com PIC16F628A
 * Frequência de saída: 1000 Hz (1 kHz)
 * Compilador: XC8 v2.40+ (modo Free aceitável)
 * MPLAB X IDE v6.25
 * Oscilador interno: 4 MHz (sem cristal)
 */

#include <xc.h>
#include <stdint.h>

// Configurações do oscilador e proteções
#pragma config FOSC = INTOSCIO  // Oscilador interno, I/O nos pinos RA6/RA7
#pragma config WDTE = OFF       // Watchdog Timer desligado
#pragma config PWRTE = OFF      // Power-up Timer desligado
#pragma config MCLRE = ON       // MCLR habilitado
#pragma config BOREN = OFF      // Brown-out Reset desligado
#pragma config LVP = OFF        // Low-Voltage Programming desligado
#pragma config CPD = OFF        // Proteção de código da EEPROM desligada
#pragma config CP = OFF         // Proteção de código do Flash desligada

#define _XTAL_FREQ 4000000UL  // Frequência do oscilador interno

// Definição dos pinos de controle do AD9850
#define DDS_DATA    RB0
#define DDS_CLK     RB1
#define DDS_FQ_UD   RB2
#define DDS_W_CLK   RB3

// Função para enviar 1 byte ao AD9850 (LSB first)
void dds_send_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; i++) {
        DDS_DATA = b & 1;
        __delay_us(1);
        DDS_CLK = 1;
        __delay_us(1);
        DDS_CLK = 0;
        b >>= 1;
    }
}

// Função para atualizar a frequência do DDS
void dds_set_frequency(uint32_t freq_hz) {
    // Frequência de referência típica do módulo AD9850: 125 MHz
    const uint32_t REF_CLK = 125000000UL;
    uint32_t tuning_word = (freq_hz * 4294967296ULL) / REF_CLK;

    // Coloca FQ_UD e W_CLK em nível baixo
    DDS_FQ_UD = 0;
    DDS_W_CLK = 0;

    // Envia os 4 bytes do tuning word (LSB first)
    dds_send_byte(tuning_word & 0xFF);
    dds_send_byte((tuning_word >> 8) & 0xFF);
    dds_send_byte((tuning_word >> 16) & 0xFF);
    dds_send_byte((tuning_word >> 24) & 0xFF);

    // Envia byte de controle (0x00 = modo normal, sem reset, sem power-down)
    dds_send_byte(0x00);

    // Pulso em W_CLK para carregar os dados
    DDS_W_CLK = 1;
    __delay_us(1);
    DDS_W_CLK = 0;

    // Pulso em FQ_UD para atualizar a saída
    DDS_FQ_UD = 1;
    __delay_us(1);
    DDS_FQ_UD = 0;
}

void main(void) {
    // Configura PORTB como saída
    TRISB = 0x00;  // Todos os pinos de PORTB como saída
    PORTB = 0x00;

    // Inicializa o AD9850 com 1 kHz
    dds_set_frequency(1000);  // 1 kHz

    // Loop infinito (opcional: você pode adicionar botões, UART, etc.)
    while (1) {
        // Nada a fazer — sinal já está sendo gerado pelo AD9850
        __delay_ms(100);
    }
}

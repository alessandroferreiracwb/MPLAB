/*
 * Controle do AD9850 via UART no PIC16F628A
 * Recebe frequência via serial (ex: "1000000" para 1 MHz)
 * Baud rate: 9600 @ 4 MHz internal oscillator
 * MPLAB X v6.25 + XC8
 */

#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Configurações de fusíveis
#pragma config FOSC = INTOSCIO  // Oscilador interno, RA6/RA7 como I/O
#pragma config WDTE = OFF
#pragma config PWRTE = OFF
#pragma config MCLRE = ON
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config CP = OFF

#define _XTAL_FREQ 4000000UL

// Pinos do AD9850
#define DDS_DATA    RB0
#define DDS_CLK     RB1
#define DDS_FQ_UD   RB2
#define DDS_W_CLK   RB3

// Buffer para recepção serial
#define BUF_SIZE 16
char rx_buffer[BUF_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t rx_ready = 0;

// Protótipos
void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
uint32_t parse_uint32(const char *str);
void dds_send_byte(uint8_t b);
void dds_set_frequency(uint32_t freq_hz);

// ISR da UART (recepção)
void __interrupt() isr(void) {
    if (PIR1bits.RCIF) {
        char c = RCREG; // Limpa a flag automaticamente ao ler
        if (c == '\n' || c == '\r') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                rx_ready = 1;
                rx_index = 0;
            }
        } else if (rx_index < BUF_SIZE - 1) {
            rx_buffer[rx_index++] = c;
        }
    }
}

// Inicializa UART a 9600 bauds @ 4 MHz
void uart_init(void) {
    TRISAbits.TRISA1 = 1; // RA1 = RX (entrada)
    TRISAbits.TRISA2 = 0; // RA2 = TX (saída)

    // Configura USART: modo assíncrono, 8 bits, 1 stop, sem paridade
    TXSTA = 0b00100100; // TXEN=1, BRGH=1 (high speed)
    RCSTA = 0b10010000; // SPEN=1, CREN=1, RX enable
    // Baud rate: 9600 @ 4 MHz → SPBRG = (4000000/(16*9600)) - 1 ≈ 25
    SPBRG = 25;

    // Habilita interrupção de recepção
    PIE1bits.RCIE = 1;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

void uart_putc(char c) {
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

// Envia byte ao AD9850 (LSB first)
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

// Atualiza DDS com nova frequência
void dds_set_frequency(uint32_t freq_hz) {
    const uint32_t REF_CLK = 125000000UL;
    uint32_t tuning_word = (freq_hz * 4294967296ULL) / REF_CLK;

    DDS_FQ_UD = 0;
    DDS_W_CLK = 0;

    dds_send_byte(tuning_word & 0xFF);
    dds_send_byte((tuning_word >> 8) & 0xFF);
    dds_send_byte((tuning_word >> 16) & 0xFF);
    dds_send_byte((tuning_word >> 24) & 0xFF);
    dds_send_byte(0x00); // control byte

    DDS_W_CLK = 1;
    __delay_us(1);
    DDS_W_CLK = 0;

    DDS_FQ_UD = 1;
    __delay_us(1);
    DDS_FQ_UD = 0;
}

// Converte string ASCII para uint32_t (sem usar sscanf)
uint32_t parse_uint32(const char *str) {
    uint32_t val = 0;
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            val = val * 10 + (*str - '0');
        } else {
            break; // caractere inválido
        }
        str++;
    }
    return val;
}

// Função principal
void main(void) {
    // Configura I/O
    TRISB = 0x00;
    PORTB = 0x00;
    CMCON = 0x07; // Desliga comparadores (importante no PIC16F628A!)

    // Inicializa UART
    uart_init();

    // Frequência inicial
    dds_set_frequency(1000); // 1 kHz
    uart_puts("AD9850 pronto. Envie frequencia em Hz (ex: 1000000)\r\n");

    while (1) {
        if (rx_ready) {
            uint32_t freq = parse_uint32(rx_buffer);
            if (freq >= 1 && freq <= 40000000) { // limite seguro (~40 MHz)
                dds_set_frequency(freq);
                uart_puts("Freq set: ");
                uart_puts(rx_buffer);
                uart_puts(" Hz\r\n");
            } else {
                uart_puts("Erro: freq fora do intervalo (1-40000000 Hz)\r\n");
            }
            rx_ready = 0;
        }
        __delay_ms(10);
    }
}


/*
 * Controle básico do AD9850 com PIC16F628A
 * Frequência de saída: 1000 Hz (1 kHz)
 * Compilador: XC8 v2.40+ (modo Free aceitável)
 * MPLAB X IDE v6.25
 * Oscilador interno: 4 MHz (sem cristal)
 */
/*
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
*/


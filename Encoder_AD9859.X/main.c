#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// Definicoes de FUSE
#pragma config FOSC = INTOSCIO, WDTE = OFF, PWRTE = OFF, MCLRE = ON, BOREN = OFF, LVP = OFF, CPD = OFF, CP = OFF

// Definicao da frequencia do oscilador interno
#define _XTAL_FREQ 4000000UL // Frequencia de 4MHz para o oscilador interno

// --- Definicoes de Pinos do AD9850 ---
#define DDS_DATA    PORTBbits.RB0
#define DDS_CLK     PORTBbits.RB1
#define DDS_FQUD    PORTBbits.RB2
#define DDS_RESET   PORTBbits.RB3

// --- Definicoes do Encoder Rotativo ---
#define ENCODER_A_PIN   RB4         // Pino CLK do encoder
#define ENCODER_B_PIN   RB5         // Pino DT do encoder

// --- Definicoes do Botao de Selecao de Incremento ---
#define BTN_INC_SEL PORTAbits.RA2

// Definicoes do AD9850
#define DDS_REF_CLK 125000000UL     // 125 MHz

// Variaveis globais
volatile int encoder_counter = 0;   // Contador de pulsos do encoder
unsigned long currentFreq = 1000000UL; // 1 MHz inicial
unsigned long minFreq = 100UL;
unsigned long maxFreq = 30000000UL;
int last_encoder_value = 0;         // Para detectar mudancas no encoder

// Array de valores de incremento
unsigned long incrementValues[] = {100UL, 1000UL, 10000UL, 100000UL, 1000000UL};
#define NUM_INCREMENTS (sizeof(incrementValues) / sizeof(unsigned long))
unsigned char currentIncrementIndex = 2; // Inicia com 10 kHz

// Prototipos das funcoes
void DDS_Reset(void);
void pulse_byte(unsigned char data);
void DDS_SetFrequency(unsigned long frequency);
void init_ports(void);

// --- Rotina de Interrupcao do Encoder ---
void __interrupt() ISR_Handler(void) {
    if (INTF) { // Verifica se a interrupcao externa do pino RB0 (INT) foi acionada
        if (ENCODER_B_PIN == 1) {
            encoder_counter++;
        } else {
            encoder_counter--;
        }
        INTF = 0; // Limpa a flag de interrupcao
    }
}

// Funcao principal
void main(void) {
    init_ports();
    
    // Configura a interrupcao externa para o pino do encoder
    GIE = 1;    // Habilita interrupcoes globais
    INTE = 1;   // Habilita a interrupcao externa no pino INT (RB0)
    INTEDG = 1; // Interrupcao na borda de subida

    // Inicia o modulo AD9850
    DDS_Reset();
    DDS_SetFrequency(currentFreq);

    while (1) {
        // Verifica se o valor do encoder mudou
        if (last_encoder_value != encoder_counter) {
            // Ajusta a frequencia com base na mudanca do encoder
            currentFreq += (encoder_counter - last_encoder_value) * incrementValues[currentIncrementIndex];
            
            // Garante que a frequencia nao saia dos limites
            if (currentFreq > maxFreq) {
                currentFreq = maxFreq;
            } else if (currentFreq < minFreq) {
                currentFreq = minFreq;
            }
            
            // Atualiza a frequencia no AD9850
            DDS_SetFrequency(currentFreq);
            
            // Atualiza o ultimo valor do encoder
            last_encoder_value = encoder_counter;
        }
        
        // Altera o valor do incremento (mantem a logica do botao)
        if (BTN_INC_SEL == 1) {
            __delay_ms(10);
            if (BTN_INC_SEL == 1) {
                currentIncrementIndex++;
                if (currentIncrementIndex >= NUM_INCREMENTS) {
                    currentIncrementIndex = 0; 
                }
                while (BTN_INC_SEL == 1);
            }
        }
    }
}

// Funcao de inicializacao das portas
void init_ports(void) {
    CMCON = 0x07;   
    TRISB = 0b00110000; // RB4 e RB5 como entrada (encoder), resto como saida
    TRISA = 0b11111111; // Todos os pinos do PORTA como entrada
}

// Funcao para reiniciar o AD9850
void DDS_Reset(void) {
    DDS_RESET = 0;
    __delay_ms(20);
    DDS_RESET = 1;
    __delay_ms(20);
    DDS_RESET = 0;
}

// Funcao para enviar um byte ao AD9850
void pulse_byte(unsigned char data) {
    for (char i = 0; i < 8; i++) {
        DDS_DATA = (data >> i) & 0x01;
        DDS_CLK = 1;
        DDS_CLK = 0;
    }
}

// Funcao para definir a frequencia do AD9850
void DDS_SetFrequency(unsigned long frequency) {
    unsigned long tuningWord = (unsigned long)(((unsigned long long)frequency * 4294967295ULL) / (unsigned long long)DDS_REF_CLK);

    for (char i = 0; i < 4; i++) {
        pulse_byte((tuningWord >> (8 * i)) & 0xFF);
    }
    pulse_byte(0x00);

    DDS_FQUD = 1;
    __delay_us(1);  
    DDS_FQUD = 0;
}
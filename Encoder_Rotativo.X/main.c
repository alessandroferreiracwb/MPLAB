#include <xc.h>

// Configurações do PIC (ajuste conforme seu cristal e necessidades)
#pragma config FOSC = INTOSCIO  // Oscilador interno
#pragma config WDTE = OFF       // Watchdog Timer desativado
#pragma config PWRTE = ON       // Power-up Timer ativado
#pragma config MCLRE = OFF      // Pino RE3/MCLR como I/O digital
#pragma config BOREN = OFF      // Brown-out Reset desativado
#pragma config LVP = OFF        // Low Voltage Programming desativado
#pragma config CPD = OFF        // Código de dados não protegido
#pragma config CP = OFF         // Código do programa não protegido

// Definições de frequência do oscilador e do encoder
#define _XTAL_FREQ 4000000UL    // Frequência do oscilador (4 MHz)
#define CLK_PIN RB0             // Pino CLK (A) conectado a INT
#define DT_PIN RB1              // Pino DT (B)

volatile int encoder_counter = 0; // Contador de pulsos do encoder

// --- Rotina de Interrupção ---
void __interrupt() ISR_Handler(void) {
    if (INTF) { // Verifica se a interrupção externa do pino RB0 (INT) foi acionada
        if (DT_PIN == 1) {
            // Se o pino DT está em nível alto, a rotação é no sentido horário
            encoder_counter++;
        } else {
            // Se o pino DT está em nível baixo, a rotação é no sentido anti-horário
            encoder_counter--;
        }

        // Limpa a flag de interrupção para permitir novas interrupções
        INTF = 0;
    }
}

// --- Função Principal (main) ---
void main(void) {
    // Configurações iniciais
    TRISB0 = 1; // Pino RB0 como entrada (CLK)
    TRISB1 = 1; // Pino RB1 como entrada (DT)
    
    // Configurações de interrupção
    GIE = 1;    // Habilita interrupções globais
    PEIE = 1;   // Habilita interrupções de periféricos (opcional, mas boa prática)
    INTE = 1;   // Habilita a interrupção externa no pino INT (RB0)
    INTEDG = 1; // Interrupção na borda de subida (rising edge)
                // Se o seu encoder for muito rápido, teste com INTEDG = 0 (falling edge) também.

    // Loop principal
    while (1) {
        // A lógica principal do seu programa fica aqui.
        // O valor do encoder_counter é atualizado automaticamente na interrupção.
        // Você pode ler ou usar o valor de encoder_counter aqui.
        // Exemplo:__delay_ms(100);
    }
}
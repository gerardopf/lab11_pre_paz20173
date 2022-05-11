/*
 * Lab11
 * Gerardo Paz Fuentes
 * 
 * Potenciómetro en RA0
 * Comunicación SPI, se envía valor de potenciómetro al esclavo
 * Valor del potenciómetro en puerto D
 *
 * Fecha de creación: 11/05/22
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>         // registros del PIC
#include <stdio.h>
#include <stdlib.h>

/* -------------CONSTANTES ------------ */
#define _XTAL_FREQ 1000000

/* ------------- VARIABLES ------------ */
uint8_t cont = 0;           // valor del potenciómetro

/* ------------- PROTOTIPOS DE FUNCIÓN ------------ */
void setup(void);

/* -------------CÓDIGO PRINCIPAL  ------------ */
void __interrupt() isr (void){
    if(ADIF){
        PIR1bits.ADIF = 0;
        cont = ADRESH;
        SSPBUF = cont;
        PORTD = cont;
    }
    if(PIR1bits.SSPIF){             // ¿Recibió datos el esclavo?
        PORTD = SSPBUF;             // Mostramos valor en el PORTD
        PIR1bits.SSPIF = 0;         // limpar bandera
    }
    return;
}

void main(void) {
    setup();
    while(1){
        // El RA0 on - Maestro
        if(PORTAbits.RA0){          // ¿Es maestro?    
            if(ADCON0bits.GO == 0){
                __delay_us(40);         // carga el capacitor
                ADCON0bits.GO = 1;      // iniciar conversión
            }
        }
    }
    return;
}

void setup(void){
    ANSEL = 0b00000010;
    ANSELH = 0;                 // AN1
    
    OSCCONbits.IRCF = 0b100;    // 1MHz
    OSCCONbits.SCS = 1;         // Reloj interno
    
    TRISA = 0b00100011;         // SS RA0 AN1 como entradas
    PORTA = 0;
    
    TRISD = 0;
    PORTD = 0;  

    // SPI
    // MAESTRO
    if(PORTAbits.RA0){
        TRISC = 0b00010000;         // SDI entrada, SCK y SD0 out
        PORTC = 0;
    
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0000;   // SPI Maestro, Reloj - Fosc/4 (250kbits/s)
        SSPCONbits.CKP = 0;         // Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // Habilitar pines de SPI
        
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 1;        // Dato al final del pulso de reloj
        SSPBUF = cont;              // Enviamos un valor inicial
        
        // INTERRUPCIONES
        INTCONbits.GIE = 1;     // globales
        INTCONbits.PEIE = 1;    // periféricos
        PIE1bits.ADIE = 1;      // ADC

        PIR1bits.ADIF = 0;      // bandera ADC limpia
        
        // ADC
        ADCON0bits.ADCS = 0b00; // Fosc/2
        ADCON1bits.VCFG0 = 0;   // VDD
        ADCON1bits.VCFG1 = 0;   // VSS
        ADCON0bits.CHS = 1;     // AN0
        ADCON1bits.ADFM = 0;    // justificado a la izquierda
        ADCON0bits.ADON = 1;    // ADC habilitado
        __delay_us(40);         // tiempo para carga del capacitor
    }
    // ESCLAVO
    else{
        TRISC = 0b00011000; // SDI y SCK in, SD0 out
        PORTC = 0;
        
        // SSPCON <5:0>
        SSPCONbits.SSPM = 0b0100;   // SPI Esclavo, SS hablitado
        SSPCONbits.CKP = 0;         // Reloj inactivo en 0
        SSPCONbits.SSPEN = 1;       // Habilitamos pines de SPI
        
        // SSPSTAT<7:6>
        SSPSTATbits.CKE = 1;        // Dato enviado cada flanco de subida
        SSPSTATbits.SMP = 0;        // Dato al final del pulso de reloj
        
        // INTECCUPCIONES
        PIE1bits.SSPIE = 1;         // SPI
        INTCONbits.PEIE = 1;        // periféricos
        INTCONbits.GIE = 1;         // globales
        
        PIR1bits.SSPIF = 0;         // bandera de SPI limpia
    }
}

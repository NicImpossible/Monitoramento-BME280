/*
 * Desenvolvido por Nicole R  13/03/2023
 * Programa��o para testar fun��es do BME280 com biblioteca diferente
 *
 * Concluida em 14/03/2023
 * Teste realizado com sucesso!
 *
 * Teste 2 - Fun��o de monitoramento completo 25/03
 * Com Timer, BME280 e LoRa
 * Um sucesso!
 *
 */

#include <msp430.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "BME280.h"

void ConfigTIMER(void);
void ConfigSPI(void);
void ConfigUART(void);
void SendUart(char *);

// Variaveis para receber dados
volatile int32_t CorT;
volatile uint32_t CorH, CorP;
char str[120];
volatile uint8_t cont; // Vari�vel para incrementar e aumentar n�mero de tempo


int main(void){

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5;

    ConfigTIMER();
    ConfigUART();
    ConfigSPI();


    if(ReadTHid()); // Verifique a presen�a do sensor, leia seu c�digo de identifica��o
    else  // Para a CPU e liga o LED vermelho se n�o for encontrado
    {
        P1OUT |= BIT1;
        while(1){}
    }

    // Obt�m os coeficientes de compensa��o do dispositivo para convers�o de dados brutos
    GetCompData();


    while(1){

        __bis_SR_register(LPM0_bits | GIE);  // habilitar interrup��es globais e Low power mode

    }
}

// Rotina de servi�o de interrup��o do Timer A0
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{
    cont++;

    // Para ativar ap�s 3 minutos
    if(cont == 180){

        // Leitura r�pida no SPI para obter 3 bytes de dados de press�o, 3 bytes de temperatura e 2 bytes de umidade
        // ReadTHsensor();

        // Aplica fatores de calibra��o aos dados brutos
        CorT = CalcTemp(); // Temperatura corrigida
        CorH = CalcHumid(); // Umidade corrigida
        CorP = CalcPress(); // Press�o corrigida

        memset(str, 0, sizeof(str));

        snprintf (str, 120, "%d.%d/%d.%d&%d.%d", CorP/1000, CorP%1000, CorT/100, CorT%100,  CorH/1000, CorH%1000);


        SendUart(str); // Enviar dados
        cont = 0;
    }
}



void ConfigTIMER(void){

    TA0CCTL0 |= CCIE;  // Habilita a interrup��o do CCR
    TA0CCR0 = 32678;

    TA0CTL |= TASSEL__ACLK | MC__UP;  // A fonte do rel�gio Timer (ACLK) e Up Mode: Timer conta at� TAxCCR0

    /*
     * Vamos fazer algumas contas com as informa��es que sabemos:
     *
     * ACLK = 32678 Hz, ent�o a contagem de um temporizador � 1/(32678 Hz) = 31 us.
     * O timer ser� interrompido quando a contagem do Timer_A TA0R = TA0CCR0 = 32678.
     *
     * Portanto o timer ir� interromper em aproximadamente 32678 * 31u = 1s.
     */
}

void ConfigSPI(void){

    /* Port 2
     *
     * P2.6 UCB1 MOSI <--> SDA
     * P2.5 UCB1 MISO <--> SDO
     * P2.4 UCB1 CLK  <--> SCL
     * P1.7 Chip select.
     *
     */

    P2SEL0 |= BIT4 | BIT5 | BIT6; // Configura os pinos para SPI MOSI, MISO e CLK
    P2SEL1 &= ~(BIT4 | BIT5 | BIT6);

    P1DIR |= BIT7 | BIT1;  // Puxe esta linha para baixo para ativar a comunica��o BME280
    P1OUT &= ~BIT1;

    // Configure o m�dulo eUSCI_B1 para SPI de 3 pinos a 1 MHz
    UCA1CTLW0 |= UCSWRST;                     // **Coloque a m�quina de estado em reset**

    // Polaridade do rel�gio alta, MSB
    UCA1CTLW0 |= UCSSEL__SMCLK;               // SMCLK

    // UCA1CTLW0 |= UCMST|UCSYNC|UCCKPL|UCMSB;   // 3-pin, 8-bit SPI master
    UCA1CTLW0 |= UCMST | UCCKPL | UCMSB | UCSYNC | UCMODE_0; // 3-pin, 8-bit SPI master

    UCA1BR0 = 0x01;                           // /1, fBitClock = fBRCLK/UCBRx
    UCA1BR1 = 0;

    UCA1MCTLW = 0;                            // Sem modula��o
    UCA1CTLW0 &= ~UCSWRST;                    // **Inicializar m�quina de estado USCI**

}

void ConfigUART(void){ // UCA0 modulo

    // Configurar pinos UART
    P1SEL1 &= ~(BIT4 | BIT5);                 // USCI_A0 UART operation
    P1SEL0 |= BIT4 | BIT5;                    // define o pino 2-UART como segunda fun��o

    P2DIR |= BIT0 | BIT1;
    P2OUT |= BIT0 | BIT1; // LoRa inicia em Sleep Mode

    // Configurar UART
    UCA0CTLW0 |= UCSWRST;                     // Colocar eUSCI em reset
    UCA0CTLW0 |= UCSSEL__SMCLK;


    /* C�lculo do Baud Rate
     * N = 1000000/9600 = 104.16
     * Como 104 > 16, UCA0BR0 = INT(104/16) = 6
     * Por��o fracion�ria = 0.5
     * UCBRSx value = 0x20 (Ver UG table 22-4)
     * UCBRFx = INT([(N/16) � INT(N/16)] � 16) = 8
     */
    UCA0BR0 = 6;
    UCA0MCTLW = 0xAA00 | UCOS16 | UCBRF_8;
    // UCA0MCTLW = 0x2000 | UCOS16 | UCBRF_8;        Segunda op��o

    UCA0BR1 = 0;
    UCA0CTLW0 &= ~UCSWRST;                    // Inicializa o eUSCI
}

void SendUart(char * msg){
    uint16_t u = 0;

    P2OUT &= ~BIT0 & ~BIT1;   // LoRa entra no Normal Mode
    __delay_cycles(100000);


    // Cabe�alho LoRa codificado (Address and Channel)
    while(!(UCA0IFG & UCTXIFG)); // Buffer USCI_A0 TX pronto?
    UCA0TXBUF = 0x00;             // Address MSB
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = 0x04;             // Address LSB
    while (!(UCA0IFG & UCTXIFG));
    UCA0TXBUF = 0x0F;             // Channel


    for (u = 0; u < sizeof(str); u++)
    {
        while (!(UCA0IFG & UCTXIFG)); // USCI_A0 TX buffer ready?
        UCA0TXBUF = str[u];  // Send data 1 byte at a time

    }

    P2OUT |= BIT0 | BIT1; // LoRa retorna para Sleep Mode
    __no_operation();                     // For debugger

}

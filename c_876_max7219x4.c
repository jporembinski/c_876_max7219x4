/*
 * File:   c_876_max7219x4.c
 * Author: Jacek Porembiński
 *
 * Created on 19-01-2021.
 */

// Ustawienie bitow konfiguracyjnych
#pragma config FOSC = EXTRC     // Oscillator Selection bits (RC oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config CP = OFF         // FLASH Program Memory Code Protection bits (Code protection off)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low Voltage In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection (Code Protection off)
#pragma config WRT = OFF        // FLASH Program Memory Write Enable (Unprotected program memory may not be written to by EECON control)

// Definicje nazw pinow mikrokontrolera do obslugi wyswietlacza po magistrali SPI
#define DATA PORTCbits.RC5
#define CLOCK PORTCbits.RC6
#define LOAD  PORTCbits.RC7

// Zdefiniowanie czestotliwosci taktowania mikrokontrolera dla funkcji __delay_ms())
#define _XTAL_FREQ 1700000

#include <xc.h>

// Prototypy funkcji
void Display(char Byte1, char Byte2, char Byte3, char Byte4, char Byte5, char Byte6, char Byte7, char Byte8);
void I2C_Master_Init(const unsigned long c);
void I2C_Master_Wait(void);
void I2C_Master_Start(void);
void I2C_Master_RepeatedStart(void);
void I2C_Master_Stop(void);
void I2C_Master_Write(unsigned char d);
unsigned char I2C_Master_Read(unsigned char a);
void setsek(void);
void setmin(unsigned char a);
void setgodz(unsigned char a);
unsigned char BCD2Binary(unsigned char a);
unsigned char Binary2BCD(unsigned char a);
// Koniec prototypow

void main(void)
{
    const unsigned char cyfra1[88] = {
    0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C,     // zero
    0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,     // jedynka
    0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E,     // dwojka
    0x3C, 0x66, 0x06, 0x1C, 0x06, 0x06, 0x66, 0x3C,     // trojka
    0x0C, 0x18, 0x30, 0x60, 0x6C, 0x7E, 0x0C, 0x0C,     // czworka
    0x7E, 0x60, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C,     // piatka
    0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x3C,     // szostka
    0x7E, 0x06, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18,     // siodemka
    0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x66, 0x3C,     // osemka
    0x3C, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C,     // dziewiatka
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };   // blank
    
    const unsigned char cyfra2[80] = {
    0x78, 0xCC, 0xCD, 0xCD, 0xCC, 0xCD, 0xCD, 0x78,     // zero
    0x30, 0x70, 0x31, 0x31, 0x30, 0x31, 0x31, 0x78,     // jedynka
    0x78, 0xCC, 0x0D, 0x19, 0x30, 0x61, 0xC1, 0xFC,     // dwojka
    0x78, 0xCC, 0x0D, 0x39, 0x0C, 0x0D, 0xCD, 0x78,     // trojka
    0x18, 0x30, 0x61, 0xC1, 0xD8, 0xFD, 0x19, 0x18,     // czworka
    0xFC, 0xC0, 0xC1, 0xF9, 0x0C, 0x0D, 0xCD, 0x78,     // piatka
    0x78, 0xCC, 0xC1, 0xF9, 0xCC, 0xCD, 0xCD, 0x78,     // szostka
    0xFC, 0x0C, 0x0D, 0x19, 0x30, 0x31, 0x31, 0x30,     // siodemka
    0x78, 0xCC, 0xCD, 0x79, 0xCC, 0xCD, 0xCD, 0x78,     // osemka
    0x78, 0xCC, 0xCD, 0xCD, 0x7C, 0x0D, 0xCD, 0x78 };   // dziewiatka
    
    const unsigned char cyfra3[48] = {
    0x1E, 0x33, 0xB3, 0xB3, 0x33, 0xB3, 0xB3, 0x1E,     // zero
    0x0C, 0x1C, 0x8C, 0x8C, 0x0C, 0x8C, 0x8C, 0x1E,     // jedynka
    0x1E, 0x33, 0x83, 0x86, 0x0C, 0x98, 0xB0, 0x3F,     // dwojka
    0x1E, 0x33, 0x83, 0x8E, 0x03, 0x83, 0xB3, 0x1E,     // trojka
    0x06, 0x0C, 0x98, 0xB0, 0x36, 0xBF, 0x86, 0x06,     // czworka
    0x3F, 0x30, 0xB0, 0xBE, 0x03, 0x83, 0xB3, 0x1E };     // piatka
    
    unsigned char sekundy, minuty, godziny, k, jm, dm, jg, dg;
    OPTION_REG &= ~(1<<7); // Wyzeruj bit RBPU (włącz podciąganie na porcie B)
    ADCON1 = 0x06;
    TRISB = 0b11111111;
    TRISC = 0b00011111;

    __delay_ms(500);    // opoznienie na wlaczenie podciagania na porcie B
    
    LOAD = 1;
    Display(0x0C, 0x01, 0x0C, 0x01, 0x0C, 0x01, 0x0C, 0x01);    // Shutdown Register (0x0C) normal operation (0x01)
    Display(0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00);    // Intensity (0x0A) 1/32 (0x00)
    Display(0x09, 0x00, 0x09, 0x00, 0x09, 0x00, 0x09, 0x00);    // Decode-Mode Register Examples (0x09) No decode (0x00)
    Display(0x0B, 0x07, 0x0B, 0x07, 0x0B, 0x07, 0x0B, 0x07);    // Scan-Limit Register Format (0x0B) Display digits 0 1 2 3 4 5 6 7 (0x07)
    Display(0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00);    // Display-Test Register Format (0x0F) normal operation (0x00)
        
    I2C_Master_Init(50000);     // zainicjowanie interfejsu i2c (predkosc 50 kHz)

    while(1) {
        if(!PORTBbits.RB1) setsek();
        if(!PORTBbits.RB2) setmin(minuty);
        if(!PORTBbits.RB3) setgodz(godziny);
        __delay_ms(25);

        I2C_Master_Start();
        I2C_Master_Write(0xD0);
        I2C_Master_Write(0x00);
        I2C_Master_RepeatedStart();
        I2C_Master_Write(0xD1);
        sekundy = I2C_Master_Read(1);
        minuty = I2C_Master_Read(1);
        godziny = I2C_Master_Read(0);
        I2C_Master_Stop();

        jm = minuty & 0x0F;
        dm = (minuty>>4);

        jg = godziny & 0x0F;
        if(!(dg = (godziny>>4)))
            dg = 10;
        if(sekundy & 0x01) {
        for(k=0;k<8;k++)
          Display(k+1, cyfra1[dg*8+k], k+1, cyfra2[8*jg+k], k+1, cyfra3[8*dm+k], k+1, cyfra1[8*jm+k]);
        }
        else {
        for(k=0;k<8;k++)    
          Display(k+1, cyfra1[dg*8+k], k+1, cyfra1[8*jg+k]<<1, k+1, cyfra1[8*dm+k]>>1, k+1, cyfra1[8*jm+k]);
        }
    }
    return;
}


// funkcja wysyla osiem bajtow do MAX7219
void Display(char Byte1, char Byte2, char Byte3, char Byte4, char Byte5, char Byte6, char Byte7, char Byte8)
{
    unsigned char i;
    
    CLOCK = 0;  // linia CLOCK w stan niski
    LOAD = 0;  // linia /LOAD w stan niski (poczatek transmisji)
    
    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte1 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte2 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte3 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte4 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte5 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte6 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte7 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    for(i=0;i<8;i++)
    {
        DATA = 0;
        if((Byte8 << i) & 0x80)    // nadawaj bajt od najstarszego bitu
            DATA = 1;
        CLOCK = 1;  // pojedynczy impuls na linii zegara zatwierdzajacy dane
        CLOCK = 0;
    }

    LOAD = 1;       // linia /LOAD w stan wysoki (koniec transmisji)
    
    return;
}


// funkcja I2C_Master_Init wymaga podania parametru w postaci szybkosci
// transmisji po magistrali i2c. Szybkosc podajemy w Hz (np. 100000)
void I2C_Master_Init(const unsigned long c)
{
  SSPCON = 0b00101000;            //SSP Module as Master
  SSPCON2 = 0;
  SSPADD = (_XTAL_FREQ/(4*c))-1; //Setting Clock Speed
  SSPSTAT = 0;
  TRISC3 = 1;                   //Setting as input as given in datasheet
  TRISC4 = 1;                   //Setting as input as given in datasheet
}

void I2C_Master_Wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F)); //Transmit is in progress
}

void I2C_Master_Start()
{
  I2C_Master_Wait();    
  SEN = 1;             //Initiate start condition
}

void I2C_Master_RepeatedStart()
{
  I2C_Master_Wait();
  RSEN = 1;           //Initiate repeated start condition
}

void I2C_Master_Stop()
{
  I2C_Master_Wait();
  PEN = 1;           //Initiate stop condition
}

void I2C_Master_Write(unsigned char d)
{
  I2C_Master_Wait();
  SSPBUF = d;         //Write data to SSPBUF
}

unsigned char I2C_Master_Read(unsigned char a)
{
  unsigned char temp;
  I2C_Master_Wait();
  RCEN = 1;
  I2C_Master_Wait();
  temp = SSPBUF;      //Read data from SSPBUF
  I2C_Master_Wait();
  ACKDT = (a)?0:1;    //Acknowledge bit
  ACKEN = 1;          //Acknowledge sequence
  return temp;
}

void setsek(void)
{
    __delay_ms(25);
    I2C_Master_Start();
    I2C_Master_Write(0xD0); // zaadresuj układ DS3231 (zapis))
    I2C_Master_Write(0x00); // znacznik na adres 0x00 (sekundy))
    I2C_Master_Write(0x00); // wyzeruj sekundy
    I2C_Master_Stop();
    while(!PORTBbits.RB1){};  // czekaj na zwolnienie przycisku
    __delay_ms(25);
}

void setmin(unsigned char m)
{
    unsigned char binmin;
    binmin = BCD2Binary(m);
    if(++binmin>59) binmin = 0;
    m = Binary2BCD(binmin);
    __delay_ms(25);
    I2C_Master_Start();
    I2C_Master_Write(0xD0); // zaadresuj układ DS3231 (zapis))
    I2C_Master_Write(0x01); // znacznik na adres 0x01 (minuty))
    I2C_Master_Write(m); // wpisz ++minuty
    I2C_Master_Stop();
    while(!PORTBbits.RB2){};  // czekaj na zwolnienie przycisku
    __delay_ms(25);    
}

void setgodz(unsigned char g)
{
    unsigned char bingodz;
    bingodz = BCD2Binary(g);
    if(++bingodz>23) bingodz = 0;
    g = Binary2BCD(bingodz);
    __delay_ms(25);
    I2C_Master_Start();
    I2C_Master_Write(0xD0); // zaadresuj układ DS3231 (zapis))
    I2C_Master_Write(0x02); // znacznik na adres 0x0 (godziny))
    I2C_Master_Write(g); // wpisz ++godziny
    I2C_Master_Stop();
    while(!PORTBbits.RB3){};  // czekaj na zwolnienie przycisku
    __delay_ms(25); 
}

unsigned char BCD2Binary(unsigned char a)             //Convert BCD to Binary so we can do some basic calculations
 {
     unsigned char r,t;
     t = a & 0x0F;
     r = t;
     a = 0xF0 & a;
     t = a >> 4;
     t = 0x0F & t;
     r = t*10 + r;
     return r;
 }
 
unsigned char Binary2BCD(unsigned char a)             //Convert Binary to BCD so we can write to the DS1307 Register in BCD
 {
      unsigned char t1, t2;
      t1 = a%10;
      t1 = t1 & 0x0F;
      a = a/10;
      t2 = a%10;
      t2 = 0x0F & t2;
      t2 = t2 << 4;
      t2 = 0xF0 & t2;
      t1 = t1 | t2; return t1;
  }

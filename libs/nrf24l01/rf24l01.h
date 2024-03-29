/*
 * spi.h
 *
 *  Created on: 3 mar 2017
 *      Author: Patryk
 */

#ifndef RF24L01_H_
#define RF24L01_H_

#include<avr/io.h>
#include<util/delay.h>


#define CE 			PB3
#define CSN 		PB4
#define IRQ			PA0
#define IRQPIN   	PINA
#define R 			0
#define W 			1
#define READ_REG  	0b00000000
#define WRITE_REG 	0x20


#define STATUS        	0x07
#define REGISTER_MASK	0x1F
#define ACTIVATE      	0x50
#define R_RX_PL_WID   	0x60
#define R_RX_PAYLOAD  	0x61
#define W_TX_PAYLOAD  	0xA0
#define W_ACK_PAYLOAD 	0xA8
#define FLUSH_TX      	0xE1
#define FLUSH_RX      	0xE2
#define REUSE_TX_PL   	0xE3
#define NOP           	0xFF
#define CONFIG			0x00


#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD	    0x1C
#define FEATURE	    0x1D

uint8_t dummy[5];
enum nrfMode {TRAN, RECE};
enum nrfMode RadioMode;

uint8_t dataToSend[5], dataCount = 0;

////////////////////////////// SPI

void init_SPI(char MASTR,char DIVIDE)
{
	if(MASTR == 1)//if master then set master bit and sck speed
	{
		DDRB |= (1<<DDB5) | (1<<DDB7) | (1<<DDB4) | (1 << DDB3); //MOSI,SCK,SS OUTPUT

		SPCR |= (1<<MSTR);


		if(DIVIDE == 64)
		{
			SPCR |= (1<<SPR1);
		}
		else if(DIVIDE == 16)
		{
			SPCR |= (1<<SPR0);
		}
		else if(DIVIDE == 4)
		{

		}
		else //fosc/128
		{
			SPCR |= (1<<SPR0) | (1<<SPR1);
		}

	}
	else
	{
		DDRB |= (1 << PB6); //MISO OUTPUT in slave
	}

	PORTB |= (1<<CSN); // RF doesn't listen to uC
	PORTB &= ~(1<<CE); // RF doesn't try to send anything
	SPCR |= (1<<SPE);//enable SPI
}

char send_SPI(char Data)
{
	SPDR = Data;
	while(! (SPSR & (1<<SPIF)));
	return SPDR;
}

////////////////////////////// RADIO

char get_reg(char Reg)
{

	PORTB &= ~(1 << CSN);
	_delay_us(10);
	send_SPI(Reg);
	_delay_us(10);
	Reg = send_SPI(0xFF);
	_delay_us(10);
	PORTB |= (1<<CSN);
	return Reg;

}

void r_w_radio(uint8_t R_W, uint8_t Reg, uint8_t Data[], int Count, uint8_t Data_Received[])
{

	if(R_W == W)
	{
		Reg = Reg + WRITE_REG; //if write, then add 0x20 to reg addr
	}

	_delay_us(10);
	PORTB &= ~(1<<CSN); // CSN goes low so the RF starts listening
	_delay_us(10);
	send_SPI(Reg); //sends command to inform which reg to read or write
	_delay_us(10);

	int i=0;
	for( i=0; i<Count; i++)
	{
		if(R_W == R && Reg != W_TX_PAYLOAD)
		{

			Data_Received[i]=send_SPI(0xFF);
			_delay_us(10);
		}
		else
		{

			send_SPI(Data[i]);
			_delay_us(10);
		}
	}
	PORTB |= (1<<CSN); // CSN back high so the RF stopps listening

}

void init_radio(uint8_t MODE, uint8_t SPEED, uint8_t D_WIDTH)
{
	//MODE- 	0- TRANSM, else REC
	//SPEED 	2- 2Mb, 1- 1Mb, else 250k
	//D_WIDTH 	1- 1byte...5- 5 bytes, else 1 byte

	_delay_ms(100);

	uint8_t data[5];
	data[0] = 1;
	//enable acknowledgment on data pipe 0
	//w�acza potwiedzenie odebrania lub wyslania danych w okre�lonym data pipe
	r_w_radio(W,EN_AA,data,1,dummy);

	data[0] = 1;
	//enables datapipes - from 0 to 5 - in this case only datapipe 0
	r_w_radio(W,EN_RXADDR,data,1,dummy);

	data[0] = 3;
	//rf address width setup - 3 to 5 bytes addresses - for 1 to 3 value
	//in this case value is 3 so address is 5 bytes long
	r_w_radio(W,SETUP_AW,data,1,dummy);

	data[0] = 1;
	//sets the rf channel frequency from 2400 to 2527 MHz, 1 Mhz per step
	//in this case value is 1 so frequency is 2401MHz
	r_w_radio(W,RF_CH,data,1,dummy);

	switch (SPEED)
	{
	case 1: 	data[0] = 0b00000110; break;
	case 2: 	data[0] = 0b00001110; break;
	default:	data[0] = 0b00000110;
	}
	//bits 2:1 set the otput power, 11 - 0db
	//bit 3 set the speed 0 - 1Mb, 1 - 2Mb
	//if bit 5 is set, then speed is anyway 250k
	r_w_radio(W,RF_SETUP,data,1,dummy);

	int i;
	for(i = 0; i < 5; i++)
	{
		data[i] = 0x12;
	}
	//sets the recive address for datapipe 0
	r_w_radio(W,RX_ADDR_P0,data,5,dummy);

	//sets the transmitter address for datapipe 0
	r_w_radio(W,TX_ADDR,data,5,dummy);

	if(D_WIDTH >= 1 && D_WIDTH <= 5)
	{
		data[0] = D_WIDTH;
	}
	else
	{
		data[0] = 1;
	}
	//sets the databytes amount: 1 to 32
	r_w_radio(W,RX_PW_P0,data,1,dummy);

	data[0] = 0b00100011;
	//7:4 sets the delay between retransmisions - 0000: 250, 0001: 500, 0010:750us etc.
	//3:0 sets the retransissions count - up to 15
	r_w_radio(W,SETUP_RETR,data,1,dummy);


	//MODE- 	0- TRANSM, else REC
	// bit 0: 1- receiver, 0- transmiter
	//bit 1: 1 power up
	//bit 2: crc encoding: 1- 2 bytes, 0- 1 byte
	//bit 3: enable crc: 1- enabled
	//bit 4: if 0, then IRQ pin ll go low if maximal retransmission count (sending error) do it in transmitter mode
	//bit 5: if 0, then IRQ pin ll go low when transmision succesfull (as transmitter)
	//bit 6: if 0, then IRQ ll go low after succesful data receive (as receiver)
	if(MODE == 0)
	{
		RadioMode = TRAN;
		data[0] = 0b00011110;
		r_w_radio(W,CONFIG,data,1,dummy);
	}
	else
	{
		RadioMode = RECE;
		data[0] = 0b00011111;
		r_w_radio(W,CONFIG,data,1,dummy);
	}

	_delay_ms(10);
}

void reset_radio()
{
	_delay_us(10);
	PORTB &= ~(1 << CSN); //CSN low so the 24L01 listens
	_delay_us(10);
	send_SPI(WRITE_REG + STATUS);
	_delay_us(10);
	send_SPI(0b01110000);
	_delay_us(10);
	PORTB |= (1 << CSN);
}

void radio_switchReceiver()
{
	uint8_t data[5];
	data[0] = 0b00011111;
	r_w_radio(W,CONFIG,data,1,dummy);
}

void radio_switchTransmiter()
{
	uint8_t data[5];
	data[0] = 0b00011110;
	r_w_radio(W,CONFIG,data,1,dummy);
}

void radio_preparePayload(uint8_t data[], uint8_t count)
{
	reset_radio();
	r_w_radio(R, FLUSH_TX,data,0,dummy);
	r_w_radio(R, W_TX_PAYLOAD,data,count,dummy);
}

void radio_transmit()
{
	PORTB |= (1 << CE); //RF starts sending
	_delay_us(200);
	PORTB &= ~(1 << CE); // RF stopps sending
}


void radio_receive(uint8_t Data_Received[],uint8_t Count)
{
	_delay_us(20);
	PORTB &= ~(1<<CE);
	r_w_radio(R,R_RX_PAYLOAD,dummy,Count,Data_Received);
	_delay_us(20);
	send_SPI(FLUSH_RX);
	_delay_us(20);
	reset_radio();
	_delay_us(20);
}




#endif /* RF24L01_H_ */

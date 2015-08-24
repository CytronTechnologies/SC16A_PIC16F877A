//==========================================================================
//	Author				: Ng Hock Yuan	
//	Project				: Sample code for SC16A using 16F877A
//	Project description	: This source code is used to control 16 servos.
//						  The 16 servos should continues rotate together from one end to another end.
//						  Delay subroutine is used to wait for servo reach the other end.    	
//  Version				: 1.1 (Add SPBRG for 20Mhz)
//==========================================================================
// This is the sample code for controlling 16 channel of servo using SC16A
/**************************************************************
*                  Communication Protocol                    *
**************************************************************
*
* UART is chosen as the interface to this module. In order to
* change the position of a servo, the host (master) needs to
* write 4 bytes of data to this module. The data format is:
* 
*        Byte 1            Byte 2             Byte 3		    Byte 4
*    --------------    ---------------    --------------    -------------
*  /  Servo channel \/    Position     \/    Position    \/   Speed value \
*  \  (0x41 - 0x60) /\  (Higher Byte)  /\  (Lower Byte)  /\      (0-63)   /
*    --------------    ---------------    --------------    -------------
*
*	Servo channel: 0b01XX XXXX
*	Higher Byte: 0b00XX XXXX
*	Lower Byte:	 0b00XX XXXX
*	Speed Value: 0b00XX XXXX	
*
* The position for the servo is in 12-bit and the valid range
* is from 0 (0.5mS) to the resolution defined below (2.5mS).
* It is the host responsibility to make sure the position will
* not stall the servo motor.
*
**************************************************************/
//	include
//==========================================================================
#include <pic.h>   					// this sample code is using 16F877A !!

//	configuration
//==========================================================================
__CONFIG ( 0x3F32 );				//configuration for the  microcontroller
											

//	define
//==========================================================================
#define sw1		RB0					//switch for start program

//	global variable
//=========================================================================



//	function prototype				(every function must have a function prototype)
//==========================================================================

void send_cmd(unsigned char num, unsigned int data, unsigned char ramp);	//UART transmit 4 bytes: servo number, higher byte position, lower byte position and speed
void delay(unsigned long data);			//delay function, the delay time
void uart_send(unsigned char data);		//UART transmit
unsigned char uart_rec(void);			//UART receive 


//	main function					(main fucntion of the program)
//==========================================================================
void main(void)
{
	unsigned int i,j;
	//set IO port for led and switch
	TRISC = 0b10000000;					//set input or output
	TRISB = 0b00000001;

	//setup USART
	BRGH = 1;					//baud rate low speed option
	SPBRG = 129;					//set boud rate to 9600bps, 64 for 10Mhz crystal; 129 for 20MHz crystal
	SPEN = 1;					//enable serial port
	RX9 = 0;					//8-bit reception
	TX9 = 0;
	CREN = 1;					//enable reception
	TXEN = 1;					//enable transmission

	for(i=0x41;i<0x51;i+=1)				//set initial position of servos
	{
	send_cmd(i,1300,0);
	}
			
	while(1)
	{
	
			if(sw1==0)							//if sw is pressed
			{
				
				while(1)						//infinity loop
				{
									
					for(i=0x41;i<0x51;i+=1)		//from channel one to sixteen
					{			
						send_cmd(i,100,28);		//send command to SC16
												//first byte is channel
												//second byte is position from 0-1463
												//last byte is the speed for each servo
					}
					
					for(j=15;j>0;j--)			//delay about 5s for all the servo reach the positon
					{
					
					delay(30000);
			
					}
					
				
					for(i=0x41;i<0x51;i+=1)
					{			
						send_cmd(i,1300,28); 	//send command to SC16
												//first byte is channel
												//second byte is position from 0-1463
												//last byte is the speed for each servo				
											
					}
					
					for(j=15;j>0;j--)			//delay about 5s for all the servo reach the positon
					{
					delay(30000);
								
					}
				}
			}
	}
		
}
	
//subroutine
//============================================================================	


void send_cmd(unsigned char num, unsigned int data, unsigned char ramp)		//send 4 bytes of command to control servo's position and speed
{
	unsigned char higher_byte=0, lower_byte=0;
	
	//position value from 0-1463 are greater than a byte
	//so needs two bytes to send
	higher_byte=(data>>6)&0x003f;	//higher byte = 0b00xxxxxx
	lower_byte=data&0x003f;			//lower byte  = 0b00xxxxxx

	
			uart_send(num);								//First byte is the servo channel 0x41-0x60
			uart_send(higher_byte);						//second byte is the higher byte of position 0b00xxxxxx
			uart_send(lower_byte);						//third byte is the lower byte of position 0b00xxxxxx
			uart_send(ramp);							//fourth byte is the speed value from 0-63 

}

void delay(unsigned long data)			//delay function, the delay time
{										//depend on the given value
	for( ;data>0;data-=1);
}
unsigned char uart_rec(void)	//receive uart value
{
	unsigned char rec_data;
	while(RCIF==0);				//wait for data
	rec_data = RCREG;
	return rec_data;			//return the received data 
}

void uart_send(unsigned char data)
{	
	while(TXIF==0);				//only send the new data after 
	TXREG=data;					//the previous data finish sent
}

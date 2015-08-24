//==========================================================================
//	Author				: Ng Hock Yuan	
//	Project				: Sample code for SC16A using 16F877A
//	Project description	: This source code is used to control 16 servos.
//						  The 16 servos will rotate from one end to another end one by one
//						  Position reporting subroutine is used to wait for servo reach the other end.  
//	Version			:1.1 (Add SPBRG for 20Mhz)  	
//
//==========================================================================
// This is the sample code for controlling 16 channel of servo using SC16A
/**************************************************************
*                  Communication Protocol                    *
**************************************************************
*
*		Position and Speed Command
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
*
*		Position Reporting Command
*
* This command is use to keep update the current position of servo.
* In order to get the feedback of current position from servo, the host needs to  
* send 2 bytes of data to SC16A.
*
* 		Byte 1            Byte 2          
*    --------------    ---------------   
*  /  Start byte    \/  Servo channel  \
*  \  '@' or 0x40   /\  (0x41 - 0x60)  /
*    --------------    ---------------   
*	Start byte: 0b0100 0000 or '@' in ASCII
*	Servo channel: 0b01XX XXXX
*
* After received this 2 bytes, SC16A will transmit the current position of requesting servo in 3 bytes
*
*  		 Byte 1            Byte 2             Byte 3		  
*    --------------    ---------------    --------------    
*  /  Servo channel \/    Position     \/    Position    \
*  \  (0x41 - 0x60) /\  (Higher Byte)  /\  (Lower Byte)  /
*    --------------    ---------------    --------------   
*	Servo channel: 0b01XX XXXX
*	Higher Byte: 0b00XX XXXX
*	Lower Byte:	 0b00XX XXXX
*
* The host needs to receive and process this 3 bytes. 
* 
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
#define sw1		RB0


//	global variable
//=========================================================================
static volatile unsigned int received_servo_position[0x11];   // Array declared to store the feedback position of servo 


//	function prototype				(every function must have a function prototype)
//==========================================================================

void send_cmd(unsigned char num, unsigned int data, unsigned char ramp); //UART transmit 4 bytes: servo number, higher byte position, lower byte position and speed
void delay(unsigned long data);				//delay function, the delay time
void uart_send(unsigned char data);			//UART transmit
unsigned char uart_rec(void);				//UART receive 

void request_feedback(unsigned char num);	//UART transmit 2 bytes: start byte '@' and servo number 0x41-0x60 to request position
void get_position(void);					//UART receive 3 bytes: servo number 0x41-0x60, higher byte and lower byte for position value to update the current position of servo

//	main function					(main fucntion of the program)
//==========================================================================
void main(void)
{
	unsigned int j;
	unsigned char i;
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
	
	for(i=0x01;i<0x11;i+=1)				//set initial position of servos
	{
	send_cmd(i,1300,0);
	}
			
	while(1)
	{
				
			if(sw1==0)
			{
				while(sw1==0);
			
				while(1)
				{
					for(i=0x01;i<0x11;i+=1)								//servo number start from 0x01 to 0x10, total 16servos
					{													//servo number will convert to 0x41 to 0x50 in send cmd routine
						send_cmd(i,200,20);								//send position and speed command to SC16A
						request_feedback(i);							//request the position to SC16A
						get_position();									//receive 3 bytes on position reporting from SC16A to get the current position
						while(received_servo_position[i]!=200)			//test if servo position reach 200 value 
						{
							request_feedback(i);						//if no yet reach, keep request
							get_position();								//receive 3 bytes on position reporting from SC16A to keep update the current position
						}
					
					}
					
					for(i=0x01;i<0x11;i+=1)
					{
						send_cmd(i,1300,20);
						request_feedback(i);
						get_position();	
						while(received_servo_position[i]!=1300)			//test if servo position reach 1300 value
						{
							request_feedback(i);						//if no yet reach, keep request
							get_position();								//receive 3 bytes on position reporting from SC16A to keep update the current position
						}
												
					}
				}//while loop
			}//if loop
			
	}//while loop
		
}//main loop
	
//subroutine
//============================================================================	


void send_cmd(unsigned char num, unsigned int data, unsigned char ramp) 	//send 4 bytes of command to control servo's position and speed
{
	unsigned char higher_byte=0, lower_byte=0;
	
	//servo channel should start with 0b01XX XXXX
	//therefore needs to change to 0x41-0x60
	num=num|0b01000000;
	
	//position value from 0-1463 are greater than a byte
	//so needs two bytes to send
	higher_byte=(data>>6)&0x003f;	//higher byte = 0b00xxxxxx
	lower_byte=data&0x003f;			//lower byte  = 0b00xxxxxx

	
			uart_send(num);								//First byte is the servo channel 0x41-0x60
			uart_send(higher_byte);						//second byte is the higher byte of position 0b00xxxxxx
			uart_send(lower_byte);						//third byte is the lower byte of position 0b00xxxxxx
			uart_send(ramp);							//fourth byte is the speed value from 0-63 

}

void request_feedback(unsigned char num)				//send command to request the current position of servo
{	
	//servo channel should start with 0b01XX XXXX
	//therefore needs to change to 0x41-0x60
		num=num|0b01000000;
		
			uart_send('@');  							//First byte is the start byte: '@' or 0x40
			uart_send(num);								//Second byte is the requsting servo channle 0x41-0x60
}

void get_position(void)									//receive 3 bytes from SC16A and update the position of servo
{unsigned int i;
static unsigned int received_servo_num=0, higher_byte=0,lower_byte=0,received_position=0;

	received_servo_num=uart_rec();															//First byte to receive: Requesting Servo number 0x41-0x60
	higher_byte=uart_rec();																	//Second byte to receive: Requesting Servo higher byte position
	lower_byte=uart_rec();																	//Third byte to receive: Requesting Servo lower byte position 
	received_servo_num=received_servo_num&0b00011111;										//Change back to 0x01-0x20
	received_servo_position[received_servo_num]=((higher_byte<<6)|(lower_byte&0x3F)); 		//Update the servo position value in corresponding array

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

void delay(unsigned long data)			//delay function, the delay time
{										//depend on the given value
	for( ;data>0;data-=1);
}

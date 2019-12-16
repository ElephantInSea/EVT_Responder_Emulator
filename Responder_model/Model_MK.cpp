#include "Model.h"
// Microcontroller code
void Model :: MK_while()
{
	// Called in One_more_step
	uc temp = 0;
	// PORT D --------------------------------------------------------------
	temp = 0x08 << d_line;
	temp |= MK_Show_ERROR ();
	PORTD = temp;
	
	// PORT C --------------------------------------------------------------
	if (d_line == led_active)	// For two iterations, the selected
	{							// indicator will be turned off
		led_blink ++;
		if (led_blink > 2)		// Delay for blinking
			led_blink = 0;
	}
	
	if (d_line > led_count) 
		temp = 0;
	else if ((d_line == led_active) && led_blink)
		temp = 0;
	else
	{
		temp = LED[(int)d_line];// The order of indicators is determined 
								// here.
		temp = Translate_num_to_LED[(int)temp];
	}

	if ((flag_send_mode == 1) && (d_line == 0))
		temp |= 0x01; // Point on the right indicator

	PORTC = temp;

	// PORT E --------------------------------------------------------------
	if (PORTE & 0x04)			//0b00000100
		flag_manual_auto = 0;	// invert
	else
		flag_manual_auto = 1;
		
	temp = (PORTE ^ 0xF8) >> 3;	// Port E is inverted
	if((d_line & 0x01) && temp > 0)	// mode
	{
		// Parity condition and nonzero reception
		temp = MK_Get_port_e(d_line);
				
		if (mode != temp)
		{
			if(mode_temp == temp)
			{
				mode_time ++;
				if (mode_time > 3)
				{
					mode = temp;
					flag_send_mode = 1;
					flag_rw = 0; //Read
					MK_Change_led_count(mode);
				}
			}
			else
			{
				mode = 255;		// Fuse
				flag_send_mode = 0;
				mode_temp = temp;
				mode_time = 0;
				led_active = 0;
				LED[0] = LED[1] = LED[2] = LED[3] = LED[4] = 0;
			}
		}	
	}
	else if ((d_line & 0x01) == 0)	//Buttons
	{
		if (temp == buttons)
		{
			if (buttons_time <= 5)	// A pressed key will work
				buttons_time ++;	// only once
				//
			if (buttons_time == 5 && buttons > 0)
				MK_Btns_action (buttons);
		}
		else 
		{
			buttons_time = 0;
			buttons = temp;
		}
	}
	
	// Send Part -----------------------------------------------------------
	if ((flag_send_mode == 1) && (mode != 255))
	{
		MK_Send_part(flag_first_launch);
		if (flag_first_launch)
			flag_first_launch = 0;
	}
		
	d_line ++;
	if (d_line > 4)
		d_line = 0;

}


void Model :: MK_Check_mail (uc mail, bool nine)
{
	// Called in MK_Handler_receiver
	while (mail)
	{
		if (mail & 0x01)
			nine = !nine;
		mail = mail >> 1;
	}
	if ((nine == 1) || (OERR || FERR))
	{
		error_code_interrupt = 1;
		CREN = 0;	//Receiver off
	}
}

void Model :: MK_Handler_receiver ()
{
	// Called in One_mode_step
	/*Reception of data in variables a, b, c, d*/
		
	if(count_receive_data == 0)
		error_code_interrupt = 0;

	// RCIF == USART receiver interrupt request flag
	while (RCIF)	
	{
		// Implementation +++
		My_recv(count_receive_data);
		// ++++++++++++++++++

		bool mail_parity = RX9D;
		uc mail = RCREG;
				
		MK_Check_mail (mail, mail_parity);

		// Implementation +++
		if ((error_code_interrupt > 0) || (count_receive_data > 3))
			RCIF = 0;
		// ++++++++++++++++++
		else
		{
			if (count_receive_data == 0)
				a = mail;
			else if (count_receive_data == 1)
				b = mail;
			else if (count_receive_data == 2)
				c = mail;
			else
				d = mail;
			count_receive_data++;
		}
	}
	
	if ((error_code_interrupt > 0) || (count_receive_data > 3))
	{
		flag_msg_received = 1;
		CREN = 0;	//Receiver off
		// Implementation +++
		OERR = FERR = 0;
		// ++++++++++++++++++
	}
	// Implementation +++
	else if ((error_code_interrupt == 0) && (count_receive_data < 4))
		RCIF = 1;
	// ++++++++++++++++++

	PEIF = 0;
	PIR1 = 0; // Just in case
}



void Model :: MK_Btns_action (uc btn)
{
	// Called in MK_while - Button part 
	uc temp = btn, count = 0;
	while(temp)
	{
		if (temp & 1)
			count ++;
		temp = temp >> 1;
	}

	// Will not work if none is pressed, or pressed more than 2 buttons
	if (count != 1)
		return;
		
	if (btn & 0x01)	// Up
	{
		LED[led_active] = LED[led_active] + 1;
		if (LED[led_active] > 9)
			LED[led_active] = 0;
	}
	else if (btn & 0x02)	// Down
	{
		if (LED[led_active] == 0)
			LED[led_active] = 10;
		LED[led_active] = LED[led_active] - 1;
	}
	else if (btn & 0x04)	// Left. LED 43210
	{
		led_active ++;
		if (led_active > led_count)
			led_active = 0;
	}
	else if (btn & 0x08)	// Right
	{
		if (led_active == 0)
			led_active = led_count + 1;
		led_active --;
	}
	else if (btn & 0x10)	// Send
	{
		if(flag_send_mode == 0)
		{
			flag_send_mode = 1;
			flag_rw = 1; //Write
		}
		else //STOP sending
			flag_send_mode = flag_rw = 0;
		//--------------------------
	}
	return;
}

void Model :: MK_Change_led_count(uc num)
{
	// Called in MK_while - Mode part
	if (num == 0)
		led_count = 2;
	else if (num == 1)
		led_count = 4;
	else if (num == 3)
		led_count = 1;
	else if (num > 7 && num < 10)	// 8, 9
		led_count = 0;
	else
		led_count = 3;
}

void Model :: MK_Check_and_correct(uc num)
{
	// Called in MK_Send
	// 10 = A, 11 = B, 15 = F
	if ((num == 7) || (num == 10) || (num == 11) || (num == 15))
	{
		error_code = 4; // Send Error
		return;
	}	
	else if (flag_rw == 0) // When reading, only the mode number is important
		return;

	int i = 0;
	int led_max = 2047;

	if (num == 0)
		led_max = 199;
	else if (num == 1)
		return;
		//led_max = 99999; // This is the maximum scoreboard
	else if (num == 2)
		led_max = 1999;
	else if (num == 3)
		led_max = 99;
	else if (num > 7 && num < 10)	// 8, 9
		led_max = 1;	
	// For 4-6 and 12-14 modes, the limit will remain 2047
	
	int led_real = 0;
	int factor = 1;

	for (i = 0; i < 5; i ++)
	{
		int j = 0, j_max = (int)LED[i];
		// led_real += factor * temp;	compilator fail
		int temp = 0;
		for (j = 0; j < j_max; j ++)
			temp += factor;
			
		led_real += temp;
		factor = factor * 10;
	}

	// If the limit is exceeded - the display will reset to the maximum value
	if (led_real > led_max)
	{
		error_code = 255; // Too large numbers on LED
		// LED[i] = (led max / (10000 / (10^i))) % 10`
		int temp = 10000;
		for (i = 4; i >= 0; i --)
		{
			factor = led_max / temp;
			factor = factor % 10;
			LED[i] = factor; 
			temp /= 10;
		}
	}
	return;
}

uc Model :: MK_Get_port_e(uc d_line)
{
	// Called in MK_while - Mode part
	// In the original, the function is easier but more confusing.
	uc temp = (PORTE ^ 0xF8) >> 3; // 0b000xxxxx
	uc temp2 = 0;

	while (temp != 0x01)
	{
		temp = temp >> 1;
		temp2 += 1;
	}

	if (d_line == 3)
	{
		temp2 += 5;
		if (temp2 > 8)
			temp2 += 3;
		else if (temp2 > 6)
			temp2 += 1;
	}

	return temp2;
}

void Model :: MK_Read_Msg()
{
	// Called in MK_Send_Part
	error_code = error_code_interrupt;

	uc temp = a >> 4;
	
	if (flag_mode_ampl == 1)
		temp -= 8;

	if (temp != mode)
		error_code = 1; 
	
	if (error_code == 0)
	{
		uc Rcv_numbers [5];
		
		Rcv_numbers[0] = Rcv_numbers[1] = 0;
		Rcv_numbers[2] = Rcv_numbers[3] = Rcv_numbers[4] = 0;
		
		// Package[1] - Package[3]
		if (temp == 8 || temp == 9)
			Rcv_numbers[0] = b & 0x01;
		else
		{
			Rcv_numbers[4] = b & 0x0F;
			Rcv_numbers[3] = c >> 4;
			Rcv_numbers[2] = c & 0x0F;
			Rcv_numbers[1] = d >> 4;
			Rcv_numbers[0] = d & 0x0F;
		}

		for (temp = 0; temp < 5; temp ++)
		{
			if (error_code == 0)
			{
				uc temp2 = Rcv_numbers[temp];	// Bugs and features of the compiler
				if (flag_rw == 0)
					LED[temp] = temp2;
				else if (LED[temp] != temp2)
					error_code = 1;
			}
		}

		if (b & 0x40)	// Alarm signal
			error_code = 3;
	}
	return ;
}

void Model :: MK_Reg_Start_up()
{
	// Called in main - Responder_mode.cpp file
	GLINTD = 1;		// Disable All Interrupts
	PORTE = 0x00;	// Getting button codes and modes
	DDRE  = 0x00;
	PORTC = 0x00;	// Numbers on the scoreboard cell
	DDRC  = 0x00;
	PORTD = 0x00;	// Power for indicator and button polling
	DDRD  = 0x00;
	
	// Start signal. Only needed for layout
	DDRE = 0;
	PORTE = 0x2E; // 0b00101110
	
	for (a = 0; (int)a < 255; a += 1)
		for (b = 0; (int)b < 255; b += 1);

	a = b = 0;	
	
	PIR1    = 0x00;	// Reset Interrupt Request Flags
	PIE1    = 0x01;	// RCIE setting: USART receiver interrupt enable bit 
					// (there is data in the receiver buffer)
	T0STA   = 0x28;	// Switching on TMR0 (internal clock frequency, 1:16 pre-selector)
	// T0STA does not matter since interruptions not allowed
	INTSTA  = 0x08;	// PEIE setting
	
	TXSTA = 0x42;	// 0b01000010 9bit, asynchronous,
	RCSTA = 0x90;	// 0b10010000 on port, 9bit, continuous reception
	SPBRG = 0x9B;	// 155
	USB_CTRL = 0x01;	// USB launch. Low Speed (1.5 Ìבטע/c),
	
	GLINTD  = 0; // Reset All Interrupt Disable Bit
	CREN = 0;
	
	DDRE = 0xFC; 	// 0b11111100 Buttons * 5 and MANUAL/AUTO
	
	PORTE = 0;
    
	// Flags
	flag_manual_auto = 0;
	flag_mode_ampl = 0;
    flag_msg_received = 0;	// Flag of received message
    flag_rw	= 0;
    flag_send_mode = 1;	// Turn on to receive data
	
	// Variables
	LED[0] = LED[1] = LED[2] = LED[3] = LED[4] = 0;
    a = b = c = d = 0;
    count_receive_data = 0;
    error_code = 0;
	error_code_interrupt = 0;
	led_active = 0;	// The number of the selected indicator. 
					// 4 is the far left
	led_count = 2;
    mode = 0;
}

void Model :: MK_Send()
{
	// Call from MK_Send_part()
	uc Package [4], temp = 0;
	
	// Receiver ON
	CREN = 1;
	count_receive_data = 0; // In case of loss of parcels, or line break
	a = b = c = d = 0;
	flag_msg_received = 0;
	
	//Package [0]
	Package[0] = mode;
	
	if ((Package[0] > 3) && (Package[0] < 7)) 
	{
		if ((flag_mode_ampl == 0) && (LED[3] == 9))
			flag_mode_ampl = 1;
	}

	if (flag_mode_ampl == 1)
	{
		if ((Package[0] < 4) || (Package[0] > 6))
			flag_mode_ampl = 0;
		else
		{
			Package[0] += 8;
			flag_rw = 0;
		}
	}
	
	// the mode is greater than 13, or does 
	// not fit into the limits for the mode

	MK_Check_and_correct(Package[0]);
	
	if (error_code > 3)
	{
		flag_msg_received = 1;
		return ;
	}

	if (flag_rw == 0) // Read
		Package[1] = Package[2] = Package[3] = 0;
	else //Write
	{
		if (Package[0] == 8 || Package[0] == 9)
		{	
			Package[1] = LED[0];
			Package[2] = Package[3] = 0;
		}
		else
		{
			Package[1] = LED[4];
			Package[2] = (LED[3] << 4) | LED[2];
			Package[3] = (LED[1] << 4) | LED[0];
		}

		Package[1] |= 0x80;
	}
	
	if (flag_manual_auto)
		Package[1] |= 0x20;
	
	Package[0] = (Package[0] << 4) | 0x0F;
	
	int i = 0, max = 4;
	temp = 0;
	
	// Implementation +++
	TXIF = 1;
	// ++++++++++++++++++

	while ((i < max) && (temp < 10)) 
	{	
		// Implementation +++
		// key 7 "Send error"
		if(GetAsyncKeyState(0x37))
			TXIF = 0;
		else
			TXIF = 1;
		// ++++++++++++++++++
		
		if (i == 4)
			TXEN = 0; // Transmitter Turn Off
		else if (TXIF == 1)	// TXIF or TRMT.
		{
			bool parity = 0;
			int t = (int)Package[i];
			while (t)
			{
				if (t & 0x01)
					parity = !parity;
				t = t >> 1;
			}
			TX9D = parity; //1
			
			TXREG = Package[i];
			TXEN = 1; // Transmitter Turn On

			// Implementation +++
			My_send (i);
			// ++++++++++++++++++

			i++;
		}
		else
			temp ++;	// fuze
	}
	
	
	if (i != max) // Sent more or less
		error_code = 4; 
	return ;
}

void Model :: MK_Send_part(bool flag_first_launch)
{
	// Called in MK_while - In the end
	static uc i;
	static uc j;
	
	j ++;
	if (j > 2)
	{
		j = 0;
		i ++;
	}
	
	if (((i == 0) && (j == 1)) || (flag_first_launch == 1))
	{
		MK_Send();
		
		if (error_code == 255)  // Too large numbers on LED
		{
			error_code = 0;
			// Reset of dispatch without consequences
			i = j = 0;
			flag_send_mode = 0;
		}
	}
	else if ((i == 3) || ((flag_msg_received == 1) && (flag_mode_ampl == 0)))
	{
		i = j = 0;
		if (flag_msg_received == 1)
		{
			MK_Read_Msg();
			
			// Implementation +++
			flag_msg_received = 0;
			if (flag_mode_ampl == 0)
			{
				if (error_code == 0)
					flag_send_mode = 0;
			}
			// ++++++++++++++++++
		}
		else if (error_code != 4)
			error_code = 2; // Line break
	}
}

uc Model :: MK_Show_ERROR()
{
	// Called in MK_while - At the beginning
	static uc i; // time_show_0;
	static uc j; // time_show_1;
	uc work_led = 0x02;	// 0x02 work; 0x01 error
	
	j++;

	if (j == 2)
	{
		j = 0;
		i ++;		
	}
	
	if(error_code == 0)			// Work
		i = j = 0;
	else if (error_code == 1)	// Parity
	{
		if (i < 2)
			work_led = 0x01; 
	}
	else if(error_code == 2)	// Line is broken
	{
		if (i < 5)
			work_led = 0x01;
	}
	else if(error_code == 3)	// 12 mode, Error
		work_led = 0x01;
	else if(error_code == 4)	// Send Error
	{
		if (i < 5)
			work_led = 0x01;
		else if((i > 5) && (i < 9))
			work_led = 0x01;
	}

	if (i == 10)
		i = 0;
	
	// Implementation +++
	if (work_led == 0x02)
		flag_led_work = true;
	else 
		flag_led_work = false;
	// ++++++++++++++++++

	return work_led;
}

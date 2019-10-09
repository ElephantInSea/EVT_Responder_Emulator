#include "Model.h"


Model::Model (void)
{
	PORTC = PORTD = PORTE = '0';
	DDRC = DDRD = DDRE = '0';
	Set_led();
	MK_Reg_Start_up();
	Variable_Start_up();
}

Model :: Model(
	char portC, char ddrC, 
	char portD, char ddrD, 
	char portE, char ddrE): 
	PORTC('2'), PORTD ('3'), PORTE ('0'), DDRC('2'), DDRD ('3'), DDRE ('0')
{
	PORTC = portC;
	DDRC = ddrC;
	PORTD = portD;
	DDRD = ddrD;
	PORTE = portE;
	DDRE = ddrE;
	

	Set_led();
	
	MK_Reg_Start_up();
	Variable_Start_up();
}


Model :: ~Model(void)
{
}


void Model :: Interrupts()
{
	if (RCIF == 1)
	{
		MK_Handler_receiver();
	}
}

void Model :: One_mode_step()
{
	Interrupts();
	Set_PortE();
	MK_main();
}

void Model :: Show_Indications()
{
	std :: cout << "LED: = " << Get_led2() << std ::endl;
	std :: cout << "Error: = " << Get_Error_status() << std :: endl; // '.'
	std :: cout << "Mode: = " << Get_binary_format (mode) << std :: endl;
	std :: cout << "Send = " << std :: endl;
	std :: cout << "Recv = " << Get_binary_format (RCSTA) << std :: endl;
	std :: cout << (flag_manual_auto ? "MANUAL" : "AUTO") << std :: endl;
	std :: cout << "PORTC = " << Get_binary_format (PORTC) << std ::endl; 
	std :: cout << "PORTD = " << Get_binary_format (PORTD) << std ::endl; 
	std :: cout << "PORTE = " << Get_binary_format (PORTE ^ 0xFF) << std ::endl; 
}


void Model :: My_send (int count)
{
	Message[count][0] = TXREG;
	Message[count][1] = (uc) TX9D;
	if (count == 3)
	{
		RCIF = 1;
	}

}

void Model :: My_recv (uc count)
{
	RCREG = Message [count][0];
	RX9D = (bool) Message [count][1];
}
	


std :: string Model :: Get_Error_status()
{
	std :: string ans = "";
	if (error_code == 0)
		ans += "Normal";
	else if (error_code == 1)
		ans += "Parity issues";
	else if (error_code == 2)
		ans += "Line is broken";
	else if(error_code == 3)
		ans += "Error signal";
	else
		ans += "Send Error";
	return ans;
}

std :: string Model :: Get_binary_format(uc target)
{
	std :: string answer = "0b";
	for(int i(0); i < 8; i ++)
	{
		if (target & 0x80)
			answer += "1";
		else
			answer += "0";
		target = target << 1;
	}
	return answer;
}

std :: string Model :: Get_led2()
{
	std :: string answer = "";
	uc temp = d_line == 0 ? 4 : (d_line - 1);
	static bool blink = true;
	int num = 0;
	for (int i(0); i < 5; i++)
	{
		if ((i == (int)led_active) && (blink))
			answer += '_';
		else
			answer += 48 + LED[i];

	}
	blink = !blink;
	return answer;
}

std :: string Model :: Get_led()
{
	std :: string answer = "";
	int num = 0;

	for (int i(0); i < 11; i++)
		if (PORTC == Translate_num_to_LED[i])
		{
			num = i;
			break;
		}
	

	uc temp = d_line == 0 ? 4 : (d_line - 1);
	for (int i(0); i < 5; i ++)
	{
		if (i == temp)
		{
			if (num == 10)
				answer += '_';
			else
				answer += 48 + num;
		}
		else
			answer += '_';
		answer += ' ';
	}
	/*
	uc temp = d_line == 0 ? 4 : (d_line - 1);

	for (int i(0); i < 5; i ++)
	{
		if (i == temp)
			answer += LED_model[i];
		else
			answer += '_';
		answer += ' ';
	}*/
	return answer;
}

void Model :: Set_led()
{
	for (int i(0); i < 5; i ++)
		LED_model[i] = '0';
}

void Model :: Set_PortE()
{
	PORTE = 0;
	static uc keys_work = 0;
	static uc keys_temp = 0;
	static uc keys_time = 0;
	uc keys = 0;
	// Коды клавиш
	//https://docs.microsoft.com/ru-ru/windows/win32/inputdev/virtual-key-codes?redirectedfrom=MSDN
	if (d_line == 1)
	{
		if(GetAsyncKeyState(0x51)) // Q
		{
//			unsigned short t = SetTimer(0, 0, 1000, NULL);
			keys |= 0x01;
		}
		if(GetAsyncKeyState(0x57)) // W
		{
//			KillTimer(0, 0);
			keys |= 0x02;
		}
		if(GetAsyncKeyState(0x45)) // E
			keys |= 0x04;
		if(GetAsyncKeyState(0x52)) // R
			keys |= 0x08;
		if(GetAsyncKeyState(0x54)) // T
			keys |= 0x10;
	}
	else if (d_line == 3)
	{
		if(GetAsyncKeyState(0x41)) // A
			keys |= 0x01;
		if(GetAsyncKeyState(0x53)) // S
			keys |= 0x02;
		if(GetAsyncKeyState(0x44)) // D
			keys |= 0x04;
		if(GetAsyncKeyState(0x46)) // F
			keys |= 0x08;
		if(GetAsyncKeyState(0x47)) // G
			keys |= 0x10;
	}
	else
	{
		if(GetAsyncKeyState(0x26)) // Up
			keys |= 0x01;
		if(GetAsyncKeyState(0x28)) // Down
			keys |= 0x02;
		if(GetAsyncKeyState(0x25)) // Left
			keys |= 0x04;
		if(GetAsyncKeyState(0x27)) // Right
			keys |= 0x08;
		if(GetAsyncKeyState(0x20)) // Space / Send
			keys |= 0x10;
	}

	if ((d_line == 1) || (d_line == 3))
	{
		if (keys > 0)
		{
			if (d_line == 3)
				keys |= 0x80;

			if (keys == keys_temp)
			{
				keys_time += keys_time < 4 ? 1 : 0;
				if (keys_time == 4)
					keys_work = keys_temp;
			}
			else
			{
				keys_temp = keys;
				keys_work = keys_time = 0;
			}
		}
		if ((d_line == 3) && (keys_work & 0x80))
			PORTE = keys_work & 0x7F; 
			// Снимается флаг о второй линии
		else if ((d_line == 1) && ((keys_work & 0x80) == 0))
			PORTE = keys_work;
		else
			PORTE = 0;
	}
	else
		PORTE = keys;
	PORTE = PORTE << 3;

	
	if(GetAsyncKeyState(0x31)) //1 - Manual-Auto
		PORTE |= 0x04;

	// If there is an inversion
	PORTE ^= 0xFF;
}

void Model :: Transmission_emulator()
{
	RCSTA ++;
	RCSTA %= 100;
}

void Model :: Variable_Start_up()
{
	// Здесь устанавливаются переменные которые устанавливались
	// перед мейном, но тут эти переменные перестали быть локальными.
	// Плюс всякие переменные нужные для эмулятора
	// Setting local variables
	main_temp = 0;
    
	LED[0] = LED[1] = LED[2] = LED[3] = LED[4] = 0;
	
    flag_send_mode = 0;		// Turn on to receive data
    flag_rw = 0;
	led_active = 4;	// The number of the selected indicator. 
					// 4 is the far left
    mode = 255;
    
    count_receive_data = 0;
    a = b = c = d = 0;
    flag_msg_received = 0;	// Flag of received message
    error_code = 0;

    send_mode_count = 0;
    send_error_count = 0;
    
	d_line = 0;
	d_work_light = 0;
    flag_first_launch = 1;
	
    led_blink = 0;
    
    uc mode_temp = 0, mode_time = 0;
    uc buttons = 0, buttons_time = 0;
	
	TXREG = 0;
	TX9D = TXEN = TXIF = 0;
	RCIF = 0;
	OERR = FERR = 0;

	// Переменные для эмулятора
	for (int i(0); i < 4; i ++)
		Message[i][0] = Message[i][0] = 0;

	for (int i(0); i < 12; i++)
	{
		Respondent[i] = "0x";
		Respondent[i] += "01234567";
	}

}
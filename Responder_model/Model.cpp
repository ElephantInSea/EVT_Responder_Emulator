#include "Model.h"


Model::Model (void)
{
	PORTC = PORTD = PORTE = '0';
	DDRC = DDRD = DDRE = '0';

	Set_led();
	Variable_Start_up_emulator();
	Variable_Start_up_local();
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
	Variable_Start_up_emulator();
	Variable_Start_up_local();
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
	std :: cout << "Send = " << Send_Message << std :: endl;
	std :: cout << "Recv = " << Recv_Message << std :: endl; // Get_binary_format (RCSTA)
	std :: cout << (flag_manual_auto ? "MANUAL" : "AUTO") << std :: endl;
	std :: cout << "PORTC = " << Get_binary_format (PORTC) << std ::endl; 
	std :: cout << "PORTD = " << Get_binary_format (PORTD) << std ::endl; 
	std :: cout << "PORTE = " << Get_binary_format (PORTE ^ 0xFF) << std ::endl; 
	std :: cout << "Count send: " << count_send_emulator << std :: endl;
	std :: cout << "Work : " << (flag_led_work ? "O" : " ") << std :: endl;
	std :: cout << "ERROR: " << (flag_led_work ? " " : "O") << std :: endl;
}


void Model :: My_send (int count)
{
	Message[count][0] = TXREG;
	Message[count][1] = (uc) TX9D;
	if (count == 3)
	{
		// key 3 "Line is broke"
		if(!GetAsyncKeyState(0x33))
			RCIF = 1;
		Send_Message = Get_str_send(Message[0][0],
			Message[1][0], Message[2][0], Message[3][0]);
		Respondent_work (Send_Message);
		count_send_emulator ++;
	}
}

void Model :: My_recv (uc count)
{
	RCREG = Message [count][0];
	RX9D = (bool) Message [count][1];
}
	

void Model :: My_recv2 (uc count)
{
	RCREG = 0;
	if (count > 3)
		return;
	// 01234567890123
	// 0x 12 34 56 78
	uc part = Recv_Message[3 * count + 3] - 48;
	if (part > 10) 
		part -= 7;
	//uc t2 = Recv_Message[3 * count + 3];
	RCREG = part << 4;
	//t2 = Recv_Message[3 * count + 4];
	part = Recv_Message[3 * count + 4] - 48;
	if (part > 10) 
		part -= 7;

	RCREG |= part;

	bool parity = 0;
	int t = (int)RCREG;
	while (t)
	{
		if (t & 0x01)
			parity = !parity;
		t = t >> 1;
	}
	// key 4 "Parity error"
	if ((count == 3) && GetAsyncKeyState(0x34))
		parity = !parity;
	if (GetAsyncKeyState(0x35))
		OERR = 1;
	if (GetAsyncKeyState(0x36))
		FERR = 1;

	RX9D = parity; //1
	
	if (count == 2 || count > 2)
		RCIF = 0;
}

void Model :: Respondent_work (std :: string income_msg)
{
	// 01234567890123
	// 0x 12 34 56 78
	int part = income_msg[3] - 48;
	if (part > 9) 
		part -= 7;
	if (part == 12)
		part = 7;
	else if (part > 12)
		part -= 3;
	Recv_Message = "0x ";

	//Recv_Message += part + ((part < 8) ? 48 : 55);
	Recv_Message += income_msg[3];
	Recv_Message += income_msg[4];
	Recv_Message += ' ';

	
	uc b2 = income_msg[6] - 48;
	if (b2 > 9) 
		b2 -= 7;
		//Recv_Message += income_msg[6];
	// key 2.
	if (GetAsyncKeyState(0x32))
		b2 |= 0x04;
	//error |= GetAsyncKeyState(0x32) ? 0x40 : 0;
	b2 += (b2 < 10) ? 48 : 55;
	Recv_Message += b2;

	//Recv_Message += income_msg[6] | (GetAsyncKeyState(0x32) ? 0x40 : 0);
	
	if (b2 & 0x08)	// write
	{
		Respondent[part] = income_msg[7];
		Respondent[part] += income_msg[9];
		Respondent[part] += income_msg[10];
		Respondent[part] += income_msg[12];
		Respondent[part] += income_msg[13];
	}

	Recv_Message += Respondent[part][0];
	char t = Respondent[part][0];
	for (int i(0); i < 2; i ++)
	{
		Recv_Message += ' ';
		Recv_Message += Respondent[part][2 * i + 1];
		Recv_Message += Respondent[part][2 * i + 2];
		t = Respondent[part][2 * i + 1];
	}
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

std :: string Hex_to_str (uc target)
{
	std :: string ans = "";
	uc temp = (target >> 4) & 0x0F;
	temp += (temp < 10) ? 48 : 55;
	ans += temp;
	temp = target & 0x0F;
	temp += (temp < 10) ? 48 : 55;
	ans += temp;
	// ans += temp + ((temp < 8) ? 48 : 57);
	return ans;
}

std :: string Model :: Get_str_send(uc A, uc B, uc C, uc D)
{
	std :: string ans = "0x ";
	ans += Hex_to_str(A) + ' ';
	ans += Hex_to_str(B) + ' ';
	ans += Hex_to_str(C) + ' ';
	ans += Hex_to_str(D);
	//uc temp = A >> 4, temp2 = A & 0x0F;
	//temp += temp < 8 ? 48 : 57;

	return ans;
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


void Model :: Variable_Start_up_emulator()
{
	TXREG = 0;
	TX9D = TXEN = TXIF = 0;
	RCIF = 0;
	OERR = FERR = 0;

	for (int i(0); i < 4; i ++)
		Message[i][0] = Message[i][0] = 0;

	for (int i(0); i < 12; i++)
	{
		//Respondent[i] = "12345";
		Respondent.push_back ("12345");
	}
	Send_Message = "Empty";
	Recv_Message = "Empty";

	count_send_emulator = 0;
	flag_led_work = 0;
}

void Model :: Variable_Start_up_local()
{
	// Здесь устанавливаются переменные которые устанавливались
	// перед мейном, но тут эти переменные перестали быть локальными.
	// Плюс всякие переменные нужные для эмулятора
	// Setting local variables
	main_temp = 0; // Нужно ли еще?
    
    send_mode_count = 0;	// Send iteration
    send_error_count = 0;	

	d_line = 0;
	d_work_light = 0;
    flag_first_launch = 1;
	
    led_blink = 0;
    
    uc mode_temp = 0, mode_time = 0;
    uc buttons = 0, buttons_time = 0;
}
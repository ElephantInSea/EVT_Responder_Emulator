#include "Model.h"


Model::Model (void)
{
	PORTC = PORTD = PORTE = '0';
	DDRC = DDRD = DDRE = '0';

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
	

	Variable_Start_up_emulator();
	Variable_Start_up_local();
}


Model :: ~Model(void)
{
}



void Model :: One_mode_step()
{
	// Called in Responder_model
	
	if (RCIF == 1)
		MK_Handler_receiver();
	Set_PortE();
	MK_while();
}

void Model :: Show_Indications()
{
	// Called in Responder_model
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


std :: string Model :: Get_Error_status()
{
	// Called in Show_Indications
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
	// Called in Show_Indications
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
	// Called in Show_Indications
	std :: string answer = "";
	static bool blink = true;
	for (int i(0); i < 5; i++)
	{
		if ((i == (int)4 - led_active) && (blink))
			answer += '_';
		else if (i < (int)(4 - led_count))
			answer += '_';
		else
			answer += 48 + LED[4 - i];
	}
	blink = !blink;
	return answer;
}

std :: string Model :: Get_led()
{
	// The function is close to reality, but less convenient. 
	// I'm using the second version
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
	return answer;
}



std :: string Hex_to_str (uc target)
{
	// Called in Get_str_send
	std :: string ans = "";
	uc temp = (target >> 4) & 0x0F;
	temp += (temp < 10) ? 48 : 55;
	ans += temp;
	temp = target & 0x0F;
	temp += (temp < 10) ? 48 : 55;
	ans += temp;

	return ans;
}

std :: string Model :: Get_str_send(uc A, uc B, uc C, uc D)
{
	// Called in My_send
	std :: string ans = "0x ";
	ans += Hex_to_str(A) + ' ';
	ans += Hex_to_str(B) + ' ';
	ans += Hex_to_str(C) + ' ';
	ans += Hex_to_str(D);
	return ans;
}

void Model :: My_send (int count)
{
	// Called in MK_Send
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
	// Called in MK_Handler_receiver
	RCREG = 0;
	if (count > 3)
		return;
	// 01234567890123
	// 0x 12 34 56 78
	uc part = Recv_Message[3 * count + 3] - 48;

	if (part > 10) 
		part -= 7;
	
	RCREG = part << 4;
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

	RX9D = parity;
	
	if (count == 2 || count > 2)
		RCIF = 0;
}

void Model :: Respondent_work (std :: string income_msg)
{
	// Called in My_send
	// 01234567890123
	// 0x 12 34 56 78 - example of the contents of the 
	// variable income_msg, type string
	int part = income_msg[3] - 48;

	// In case of amplitude mode
	if (part > 9)
		part -= 7;
	
	if (part == 12)		// This code is needed because I decided to
		part = 7;		// save a place and an array of the respondent 
	else if (part > 12)	// for 14 places instead of 16
		part -= 3;
	
	Recv_Message = "0x ";
	Recv_Message += income_msg[3];
	Recv_Message += income_msg[4];
	Recv_Message += ' ';
	
	uc h_half_sec_byte = income_msg[6] - 48; // high half second byte

	if (h_half_sec_byte > 9) 
		h_half_sec_byte -= 7;
		
	if (GetAsyncKeyState(0x32)) // key 2.
		h_half_sec_byte |= 0x04;
	
	h_half_sec_byte += (h_half_sec_byte < 10) ? 48 : 55;

	Recv_Message += h_half_sec_byte;
		
	if (h_half_sec_byte & 0x08)	// write
	{
		Respondent[part] = income_msg[7];
		Respondent[part] += income_msg[9];
		Respondent[part] += income_msg[10];
		Respondent[part] += income_msg[12];
		Respondent[part] += income_msg[13];
	}

	Recv_Message += Respondent[part][0];
	for (int i(0); i < 2; i ++)
	{
		Recv_Message += ' ';
		Recv_Message += Respondent[part][2 * i + 1];
		Recv_Message += Respondent[part][2 * i + 2];
	}
}

void Model :: Set_PortE()
{
	// Called in One_mode_step
	PORTE = 0;
	static uc keys_work = 0;
	static uc keys_temp = 0;
	static uc keys_time = 0;
	uc keys = 0;
	// ���� ������
	//https://docs.microsoft.com/ru-ru/windows/win32/inputdev/virtual-key-codes?redirectedfrom=MSDN
	if (d_line == 1)
	{
		if(GetAsyncKeyState(0x51)) // Q
			keys |= 0x01;
		if(GetAsyncKeyState(0x57)) // W
			keys |= 0x02;
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
			// ��������� ���� � ������ �����
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

void Model :: Variable_Start_up_emulator()
{
	// Called in constructor
	for (int i(0); i < 4; i ++)
		Message[i][0] = Message[i][0] = 0;

	for (int i(0); i < 12; i++)
		Respondent.push_back ("12345");
	
	Recv_Message = "Empty";
	Send_Message = "Empty";

	count_send_emulator = 0;
	flag_led_work = 0;

	RCIF = TXREG = RCREG = 0;	// Registers and bits whose values 
	TX9D = TXEN = TXIF = 0;		// are set by MK itself.
	RX9D = OERR = FERR = 0;
}

void Model :: Variable_Start_up_local()
{
	// Called in constructor
	// Here variables are set that are local to the main,
	// but here are class data

	d_line = 0;
    flag_first_launch = 1;
    led_blink = 0;
    
    uc mode_temp = 0, mode_time = 0;
    uc buttons = 0, buttons_time = 0;
}
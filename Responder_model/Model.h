#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

typedef unsigned char uc;
const uc Translate_num_to_LED[11] = {
//  0,	  1,	2,	  3,	4,	  5,	6,	  7,	8,	  9,    _.
	0xC0, 0xF9, 0xA4, 0xB0, 0x98, 0x92, 0x82, 0xF8, 0x80, 0x90, 0x00};

class Model
{
public:
	
	bool GLINTD;
	uc PIR1, PIE1, T0STA, INTSTA;
	bool PEIF;
	uc TXSTA, RCSTA, SPBRG, USB_CTRL;
	uc TXREG, RCREG;
	bool CREN, TX9D, TXEN, TXIF;
	bool RCIF, RX9D, FERR, OERR;
	uc PORTC, PORTD, PORTE;
	uc DDRC, DDRD, DDRE;
	uc LED_model [5];

	// Global variables
	bool flag_send_mode;
    bool flag_rw;
    bool flag_msg_received;
	bool flag_manual_auto;
	uc LED[5];
	uc a, b, c, d;
	uc led_active;
    uc mode;
    uc count_receive_data;
    uc error_code;
	uc error_code_interrupt;

	// Local variables for an infinite loop in main
	uc main_temp;
    int send_mode_count;
    int send_error_count;	
	int d_line;	
	uc d_work_light; 
    int led_blink;
    uc mode_temp, mode_time;
    uc buttons, buttons_time; 
    bool flag_first_launch;

	//std :: string Respondent [12];
	std :: vector<int> asd;
	std :: vector <std :: string> Respondent;
	uc Message [4][2];
	std :: string Send_Message;
	std :: string Recv_Message;
	int count_send_emulator;
	bool flag_led_work;

public:
	Model (void);
	Model (
	char portC, char ddrC = '1', 
	char portD = '1', char ddrD = '1', 
	char portE = '1', char ddrE = '1');
		//(char, char, char, char, char, char);

	~Model(void);
	
	// Model code
	
	void Interrupts();
	void One_mode_step();
	void Show_Indications();

	void My_send (int);
	void My_recv (uc);
	void My_recv2 (uc);

	void Respondent_work (std :: string);

	std :: string Get_Error_status();
	std :: string Get_binary_format(uc ); 
	std :: string Get_led2();
	std :: string Get_led();
	std :: string Get_str_send(uc, uc, uc, uc);

	void Set_led ();
	void Set_PortE();
	void Transmission_emulator();
	void Variable_Start_up_emulator();
	void Variable_Start_up_local();

	// Microcontroller code - in in the file Model_MK.cpp
	void MK_main();

	void MK_Check_mail (uc mail, bool nine);
	void MK_Handler_receiver ();
	
	bool MK_Check(uc num);
	void MK_Btns_action (uc btn);
	void MK_Read_Msg();
	void MK_Reg_Start_up();
	void MK_Send();
	void MK_Send_part(bool flag_first_launch);
	uc MK_Show_ERROR();

	uc MK_Get_port_e(uc d_line);
};


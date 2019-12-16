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
	// Registers and bits customizable in MK_Reg_Start_up 
	bool GLINTD;
	uc PORTC, PORTD, PORTE;
	uc DDRC, DDRD, DDRE;
	uc PIR1, PIE1, T0STA, INTSTA;
	uc TXSTA, RCSTA, SPBRG, USB_CTRL;
	bool CREN;

	// Registers and bits are not configurable when starting
	// MK, or read-only. Installed in Variable_Start_up_emulator
	bool PEIF;
	uc TXREG, RCREG;
	bool TX9D, TXEN, TXIF;
	bool RCIF, RX9D, FERR, OERR;


	// Variables global in the original code
	bool flag_manual_auto;
	bool flag_mode_ampl;
    bool flag_msg_received;
    bool flag_rw;
	bool flag_send_mode;

	uc LED[5];
	uc a, b, c, d;
    uc count_receive_data;
    uc error_code;
	uc error_code_interrupt;
	uc led_active;
	uc led_count;
    uc mode;

	// Local variables for an infinite loop in main
	int d_line;	
    bool flag_first_launch;
    int led_blink;
	// uc led_blink_temp; // No need in the emulator - blinking so slow
    uc mode_temp, mode_time;
    uc buttons, buttons_time;

	// Variables needed only for this emulator
	uc Message [4][2];	// 4 bytes of messages, plus parity bits
	std :: vector <std :: string> Respondent;
	std :: string Recv_Message;
	std :: string Send_Message;
	int count_send_emulator;
	bool flag_led_work;
	

public:
	Model (void);
	Model (
	char portC, char ddrC = '1', 
	char portD = '1', char ddrD = '1', 
	char portE = '1', char ddrE = '1');

	~Model(void);
	
	// Model code - in in the file Model.cpp
	void One_mode_step();
	void Show_Indications();


	// Part of Show_Indications()
	std :: string Get_Error_status();
	std :: string Get_binary_format(uc ); 
	std :: string Get_led2();
	std :: string Get_led();

	// Other
	std :: string Get_str_send(uc, uc, uc, uc);
	void My_send (int);
	void My_recv (uc);
	void Respondent_work (std :: string);
	void Set_PortE();
	void Variable_Start_up_emulator();
	void Variable_Start_up_local();

	// Microcontroller code - in in the file Model_MK.cpp
	void MK_while();

	void MK_Check_mail (uc mail, bool nine);
	void MK_Handler_receiver ();
	
	void MK_Btns_action (uc btn);
	void MK_Change_led_count(uc num);
	void MK_Check_and_correct(uc num);
	uc MK_Get_port_e(uc d_line);
	void MK_Read_Msg();
	void MK_Reg_Start_up();
	void MK_Send();
	void MK_Send_part(bool flag_first_launch);
	uc MK_Show_ERROR();
};


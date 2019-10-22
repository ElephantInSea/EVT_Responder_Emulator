#include <iostream>
#include <Windows.h>
#include "Model.h"

using namespace std;
int main()
{
	setlocale(LC_ALL, "rus");
	Model Responder('1');
	Responder.MK_Reg_Start_up();


	while (GetAsyncKeyState(VK_ESCAPE) == 0)
	{
		system("cls");
		/*
		//Model Responder('1', '1', '1', '1', '1', '1');
		cout << "Hello World" << endl;
		cout << "PORTE = " << Responder.PORTE << endl;
		cout << "PORTC = " << Responder.PORTC << endl;
		cout << "PORTD = " << Responder.PORTD << endl;*/
		Responder.One_mode_step();
		Responder.Show_Indications();
		//Sleep (10);
	}
	//system("pause");
	return 0;
}
// https://myzcloud.me/artist/107133/clark/albums
// https://music.yandex.ru/artist/29081
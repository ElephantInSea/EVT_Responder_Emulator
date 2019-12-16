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
		Responder.One_mode_step();
		Responder.Show_Indications();
	}
	Responder.~Model();
	//system("pause");
	return 0;
}
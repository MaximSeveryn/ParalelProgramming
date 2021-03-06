#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#pragma warning(disable: 4996)//приберемо помилку 4996

SOCKET Connection;

void ClientHandler() {
	int msg_size;
	while (true) {//отримуємо результат від сервера (в іншому потоці)
		recv(Connection, (char*)&msg_size, sizeof(int), NULL);
		char* msg = new char[msg_size + 1];
		msg[msg_size] = '\0';
		recv(Connection, msg, msg_size, NULL);
		std::cout <<"\n\t"<< msg << std::endl;
		delete[] msg;
	}
}

int main(int argc, char* argv[]) {
	//WSAStartup
	WSAData wsaData;//підключення бібліотек до структури таке саме, як і на сервері
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;//бібліотеки не підключились до структури
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	Connection = socket(AF_INET, SOCK_STREAM, NULL);
	if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Error: failed connect to server.\n";
		return 1;
	}
	std::cout << "Connected!\n";

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);

	string msg1;
	while (true) {//рекурсивне меню, щоб мати чіткі сигнали від клієнта
		cout<<"\n\tMenu\n\t1 - Enter number of threads to build Index\n\t2 - Enter any word to find"<<endl;
		std::getline(cin, msg1);

		stringstream geek(msg1);
		int action;//надішлемо команду серверу (число або рядок)
		geek >> action;

		if (action) {
			int msg_size = msg1.size();

			send(Connection, (char*)&msg_size, sizeof(int), NULL);
			send(Connection, msg1.c_str(), msg_size, NULL);
			Sleep(10); 	//пауза після кожної команди обов'язкова
		}
		else {
			int msg_size = msg1.size();

			send(Connection, (char*)&msg_size, sizeof(int), NULL);
			send(Connection, msg1.c_str(), msg_size, NULL);
			Sleep(10); 
		}
	}
	system("pause");
	return 0;
}
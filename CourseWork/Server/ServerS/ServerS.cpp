#include "stdafx.h"
#pragma comment(lib, "ws2_32.lib")//
#include <winsock2.h>//для роботи з сокетами
#include <iostream>
#include <string>
#include<sstream>
using namespace std;
#pragma warning(disable: 4996)

#include<vector>
#include<map>
#include<fstream>
#include<regex>
#include <thread>
#include <mutex>

class InvertedIndex
{
	map<string, vector<string> > Dictionary;
	map<string, mutex> M; //набір унікальних м'ютексів
public:
	void addfile(string filename);
	string search(string word);
	void clear_dictionary();
};

void InvertedIndex::addfile(string filename)
{
	ifstream fp;
	fp.open(filename, ios::in);

	if (!fp)
	{
		cout << "File Not Found\n";
	}

	string  word;
	while (fp >> word)
	{
			M[word].lock();
			Dictionary[word].push_back(filename);
			M[word].unlock();
	}
	fp.close();
}

string InvertedIndex::search(string word)
{
	if (Dictionary.find(word) == Dictionary.end())
	{
		return "No instance exist";
	}

	int size = (int)Dictionary[word].size();

	string list = word + ": { ";//індекси слова
	for (int counter = 0;counter < size;counter++)
	{
		string s1 = Dictionary[word][counter];
		regex target("(.txt)+");
		string replacement = " ";
		string s2 = regex_replace(s1, target, replacement);
		list += s2;
	}
	list += "}";
	return list;
}

void InvertedIndex::clear_dictionary() {
	Dictionary.clear();
}

void IndexInput(InvertedIndex& Data, int start, int end) {
	for (int i = start; i < end; i++)
	{
		string name ="("+  to_string(i)  +").txt";
		Data.addfile(name);
	}
}

SOCKET Connections[100];
int Counter = 0;//для сокетів

void ClientHandler(int index) {
	int msg_size;
	
	InvertedIndex Data;
	int start = 1;
	int end = 2000;

	while (true) {
		recv(Connections[index], (char*)&msg_size, sizeof(int), NULL);
		char* msg = new char[msg_size + 1];//отримуємо повідомлення від клієнта
		msg[msg_size] = '\0';
		recv(Connections[index], msg, msg_size, NULL);
		
		stringstream geek(msg);
		int action;//переводим у число
		geek >> action;
		string s;//запишемо результат обробки масивів у змінну string
		if (action) {//обробка залежно від команди клієнта
			Data.clear_dictionary();
			unsigned int start_time = clock();//початковий час
			
			int num = action;
			vector<thread> threads;
			threads.reserve(num);
			for (int i = 0; i < num; i++) {  //створюємо потоки і обробляємо дані
				threads.emplace_back(IndexInput, ref(Data), i * end / num, (i + 1) * end / num);
			}

			for (auto& thread : threads) {  //завершуємо потоки
				thread.join();
			}

			unsigned int end_time = clock(); // кінцевий час
			s = "Time of building Index is " + to_string((end_time - start_time) / 1000.0);
		}
		else {
		s = Data.search(msg);
		}
		
		int s_size = s.size();//повертаємо результат клієнту
			send(Connections[index], (char*)&s_size, sizeof(int), NULL);
			send(Connections[index], s.c_str(), s_size, NULL);
		delete[] msg;

	}
}

int main(int argc, char* argv[]) {
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);//запит версії бібліотеки WinSock
	if (WSAStartup(DLLVersion, &wsaData) != 0) {//загрузка бібліотеки до wsaData
		std::cout << "Error" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;//заповнимо адресу сокета
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");//зберігаємо local host
	addr.sin_port = htons(1111);//порт має бути не зайнятий іншою програмою
	addr.sin_family = AF_INET;//сімейство протоколів, константа AF_INET

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);//створення сокета
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));//прив'язка адреси
	listen(sListen, SOMAXCONN);// прослуховування клієнтів

	SOCKET newConnection;
	for (int i = 0; i < 100; i++) {
		newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);

		if (newConnection == 0) {//клієнт не зміг приєднатися
			std::cout << "Error #2\n";
		}
		else {
			std::cout << "Client Connected!\n";
			std::string msg = "Server works!";//клієнт дізнається, що він підключився
			int msg_size = msg.size();//спочатку відправляємо довжину повідомлення для виділення пам'яті
			send(newConnection, (char*)&msg_size, sizeof(int), NULL);//потім повідомлення
			send(newConnection, msg.c_str(), msg_size, NULL);

			Connections[i] = newConnection;
			Counter++;//кожного нового клієнта підключаємо в наступний сокет
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
			//функція обробки масивів працює в окремому потоці, і у кожного клієнта він свій
		}
	}

	system("pause");
	return 0;
}
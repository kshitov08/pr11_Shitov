// pr11_Shitov.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <ctime>
#include <conio.h>

using namespace std;

CRITICAL_SECTION cs;

struct Robot {
    int id;
    int energy;
    int points;
    HANDLE hThread;
    int status;
};

int tests[4];
int testsCount = 0;

void CreateBinFile()
{
    ofstream file("arena.dat", ios::binary);
    int testsData[] = { 1, 2, 3, 4 };
    file.write((char*)testsData, sizeof(testsData));
    file.close();
}


DWORD WINAPI RobotThread(LPVOID lpParam) {
    Robot* robot = (Robot*)lpParam;
    srand(time(NULL) + robot->id);
    for (int i = 0; i < testsCount; i++) {
        if (robot->status == 3) break;

        Sleep((rand() % 3 + 1) * 1000);

        int testType = tests[i];
        bool canPass = true;
        int duration = 0;

        if (testType == 1) duration = 3;
        else if (testType == 2) duration = 4;
        else if (testType == 3)
        {
            duration = rand() % 5 + 1;
            EnterCriticalSection(&cs);
            cout << "Робот " << robot->id << " подзаряжается " << duration << " с." << endl;
            LeaveCriticalSection(&cs);
        }
        else if (testType == 4) duration = 10;

        if (testType == 1 && robot->energy < 10) canPass = false;
        if (testType == 2 && robot->energy < 20) canPass = false;
        if (testType == 4 && robot->energy < 60) canPass = false;

        if (canPass) {
            Sleep(duration * 1000);

            if (testType == 1) { robot->energy -= 10; robot->points += 15; }
            if (testType == 2) { robot->energy -= 20; robot->points += 30; }
            if (testType == 3)
            {
                robot->energy += duration * 10;
                if (robot->energy > 100) robot->energy = 100;
            }
            if (testType == 4) { robot->energy -= 60; robot->points += 55; }

            EnterCriticalSection(&cs);
            cout << "Робот " << robot->id << " прошел испытание " << testType
                << ". Энергия: " << robot->energy << ", Очки: " << robot->points << "\n";
            LeaveCriticalSection(&cs);
        }
        else {
            EnterCriticalSection(&cs);
            cout << "Робот " << robot->id << " пропустил испытание " << testType << " (мало энергии)\n";
            LeaveCriticalSection(&cs);
        }
    }

    robot->status = 3;

    EnterCriticalSection(&cs);
    cout << "=== Робот " << robot->id << " ЗАВЕРШИЛ! Очки: " << robot->points << " ===\n";
    LeaveCriticalSection(&cs);

    return 0;
}
int main() {
    setlocale(NULL, "Russian");
    srand(time(NULL));
    InitializeCriticalSection(&cs);
    CreateBinFile();

    ifstream file("arena.dat", ios::binary);
    if (!file.is_open()) {
        cout << "Ошибка открытия файла!\n";
        return 1;
    }

    file.seekg(0, ios::end);
    int fileSize = file.tellg();
    file.seekg(0, ios::beg);

    testsCount = fileSize / sizeof(int);
    if (testsCount > 100) testsCount = 100;

    if (testsCount <= 0) {
        cout << "Ошибка: файл пустой или поврежден!\n";
        return 1;
    }

    file.read((char*)tests, testsCount * sizeof(int));
    file.close();

    cout << "Загружено испытаний: " << testsCount << "\n";

    Robot robots[4];
    for (int i = 0; i < 4; i++) {
        robots[i].id = i + 1;
        robots[i].energy = 60;
        robots[i].points = 0;
        robots[i].status = 1;
    }

    for (int i = 0; i < 4; i++) {
        robots[i].hThread = CreateThread(NULL, 0, RobotThread, &robots[i], 0, NULL);
    }

    cout << "Введите ID робота (1-4) для паузы/возобновления. 0 - ждать финала.\n";
    int cmd;
    while (true) {
        cin >> cmd;
        if (cmd == 0) break;

        if (cmd >= 1 && cmd <= 4) {
            if (robots[cmd - 1].status == 3) {
                cout << "Робот уже завершил испытания!\n";
                continue;
            }

            if (robots[cmd - 1].status == 1) {
                SuspendThread(robots[cmd - 1].hThread);
                robots[cmd - 1].status = 2;
                EnterCriticalSection(&cs);
                cout << ">> Робот " << cmd << " на ПАУЗЕ\n";
                LeaveCriticalSection(&cs);
            }
            else if (robots[cmd - 1].status == 2) {
                ResumeThread(robots[cmd - 1].hThread);
                robots[cmd - 1].status = 1;
                EnterCriticalSection(&cs);
                cout << ">> Робот " << cmd << " ПРОДОЛЖАЕТ работу\n";
                LeaveCriticalSection(&cs);
            }
        }
        else cout << "Введите корректный ID робота(1-4)\n";
    }

    cout << "\nОжидание завершения всех роботов...\n";

    for (int i = 0; i < 4; i++) {
        WaitForSingleObject(robots[i].hThread, INFINITE);
        CloseHandle(robots[i].hThread);
    }

    cout << "\nВсе роботы завершили испытания! Подводим итоги...\n";

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3 - i; j++) {
            if (robots[j].points < robots[j + 1].points) {
                Robot temp = robots[j];
                robots[j] = robots[j + 1];
                robots[j + 1] = temp;
            }
        }
    }

    cout << "\n===== РЕЗУЛЬТАТЫ =====\n";
    for (int i = 0; i < 4; i++) {
        cout << i + 1 << " место: Робот " << robots[i].id << " | Очки: " << robots[i].points << "\n";
    }

    if (robots[0].points == robots[1].points) {
        cout << "\nНИЧЬЯ!\n";
    }
    else {
        cout << "\nПОБЕДИТЕЛЬ: Робот " << robots[0].id << "\n";
    }

    DeleteCriticalSection(&cs);

    cout << "\nНажмите любую клавишу для выхода...";
    _getch();

    return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <math.h>
#include <SimConnect.h>

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "10110"

struct Struct1
{
    char    title[256];
    double  kohlsmann;
    double  altitude;
    double  latitude;
    double  longitude;
    double  trueheading;
    double  heading;
    double speed;
};

class CMyCriticalSection

{
public:

    CMyCriticalSection() { InitializeCriticalSection(&m_CriticalSection); }

    ~CMyCriticalSection() { DeleteCriticalSection(&m_CriticalSection); }

    void Enter() { EnterCriticalSection(&m_CriticalSection); }

    void Leave() { LeaveCriticalSection(&m_CriticalSection); }

private:

    CRITICAL_SECTION m_CriticalSection;

};

class MyString
{
public:
    CMyCriticalSection m_Section;
    int m_size;
    static const int DefaultSize = DEFAULT_BUFLEN;
    char* m_p;
    MyString(int buff = DefaultSize) {
        if (NULL != buff)
        {
            m_p = new char[buff];
            m_size = buff;
        }
        else
        {
            m_p = NULL;
            m_size = 0;
        }

    }
    ~MyString()
    {
        if (NULL != m_p)
        {
            delete m_p;
        }
    }

};

bool getCheckSum(MyString& string);
double toNMEACoordinate(double number);
bool returnGPRMCSentence(MyString& string, Struct1& Raw);

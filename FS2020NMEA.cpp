#include "FS2020NMEA.h"


//

int     quit = 0;
HANDLE  hSimConnect = NULL;



bool bStopSending = false;



// the most stupid class in the world. 2DO: replace with proper string class

MyString GPRMC;
MyString GPGGA;
MyString GPGSA;



bool getCheckSum(MyString& string)
{
    char* current = string.m_p;
    byte checksum = 0;
    if (*current != '$')
        return false;

    while (++current)
    {
        if ((*current == '*')) //terminator reached
        {
            char temp[DEFAULT_BUFLEN];
            snprintf(temp, DEFAULT_BUFLEN, "%x\n", checksum);
            strcat_s(string.m_p,string.m_size, temp);
            break;
        }
        else if (current == (string.m_p + string.m_size)) // end of string reached
        {
            return false;
        }
        else // "normal" XOR needed at current position
        {
            checksum ^= *current;
        }
    }
    return true;
}

double toNMEACoordinate(double number)
{
    double RetVal = 0.0;

    RetVal = fabs(floor(number)) ;
    double remaining = number - RetVal;
    RetVal *= 100.0;
    remaining *= 60.0;
    RetVal += remaining;

    return RetVal;
}




bool returnGPRMCSentence(MyString& string, Struct1& Raw)
{
    SYSTEMTIME Time;
    GetSystemTime(&Time);

    string.m_Section.Enter();
    snprintf(string.m_p, string.m_size, "$GPRMC,%02d%02d%02d.00,A,%010.5f,%c,%010.5f,%c,%2.1f,%2.1f,%d%d%d,0.0,E,A*",
        Time.wHour, Time.wMinute, Time.wSecond, toNMEACoordinate(Raw.latitude), ((Raw.latitude > 0.0) ? 'N' : 'S'), toNMEACoordinate(Raw.longitude), ((Raw.longitude > 0.0) ? 'E' : 'W'), Raw.speed, Raw.heading, Time.wDay, Time.wMonth, Time.wYear);

 

    bool bRet = getCheckSum(string);
    string.m_Section.Leave();

    return bRet;
}

bool  returnGPGGASentence(MyString & string, Struct1 & Raw)
{
    SYSTEMTIME Time; 
    GetSystemTime(&Time);
    string.m_Section.Enter();

    snprintf(string.m_p, string.m_size, "$GPGGA,%02d%02d%02d.00,%010.5f,%c,%010.5f,%c,1,8,0.0,%.1f,M,0.0,M,,,*",
        Time.wHour, Time.wMinute, Time.wSecond, toNMEACoordinate(Raw.latitude), ((Raw.latitude > 0.0) ? 'N' : 'S'), toNMEACoordinate(Raw.longitude), ((Raw.longitude > 0.0) ? 'E' : 'W') , Raw.altitude/ 3.2808 );
    bool bRet = getCheckSum(string);
    string.m_Section.Leave();

    return bRet;
}
bool returnGPGSASentence(MyString& string, Struct1& Raw)
{
    string.m_Section.Enter();
    snprintf(string.m_p, string.m_size, "$GPGSA,A,3,19,28,14,18,27,22,31,39,,,,,1.7,1.0,1.3*34\n");
    //bool bRet = getCheckSum(string);
    string.m_Section.Leave();

    return true;
}

bool bSendDummySentences = true;
DWORD SendDummySentence(
    _In_ LPVOID lpParameter
)
{
    Struct1 s;
    s.altitude = 1883;
    s.heading = s.trueheading = 260;
    s.kohlsmann = 29.93;
    s.latitude = 49.35220;
    s.longitude = 11.48936;
    s.speed = 0;

    while (bSendDummySentences)
    {
        returnGPRMCSentence(GPRMC, s);
        returnGPGSASentence(GPGSA, s);
        returnGPGGASentence(GPGGA, s);
        Sleep(1000);
    }

    return 0;
}


static enum EVENT_ID {
    EVENT_SIM_START,
};

static enum DATA_DEFINE_ID {
    DEFINITION_1,
};

static enum DATA_REQUEST_ID {
    REQUEST_1,
};

void CALLBACK MyDispatchProcRD(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    HRESULT hr;

    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_EVENT:
    {
        SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;

        switch (evt->uEventID)
        {
        case EVENT_SIM_START:

            // Now the sim is running, request information on the user aircraft
            hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, REQUEST_1, DEFINITION_1, 0, SIMCONNECT_SIMOBJECT_TYPE_USER);

            break;

        default:
            break;
        }
        break;
    }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
    {
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE*)pData;

        switch (pObjData->dwRequestID)
        {
        case REQUEST_1:
        {
            DWORD ObjectID = pObjData->dwObjectID;
            Struct1* pS = (Struct1*)&pObjData->dwData;
            if (SUCCEEDED(StringCbLengthA(&pS->title[0], sizeof(pS->title), NULL))) // security check
            {
                
                bSendDummySentences = false;
                if (returnGPRMCSentence(GPRMC,*pS))
                {
                 //   printf("%s\r\n", GPRMC.m_p);
                }
                if (returnGPGGASentence(GPGGA, *pS))
                {
                //    printf("%s\r\n", GPGGA.m_p);
                }
                //printf("\nObjectID=%d  Title=\"%s\"\nLat=%f  Lon=%f  Alt=%f  Kohlsman=%.2f Heading=%f Speed=%f\r\n", ObjectID, pS->title, pS->latitude, pS->longitude, pS->altitude, pS->kohlsmann,pS->heading,pS->speed);
            }
            Sleep(1000);
            hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, REQUEST_1, DEFINITION_1, 0, SIMCONNECT_SIMOBJECT_TYPE_USER);
            break;
        }

        default:
            break;
        }
        break;
    }


    case SIMCONNECT_RECV_ID_QUIT:
    {
        quit = 1;
        break;
    }

    case SIMCONNECT_RECV_ID_OPEN:
    {
        //printf("Attached to new Flight.\n");
        // just no message ....
        break;
    }

    default:
        printf("\nReceived:%d", pData->dwID);
        break;
    }
}

void testDataRequest()
{
    HRESULT hr;
    bool bConnected = false;
    const int iCount = 600;
    for (int i = iCount; i > 0; i--)
    {
        if (SUCCEEDED(SimConnect_Open(&hSimConnect, "Request Data", NULL, 0, 0, 0)))
        {
            bConnected = true;
            break;
        }
        else
        {
            if (i == iCount)
            {
                printf("Not connected to Flight Simulator yet. Please start the app or wait until it is fully started.\n", i);
            }
            printf("Will retry FS connection for %d seconds\n", i);
            Sleep(1000);
        }
    }

    if (bConnected)
    {
        printf("\nConnected to Flight Simulator!\n");

        // Set up the data definition, but do not yet do anything with it
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Title", NULL, SIMCONNECT_DATATYPE_STRING256);
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Kohlsman setting hg", "inHg");
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Altitude", "feet");
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Latitude", "degrees");
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Longitude", "degrees");
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Heading Degrees True", "degrees");
        // GPS GROUND SPEED & Heading
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "GPS GROUND TRUE HEADING", "degrees");
        hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Gps Ground Speed", "knots");

        // Request an event when the simulation starts
        hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");

        printf("\nYou can now start a new flight. If flight was started before no data can be received.\n");

        while (0 == quit)
        {
            SimConnect_CallDispatch(hSimConnect, MyDispatchProcRD, NULL);
            Sleep(1);
        }

        hr = SimConnect_Close(hSimConnect);
    }
    else
    {
        printf("Could not establish connection. Will shut down now\n");
    }
}

DWORD DoTCPIPSocket(
    _In_ LPVOID lpParameter
)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
    int i = 0;

    // Receive until the peer shuts down the connection
    do {

            if (0 == i)
            {
                //snprintf(recvbuf, recvbuflen, "$GPRMC,144326.00,A,5107.0017737,N,11402.3291611,W,0.080,323.3,210307,0.0,E,A*20\n");
                GPRMC.m_Section.Enter();
                strcpy_s(recvbuf, recvbuflen, GPRMC.m_p);
                GPRMC.m_Section.Leave();
            }
            else if (1 == i )
            {
                GPGGA.m_Section.Enter();
                strcpy_s(recvbuf, recvbuflen, GPGGA.m_p);
                GPGGA.m_Section.Leave();
            }
            else
            {
                GPGSA.m_Section.Enter();
                strcpy_s(recvbuf, recvbuflen, GPGSA.m_p);
                GPGSA.m_Section.Leave();
            }
            i++;


                   // Echo the buffer back to the sender
            iSendResult = send(ClientSocket, recvbuf, strnlen_s(recvbuf, recvbuflen), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            //printf("sent %s", recvbuf);
            
            if (3 == i)
            {
                i = 0;
                Sleep(1000);
            }

    } while (!bStopSending);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}

bool DisplayIPAdresses()
{
    /* Declare and initialize variables */

// It is possible for an adapter to have multiple
// IPv4 addresses, gateways, and secondary WINS servers
// assigned to the adapter. 
//
// Note that this sample code only prints out the 
// first entry for the IP address/mask, and gateway, and
// the primary and secondary WINS server for each adapter. 

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    UINT i;

    /* variables used to print DHCP time info */
    struct tm newtime;
    char buffer[32];
    errno_t error;

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO*)new byte[sizeof(IP_ADAPTER_INFO)];
    if (pAdapterInfo == NULL) {
        printf("Error allocating memory needed to call GetAdaptersinfo\n");
        return false;
    }
    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        delete(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO*)new byte[ulOutBufLen];
        if (pAdapterInfo == NULL) {
            printf("Error allocating memory needed to call GetAdaptersinfo\n");
            return false;
        }
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
           // printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);


            printf("\tIP Address: \t%s\n",
                pAdapter->IpAddressList.IpAddress.String);
 
            printf("\tPort: %s\n", DEFAULT_PORT);

            pAdapter = pAdapter->Next;
            printf("\n");
        }
    }
    else {
        printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

    }
    if (pAdapterInfo)
        delete(pAdapterInfo);

    return true;
}


int main()
{
    HANDLE TCPThread=NULL;
    HANDLE DummySender = NULL;
    printf("Welcome to Fs2020 NMEA Adapter.\n\n");

    printf("You can use the following IP Adresses & ports to connect to the Flight Simulator 2020 NMEA stream\n");
    DisplayIPAdresses();

    /*
    Struct1 x;
    x.altitude = 0.0;
    x.heading = 0.0;
    x.kohlsmann = 29.93;
    x.latitude = 0.0;
    x.longitude = 0.0;
    x.speed = 0.0;
    returnGPRMCSentence(GPRMC, x);
    returnGPGGASentence(GPGGA, x);
    returnGPGSASentence(GPGSA, x);
*/
    DummySender = CreateThread(NULL, 0, SendDummySentence, NULL, 0, NULL);

    TCPThread = CreateThread(NULL, 0, DoTCPIPSocket, NULL, 0, NULL);
    if (NULL == TCPThread)
    {
        printf("Creating TCP Thread Failed\n\n");
        return -1;
    }

    testDataRequest();
     
    //Terminate TCP Thread
    bStopSending = true;
    bSendDummySentences = false;
    WaitForSingleObject(DummySender, INFINITE);
    WaitForSingleObject(TCPThread, INFINITE);
    CloseHandle(TCPThread);
    return 0;
 }


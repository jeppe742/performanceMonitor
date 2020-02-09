#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <WbemIdl.h>

// #pragma comment(lib, "wbemuuid.lib")

using namespace std;

ULONG_PTR GetParentProcessId(HANDLE process) // By Napalm @ NetCore2K
{
 ULONG_PTR pbi[6];
 ULONG ulSize = 0;
 LONG (WINAPI *NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass,
  PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); 
 *(FARPROC *)&NtQueryInformationProcess = 
  GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
 if(NtQueryInformationProcess){
  if(NtQueryInformationProcess(process, 0,
    &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
     return pbi[5];
 }
 return (ULONG_PTR)-1;
}

ULONG_PTR GetNthParentId(int n){
    ULONG_PTR parentId, grandParentId;
    HANDLE currentProcess, parentProcess;

    if(n==0){
        return GetProcessId(GetCurrentProcess());
    }

    parentId = GetParentProcessId(GetCurrentProcess());

    for (size_t i = 1; i < n; i++)
    {
        parentProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, parentId);
        grandParentId = GetParentProcessId(parentProcess);
        parentId = grandParentId;
    }
    // CloseHandle(parentProcess);
    return parentId;
}

inline BSTR ConvertStringToBSTR(const char* pSrc)
{
    if(!pSrc) return NULL;

    DWORD cwch;

    BSTR wsOut(NULL);

    if(cwch = ::MultiByteToWideChar(CP_ACP, 0, pSrc, 
         -1, NULL, 0))//get size minus NULL terminator
    {
                cwch--;
            wsOut = ::SysAllocStringLen(NULL, cwch);

        if(wsOut)
        {
            if(!::MultiByteToWideChar(CP_ACP, 
                     0, pSrc, -1, wsOut, cwch))
            {
                if(ERROR_INSUFFICIENT_BUFFER == ::GetLastError())
                    return wsOut;
                ::SysFreeString(wsOut);//must clean up
                wsOut = NULL;
            }
        }

    };

    return wsOut;
};


int main(int argc, char **argv)
{
    int numParents = 1;
    ULONG_PTR pid;
    if(argc==2){
        numParents = atoi(argv[1]);
    }
    pid = GetNthParentId(numParents);
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres =  CoInitializeEx(0, COINIT_MULTITHREADED); 
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x" 
            << hex << hres << endl;
        return 1;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );

                      
    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x" 
            << hex << hres << endl;
        CoUninitialize();
        return 1;                    // Program has failed.
    }
    
    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator *pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object."
            << " Err code = 0x"
            << hex << hres << endl;
        CoUninitialize();
        return 1;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices *pSvc = NULL;
 
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
         SysAllocString(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (for example, Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x" 
             << hex << hres << endl;
        pLoc->Release();     
        CoUninitialize();
        return 1;                // Program has failed.
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        SysAllocString(L"WQL"), 
        SysAllocString(L"SELECT * FROM Win32_Process Where ProcessId="+pid),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator);
    
    if (FAILED(hres))
    {
        cout << "Query for operating system name failed."
            << " Error code = 0x" 
            << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return 1;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------
 
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
   
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
            &pclsObj, &uReturn);

        if(0 == uReturn)
        {
            break;
        }

        VARIANT name;
        VARIANT usertime;
        VARIANT kerneltime;

        // Get the value of the Name property
        hr = pclsObj->Get(L"Name", 0, &name, 0, 0);
        hr = pclsObj->Get(L"UserModeTime", 0, &usertime, 0, 0);
        hr = pclsObj->Get(L"KernelModeTime", 0, &kerneltime, 0, 0);
        wcout << "Name : " << name.bstrVal << " Usertime : " <<usertime.bstrVal << " Kerneltime : " <<kerneltime.bstrVal << endl;
        VariantClear(&name);
        VariantClear(&usertime);
        VariantClear(&kerneltime);

        pclsObj->Release();
    }

    // Cleanup
    // ========
    
    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();

    return 0;   // Program successfully completed.
 
}

// #include <vector>
// #include <string>
// #include <processthreadsapi.h>
// #include <windows.h>
// #include <timezoneapi.h>
// #define _SECOND ((int64) 10000000)
// #define _MINUTE (60 * _SECOND)
// #define _HOUR   (60 * _MINUTE)
// #define _DAY    (24 * _HOUR)

// using namespace std;

// int main()
// {
   // FILETIME createtime;
   // FILETIME kerneltime;
   // FILETIME usertime;
   // FILETIME exittime;
   // SYSTEMTIME usersystime;
   // SYSTEMTIME kernelsystime;
   // std::cout << "test"<<std::endl;

   // GetProcessTimes( GetCurrentProcess(), &createtime, &exittime, &kerneltime, &usertime);
   // FileTimeToSystemTime(&usertime, &usersystime);
   // FileTimeToSystemTime(&kerneltime, &kernelsystime);

   // cout << "user time: " <<  usersystime.wSecond << "."<< usersystime.wMilliseconds << endl;
   // cout << "kernel time: " <<  kernelsystime.wSecond << "."<< kernelsystime.wMilliseconds << endl;

// }
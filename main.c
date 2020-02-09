#include <stdio.h>
#include <windows.h>
#include <Psapi.h>

#ifdef TlHelp32
#include <TlHelp32.h>


ULONG_PTR GetNthParentId(int n){
    int pid = -1;
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32);

    pid = GetCurrentProcessId();
    if(n==0){
        return pid;
    }

    for (size_t i = 0; i < n; i++)
    {

        if( Process32First(h, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    printf("PID: %i; PPID: %i\n", pid, pe.th32ParentProcessID);
                    pid=pe.th32ParentProcessID;
                }
            } while( Process32Next(h, &pe));
        }
    }

    CloseHandle(h);
    return pid;
}

#else
ULONG_PTR GetParentProcessId(HANDLE process) 
{
    ULONG_PTR pbi[6];
    ULONG ulSize = 0;
    LONG (WINAPI *NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); 
    //NTDLL has to be loaded in a special way
    *(FARPROC *)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
    if(NtQueryInformationProcess){
        //Get queryinformation from process. This includes the parent PID
        if(NtQueryInformationProcess(process, 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
            return pbi[5];
    }
 return (ULONG_PTR)-1;
}

ULONG_PTR GetNthParentId(int n){
    ULONG_PTR parentId, grandParentId;
    HANDLE currentProcess, parentProcess;
    // Just get process id of current process
    if(n==0){
        return GetProcessId(GetCurrentProcess());
    }

    // Get parent Process ID
    parentId = GetParentProcessId(GetCurrentProcess());

    // If we need to go to ealiers ancestors 
    for (size_t i = 1; i < n; i++)
    {
        parentProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, parentId);
        grandParentId = GetParentProcessId(parentProcess);
        parentId = grandParentId;
    }
    return parentId;
}

#endif

int main(int argc, char** argv)
{
    FILETIME createtime, kerneltime, usertime, exittime;
    SYSTEMTIME usersystime, kernelsystime;
    ULONG_PTR pid;
    IO_COUNTERS ioCounters;
    PROCESS_MEMORY_COUNTERS memCounters;
    TCHAR nameProc[1024];
    int numParents = 1; // Command line argument, to control which ancestor to use
    
    if(argc==2){
        numParents = atoi(argv[1]);
    }
    
    pid = GetNthParentId(numParents);

    if(!pid){
        printf("Could not find process ID of %s-th parent \n",numParents);
        return 1;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid);

    if(NULL == process){
        printf("Could not open process with PID:%d\n",pid);
        return 2;
    }
    
    // Get the full name of the process of interest
    if(GetProcessImageFileName(process,nameProc,sizeof(nameProc)/sizeof(*nameProc))==0){
        printf("Failed to get name of process with PID:%d\n",pid);
        return 3;
    }
    printf("Name: %s | ",nameProc);

    // Get CPU time information
    if(GetProcessTimes(process, &createtime, &exittime, &kerneltime, &usertime)){
        FileTimeToSystemTime(&usertime, &usersystime);
        FileTimeToSystemTime(&kerneltime, &kernelsystime);
        printf("User time: %d.%d | ", usersystime.wSecond, usersystime.wMilliseconds);
        printf("Kernel time: %d.%d | ", kernelsystime.wSecond, kernelsystime.wMilliseconds);
    }
    else{
        printf("Failed to get CPU times for %s\n",nameProc);
        return 4;
    }
    // Get IO information
    if(GetProcessIoCounters(process, &ioCounters)){
        printf("Reads: %d | Writes: %d | Other: %d | ", ioCounters.ReadOperationCount, ioCounters.WriteOperationCount, ioCounters.OtherOperationCount);
    }
    else{
        printf("Failed to get IO counters for %s\n",nameProc);
        return 5;
    }
    // Get memory information
    if(GetProcessMemoryInfo(process, &memCounters, sizeof(memCounters))){
        printf("Page faults: %d | working set size: %d | peak working set size %d | ", memCounters.PageFaultCount, memCounters.WorkingSetSize, memCounters.PeakWorkingSetSize);
        printf("Peak paged pool usage: %d | peak non-paged pool usage: %d | peak pagefile usage: %d\n", memCounters.QuotaPeakPagedPoolUsage, memCounters.QuotaPeakNonPagedPoolUsage, memCounters.PeakPagefileUsage);
    }
    else{
        printf("Failed to get memory info for %s\n",nameProc);
        return 6;
    }
   return 0;
}
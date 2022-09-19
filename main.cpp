#include "pch.h"

std::vector<HANDLE> WaitObjects;
std::vector<DWORD> RegisteredProcesses;

bool FinishExecution = false;

VOID CALLBACK WaitOrTimerCallback(IN PVOID lpParameter, IN BOOLEAN TimerOrWaitFired) {

    PROCESSENTRY32* PE32 = (PROCESSENTRY32*)lpParameter;

    // Check to see if process is inside of RegisteredProcesses. If it is, we remove it.
    if (std::find(RegisteredProcesses.begin(), RegisteredProcesses.end(), PE32->th32ProcessID) != RegisteredProcesses.end()) {
        RegisteredProcesses.erase(std::remove(RegisteredProcesses.begin(), RegisteredProcesses.end(), PE32->th32ProcessID), RegisteredProcesses.end());
    }

    std::wcout << "[-] " << PE32->szExeFile << " has exited [" << PE32->th32ProcessID << "]\n";

    return;

}

void CreateCallbacks() {

    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    PROCESSENTRY32 ProcessEntry { sizeof(PROCESSENTRY32) };

    if (Process32First(Snapshot, &ProcessEntry)) {

        do {
            
            // If the current process entry is our process.
            if (GetCurrentProcessId() == ProcessEntry.th32ProcessID) {
                continue;
            }

            HANDLE WaitObject = 0, ProcessHandle = OpenProcess(SYNCHRONIZE, FALSE, ProcessEntry.th32ProcessID);

            // If process handle is valid.
            if (!ProcessHandle) {
                continue;
            }

            // If process is already in RegisteredProcesses.
            if (std::find(RegisteredProcesses.begin(), RegisteredProcesses.end(), ProcessEntry.th32ProcessID) != RegisteredProcesses.end()) {
                continue;
            }

            // If we can register a callback.
            if (!RegisterWaitForSingleObject(&WaitObject, ProcessHandle, WaitOrTimerCallback, &ProcessEntry, INFINITE, WT_EXECUTEONLYONCE)) {
                continue;
            }

            // Mark this specific process as having a callback already registered.
            RegisteredProcesses.push_back(ProcessEntry.th32ProcessID);

            // Bookkeep wait objects in order to cancel them once we're done.
            WaitObjects.push_back(WaitObject);

        }

        while (Process32Next(Snapshot, &ProcessEntry));
    
    }

    CloseHandle(Snapshot);

}

void UsermodeProcessCallback() {

    while (true) {

        CreateCallbacks();

        if (FinishExecution)
            break;

        Sleep(1000);

    }
}

int main() {

    std::thread CreateCallbackThread(UsermodeProcessCallback);

    while (true) {

        if (GetKeyState(VK_DELETE)) {

            for (HANDLE WaitObject : WaitObjects) {

                if (!WaitObject)
                    continue;

                UnregisterWait(WaitObject);

            }

            FinishExecution = true;

            CreateCallbackThread.join();

            std::wcout << "\nProcess successfully terminated.\n";
            return 0;
     
        }

        Sleep(100);

    }
}

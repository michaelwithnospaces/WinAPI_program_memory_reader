#include <iostream>
#include <Windows.h>
#include <vector>

int printAndGetOptions();
void getHandleFromPid();
int readAndPrintInt(LPCVOID baseAddress, SIZE_T readSize);
std::string readAndPrintString(LPCVOID baseAddress);
void readAndPrintCharArray(LPCVOID baseAddress, SIZE_T readSize);
void readProcessMemoryError(BOOL value);
uintptr_t readAndPrintPtrAddress(uintptr_t addressBuffer);
uintptr_t followPointerChain(HANDLE hProcess, const std::vector<uintptr_t>& addresses);
void terminateProgram();

SIZE_T* lpNumberOfBytesRead = NULL;
HANDLE hProcess;

int main()
{
    getHandleFromPid();

    bool continueProgram = true;
    int userOption = printAndGetOptions();

    while (continueProgram) {

        switch (userOption) {
            case 1: {
                // read memory address varInt
                uintptr_t memoryAddress;
                std::cout << "Memory address of the integer to read (in hexadecimal): 0x";
                std::cin >> std::hex >> memoryAddress; 

                // dereference memory address varInt
                readAndPrintInt((LPCVOID)memoryAddress, sizeof(int));
                break;
            }
            case 2: {
                // read memory address ptr2int
                uintptr_t ptrAddressRead = 0x0;
                ptrAddressRead = readAndPrintPtrAddress(ptrAddressRead);

                // dereference the ptr2int
                readAndPrintInt((LPCVOID)ptrAddressRead, sizeof(int));
                break;
            }
            case 3: {
                // following a pointer chain;
                uintptr_t baseAddress;
                std::cout << "Base address to follow (in hexadecimal): 0x";
                std::cin >> std::hex >> baseAddress;
                std::vector<uintptr_t> ptrChainVector = { baseAddress, 0, 0, 0 };
                uintptr_t finalAddress = followPointerChain(hProcess, ptrChainVector);
                std::cout << "finalAddress = 0x" << std::hex << std::uppercase << finalAddress << std::endl;

                // dereference the final pointer
                readAndPrintInt((LPCVOID)finalAddress, sizeof(int));
                break;
            }
            case 4: {
                uintptr_t baseAddress;
                std::cout << "Memory address of the string to read (in hexadecimal): 0x";
                std::cin >> std::hex >> baseAddress;

                readAndPrintString((LPCVOID)baseAddress);
                break;
            }
            case 5: {
                uintptr_t baseAddress;
                std::cout << "Memory address of the character array to read (in hexadecimal): 0x";
                std::cin >> std::hex >> baseAddress;

                readAndPrintCharArray((LPCVOID)baseAddress, 128);
                break;
            }
        }

        char quitCondition;
        std::cout << "Quit? (y/n): ";
        std::cin >> quitCondition;
        if (quitCondition == 'y') {
            continueProgram = false;
            terminateProgram();
        }
        else {
            userOption = printAndGetOptions();
        }
    }
    CloseHandle(hProcess);
    return EXIT_SUCCESS;
}

int printAndGetOptions() {
    int userOption;
    std::cout << std::endl << "Enter corresponding value to read: " << std::endl <<
        "    (1) varInt\n    (2) ptr2int\n    (3) Pointer chain (ptr2ptr2)\n    (4) varString\n    (5) arrChar\n\n>> ";
    std::cin >> userOption;
    std::cout << std::endl;
    return userOption;
}

// reads PID and returns HANDLE
void getHandleFromPid() {
    DWORD pid;

    // read process ID
    std::cout << "Process ID: ";
    std::cin >> pid;

    hProcess = OpenProcess(
        PROCESS_VM_READ, // desired access
        FALSE, // child process' inherit handle
        pid // target PID
    );

    // handle OpenProcess errors
    if (hProcess == NULL) {
        std::cout << "OpenProcess failed. GetLastError = " << std::dec << GetLastError() << std::endl;
        system("pause");
    }
}

// reads memory of int and returns value (dereferences memory)
int readAndPrintInt(LPCVOID baseAddress, SIZE_T readSize) {
    int intBuffer = 0;

    BOOL intResult = ReadProcessMemory(
        hProcess, // handle of process
        baseAddress, // base address to read
        &intBuffer, // store content buffer
        readSize, // bytes to read from process
        lpNumberOfBytesRead // store bytes read buffer
    );

    readProcessMemoryError(intResult);

    // print varInt value
    std::cout << "Value at address = " << std::dec << intBuffer << std::endl << std::endl;

    return intBuffer;
}

// reads memory of string and returns value (dereferences memory)
std::string readAndPrintString(LPCVOID baseAddress) {
    char stringBuffer[256];  // Adjust size as necessary
    BOOL stringResult = ReadProcessMemory(
        hProcess,
        baseAddress,
        stringBuffer,
        sizeof(stringBuffer) - 1,  // Leave space for null-terminator
        lpNumberOfBytesRead
    );

    stringBuffer[sizeof(stringBuffer) - 1] = '\0';  // Ensure null-termination

    readProcessMemoryError(stringResult);

    std::cout << "String at address = " << stringBuffer << std::endl << std::endl;

    return std::string(stringBuffer);
}


// reads memory of character array(dereferences memory)
void readAndPrintCharArray(LPCVOID baseAddress, SIZE_T readSize) {
    char bufferArray[128] = "";

    // Ensure we do not read more data than the buffer can hold

    BOOL charArrayResult = ReadProcessMemory(
        hProcess, // handle of process
        baseAddress, // base address to read
        &bufferArray, // store content buffer
        readSize, // bytes to read from process
        lpNumberOfBytesRead // store bytes read buffer
    );

    readProcessMemoryError(charArrayResult);

    // print bufferArray value
    std::cout << "Value at address = " << bufferArray << std::endl << std::endl;
}

// reads pointer, returns address pointer to
uintptr_t readAndPrintPtrAddress(uintptr_t addressBuffer) {
    uintptr_t memoryAddress = 0x0;
    std::cout << "Memory address of the ptr to read (in hexadecimal): 0x";
    std::cin >> std::hex >> memoryAddress;
    BOOL ptrResult = ReadProcessMemory(
        hProcess, // handle of process
        (LPCVOID)memoryAddress, // base address to read
        &addressBuffer, // store content buffer
        8, // bytes of x64
        lpNumberOfBytesRead // store bytes read buffer
    );

    // handle and print ReadProcessMemory errors varInt
    readProcessMemoryError(ptrResult);

    std::cout << "ptrAddressRead = 0x" << std::hex << std::uppercase << addressBuffer << std::endl;

    return addressBuffer;
}

// takes BOOL of ReadProcessMemory and handles errors
void readProcessMemoryError(BOOL value) {
    if (value == FALSE) {
        std::cout << "ReadProcessMemory failed. GetLastError = " << std::dec << GetLastError() << std::endl;
        system("pause");
    }
}

// follows pointer chain and returns final address with offsets
uintptr_t followPointerChain(HANDLE hProcess,  const std::vector<uintptr_t>& addresses) {
    
    uintptr_t currentAddress = addresses[0];
    LPCVOID lpBaseAddress = (LPCVOID)currentAddress;
    uintptr_t tempAddress = 0x0;
    
    for (int i = 1; i < addresses.size(); ++i) {
        BOOL ptrResult = ReadProcessMemory(
            hProcess, // handle of process
            (LPCVOID)currentAddress, // base address to read
            &tempAddress, // store content buffer
            8, // bytes to read from process
            lpNumberOfBytesRead // store bytes read buffer
        );

        readProcessMemoryError(ptrResult);

        currentAddress = tempAddress + addresses[i];
        lpBaseAddress = (LPCVOID)currentAddress;
    }
    return tempAddress;
}

void terminateProgram() {
    std::cout << std::endl;
    std::cout << "Press ENTER to quit." << std::endl;
    system("pause > nul");
}
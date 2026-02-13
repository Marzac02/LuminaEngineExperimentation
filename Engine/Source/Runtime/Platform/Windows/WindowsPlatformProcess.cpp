#include "pch.h"
#ifdef _WIN32

#include "Containers/Array.h"
#include "Containers/String.h"
#include "Paths/Paths.h"
#include "Platform/Process/PlatformProcess.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <shellapi.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <tchar.h>
#include <PathCch.h>  // For PathFindFileName
#include <psapi.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "PathCch.lib")

namespace Lumina::Platform
{
    namespace
    {
        static TVector<FString> GDLLSearchPaths;
    }
    
    void* GetDLLHandle(const TCHAR* Filename)
    {
        FWString WideString = Filename;
        TVector<FString> SearchPaths = GDLLSearchPaths;

        
        return LoadLibraryWithSearchPaths(StringUtils::FromWideString(WideString), SearchPaths);
    }

    bool FreeDLLHandle(void* DLLHandle)
    {
        return FreeLibrary((HMODULE)DLLHandle);
    }

    void* GetDLLExport(void* DLLHandle, const char* ProcName)
    {
        return (void*)::GetProcAddress((HMODULE)DLLHandle, ProcName);
    }

    void AddDLLDirectory(const FString& Directory)
    {
        GDLLSearchPaths.push_back(Directory);
    }

    void PushDLLDirectory(const TCHAR* Directory)
    {
        SetDllDirectory(Directory);
        
        GDLLSearchPaths.push_back(StringUtils::FromWideString(Directory));

        LOG_WARN("Pushing DLL Search Path: {0}", StringUtils::FromWideString(Directory));
    }

    void PopDLLDirectory()
    {
        GDLLSearchPaths.pop_back();

        if (GDLLSearchPaths.empty())
        {
            SetDllDirectory(L"");
        }
        else
        {
            SetDllDirectory(StringUtils::ToWideString(GDLLSearchPaths.back()).c_str());
        }
    }

    uint32 GetCurrentProcessID()
    {
        return 0;
    }

    uint32 GetCurrentCoreNumber()
    {
        return 0;
    }

    FString GetCurrentProcessPath()
    {
        char buffer[MAX_PATH];
        DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (length == 0)
        {
            return "";
        }
        
        return FString(buffer, length);
    }

    int LaunchProcess(const TCHAR* URL, const TCHAR* Params, bool bLaunchDetached)
    {
        if (!URL)
        {
            return -1;
        }

        FWString URLString(URL);
        eastl::replace(URLString.begin(), URLString.end(), '/', '\\');
        
        STARTUPINFOW si{};
        PROCESS_INFORMATION pi{};

        si.cb = sizeof(si);

        if (Params)
        {
            URLString += L" ";
            URLString += Params;
        }

        DWORD creationFlags = 0;
        if (bLaunchDetached)
        {
            creationFlags |= DETACHED_PROCESS | CREATE_NEW_CONSOLE;
        }

        // CreateProcessW modifies the command line string, so make a writable buffer
        TVector<wchar_t> cmdBuffer(URLString.begin(), URLString.end());
        cmdBuffer.push_back(L'\0');

        BOOL result = CreateProcessW(
            nullptr,                  // lpApplicationName
            cmdBuffer.data(),         // lpCommandLine
            nullptr,                  // lpProcessAttributes
            nullptr,                  // lpThreadAttributes
            FALSE,                    // bInheritHandles
            creationFlags,            // dwCreationFlags
            nullptr,                  // lpEnvironment
            nullptr,                  // lpCurrentDirectory
            &si,                      // lpStartupInfo
            &pi                       // lpProcessInformation
        );

        if (!result)
        {
            return static_cast<int>(GetLastError());
        }

        // Optionally detach from our process
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        return 0; // Success
    }
    
    void LaunchURL(const TCHAR* URL)
    {
        ShellExecuteW(nullptr, TEXT("open"), URL, nullptr, nullptr, SW_SHOWNORMAL);
    }

    const TCHAR* ExecutableName(bool bRemoveExtension)
    {
        static TCHAR ExecutablePath[MAX_PATH];
    
        if (GetModuleFileName(NULL, ExecutablePath, MAX_PATH) == 0)
        {
            return nullptr;
        }

        TCHAR* ExecutableName = PathFindFileName(ExecutablePath);

        // If bRemoveExtension is true, remove the file extension
        if (bRemoveExtension)
        {
            PathCchRemoveExtension(ExecutableName, MAX_PATH);
        }

        return ExecutableName;
    }

    size_t GetProcessMemoryUsageBytes()
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.PrivateUsage;
        }
        return 0;
    }

    size_t GetProcessMemoryUsageMegaBytes()
    {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
        {
            return pmc.PrivateUsage / (1024 * 1024);
        }
        return 0;
    }


    const TCHAR* BaseDir()
    {
        static TCHAR Buffer[MAX_PATH] = {};
        if (Buffer[0] == 0)
        {
            GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
        }
        return Buffer;
    }

    FVoidFuncPtr LumGetProcAddress(void* Handle, const char* Procedure)
    {
        return reinterpret_cast<FVoidFuncPtr>(GetProcAddress((HMODULE)Handle, Procedure));
    }

    void* LoadLibraryWithSearchPaths(const FString& Filename, const TVector<FString>& SearchPaths)
    {
        FWString Wide = StringUtils::ToWideString(Filename);
        if (void* Handle = GetModuleHandleW(Wide.c_str()))
        {
            return Handle;
        }

        if (void* Handle = LoadLibraryW(Wide.c_str()))
        {
            return Handle;
        }

        for (const FString& Path : SearchPaths)
        {
            FFixedString FullPath = Paths::Combine(Path, Filename);
            if (Paths::Exists(FullPath))
            {
                FWString WideStr = StringUtils::ToWideString(FullPath);
                if (void* Handle = LoadLibraryW(WideStr.c_str()))
                {
                    return Handle;
                }
            }
        }

        return nullptr;
    }

    bool OpenFileDialogue(FFixedString& OutFile, const char* Title, const char* Filter, const char* InitialDir)
    {
        // ------------------------------------------------------------------
        // COM Initialization
        // ------------------------------------------------------------------
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr))
            assert(false && "Failed to initialize COM library");

        IFileDialog* pFileDialog = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
            IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));

        if (FAILED(hr))
        {
            CoUninitialize();
            assert(false && "Failed to create File Open Dialog");
            return false;
        }

        // ------------------------------------------------------------------
        // Set Dialog Options
        // ------------------------------------------------------------------
        DWORD options;
        pFileDialog->GetOptions(&options);

        // If no filter is provided, we assume folder selection mode
        if (!Filter)
            pFileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        else
            pFileDialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST , );

        if (Title)
        {
            std::wstring wTitle(Title, Title + strlen(Title));
            pFileDialog->SetTitle(wTitle.c_str());
        }

        if (InitialDir)
        {
            std::wstring wDir(InitialDir, InitialDir + strlen(InitialDir));
            IShellItem* pFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(wDir.c_str(), nullptr,
                IID_PPV_ARGS(&pFolder))))
            {
                pFileDialog->SetFolder(pFolder);
                pFolder->Release();
            }
        }

        // ------------------------------------------------------------------
        // FIX: Dynamic Filter Setup
        // ------------------------------------------------------------------
        std::vector<COMDLG_FILTERSPEC> fileTypes;
        // Important: wstrings must remain in memory until the Show() method returns
        std::vector<std::wstring> wstringStorage;

        if (Filter && strlen(Filter) > 0)
        {
            // Expected Filter format: "Description1\0Extension1\0Description2\0Extension2\0"
            const char* p = Filter;
            while (*p)
            {
                std::wstring name;
                std::wstring spec;

                // Extract Name
                size_t len = strlen(p);
                name.assign(p, p + len);
                p += len + 1;

                // Extract Extension
                len = strlen(p);
                spec.assign(p, p + len);
                p += len + 1;

                wstringStorage.push_back(name);
                wstringStorage.push_back(spec);

                // Use c_str() pointers from the storage vector
                fileTypes.push_back({ wstringStorage[wstringStorage.size() - 2].c_str(),
                                      wstringStorage[wstringStorage.size() - 1].c_str() });
            }
        }

        if (!fileTypes.empty())
        {
            pFileDialog->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data());
        }
        // If no filter is provided, we don't explicitly set file types,
        // or you could add a default "All Files" here if needed.

        // ------------------------------------------------------------------
        // Show Dialog
        // ------------------------------------------------------------------
        bool result = false;
        if (SUCCEEDED(pFileDialog->Show(nullptr)))
        {
            IShellItem* pItem = nullptr;
            if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
            {
                PWSTR pszPath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
                {
                    std::wstring wPath = pszPath;
                    // Convert to string for storage
                    std::string path(wPath.begin(), wPath.end());

                    OutFile = path.c_str();
                    // Normalize path separators
                    eastl::replace(OutFile.begin(), OutFile.end(), '\\', '/');

                    CoTaskMemFree(pszPath);
                    result = true;
                }
                pItem->Release();
            }
        }

        pFileDialog->Release();
        CoUninitialize();
        return result;
    }
}


#endif
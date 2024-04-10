#include "Platforms/Windows/PlatformFileDialogWindows.h"
#include "FileSystem/FileSystem.h"

#ifdef IS_PLATFORM_WINDOWS

#include <Windows.h>
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents

#include "Platforms/Platform.h"
#include "Core/Logger.h"
#include "Core/Asserts.h"

namespace Insight
{
    namespace Platforms::Windows
    {
        bool PlatformFileDialogWindows::ShowSave(std::string* selectedItem, bool appendExtension)
        {
            return ShowSave(selectedItem, {}, appendExtension);
        }

        bool PlatformFileDialogWindows::ShowLoad(std::string* selectedItem)
        {
            return ShowLoad(selectedItem, {});
        }

        bool PlatformFileDialogWindows::ShowSave(std::string* selectedItem, const std::vector<FileDialogFilter>& fileFilters, bool appendExtension)
        {
            return Show(PlatformFileDialogOperations::SaveFile, selectedItem, fileFilters, appendExtension);
        }

        bool PlatformFileDialogWindows::ShowLoad(std::string* selectedItem, const std::vector<FileDialogFilter>& fileFilters)
        {
            return Show(PlatformFileDialogOperations::LoadFile, selectedItem, fileFilters);
        }

        bool PlatformFileDialogWindows::Show(PlatformFileDialogOperations operation, std::string* selectedItem, bool appendExtension)
        {
            return Show(operation, selectedItem, {}, appendExtension);
        }

        bool PlatformFileDialogWindows::Show(PlatformFileDialogOperations operation, std::string* selectedItem, const std::vector<FileDialogFilter>& fileFilters, bool appendExtension)
        {
            if (operation == PlatformFileDialogOperations::LoadFile 
                || operation == PlatformFileDialogOperations::SelectFile 
                || operation == PlatformFileDialogOperations::SelectFolder
                || operation == PlatformFileDialogOperations::SelectAll)
            {
                if (OpenDialog(operation, selectedItem, fileFilters))
                {
                    return true;
                }
            }
            else if (operation == PlatformFileDialogOperations::SaveFile)
            {
                if (SaveDialog(operation, selectedItem, fileFilters, appendExtension))
                {
                    return true;
                }
            }
            else
            {
                FAIL_ASSERT();
            }

            DWORD errorCode = CommDlgExtendedError();
            if (errorCode != 0)
            {
                IS_LOG_CORE_ERROR("[PlatformFileDialogWindows::Show] Error code: {}.", errorCode);
            }

            return false;
        }

        u32 PlatformFileDialogWindows::PlatformFileDialogOperationsToFileDialogOptions(PlatformFileDialogOperations operation)
        {
            switch (operation)
            {
            case Insight::PlatformFileDialogOperations::LoadFile:     return FOF_FILESONLY;
            case Insight::PlatformFileDialogOperations::SaveFile:     return FOF_FILESONLY;
            case Insight::PlatformFileDialogOperations::SelectFolder: return FOS_PICKFOLDERS;
            case Insight::PlatformFileDialogOperations::SelectFile:   return FOF_FILESONLY;
            case Insight::PlatformFileDialogOperations::SelectAll:    return FOF_FILESONLY | FOS_PICKFOLDERS;
            default:
                break;
            }
            FAIL_ASSERT();
            return 0;
        }

        bool PlatformFileDialogWindows::OpenDialog(PlatformFileDialogOperations operation, std::string* selectedItem, const std::vector<FileDialogFilter>& fileFilters)
        {
            IFileOpenDialog* openDialogHandle;
            HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&openDialogHandle));
            ASSERT(SUCCEEDED(hr));

            openDialogHandle->SetOptions(PlatformFileDialogOperationsToFileDialogOptions(operation));

            if (!fileFilters.empty())
            {
                std::vector<COMDLG_FILTERSPEC> fileTypes;
                fileTypes.resize(fileFilters.size());
                for (size_t i = 0; i < fileTypes.size(); ++i)
                {
                    fileTypes.at(i).pszName = fileFilters.at(i).Name;
                    fileTypes.at(i).pszSpec = fileFilters.at(i).Extension;
                }

                hr = openDialogHandle->SetFileTypes(static_cast<unsigned int>(fileTypes.size()), fileTypes.data());
                if (hr != S_OK)
                {
                    openDialogHandle->Release();
                    return false;
                }
            }

            hr = openDialogHandle->Show(NULL);
            if (hr != S_OK)
            {
                openDialogHandle->Release();
                return false;
            }

            IShellItem* item;
            hr = openDialogHandle->GetResult(&item);
            if (hr != S_OK)
            {
                IS_LOG_CORE_ERROR("[PlatformFileDialogWindows::OpenDialog] Unable to get selected item.");
                item->Release();
                openDialogHandle->Release();
                return false;
            }

            PWSTR filePath;
            hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
            if (hr != S_OK)
            {
                IS_LOG_CORE_ERROR("[PlatformFileDialogWindows::OpenDialog] Unable to get selected item display name.");
                item->Release();
                openDialogHandle->Release();
                return false;
            }

            *selectedItem = Platform::StringFromWString(filePath);
            if ((*selectedItem).back() == '\0')
            {
                (*selectedItem).pop_back();
            }
            FileSystem::PathToUnix(*selectedItem);

            item->Release();
            openDialogHandle->Release();

            return true;
        }

        bool PlatformFileDialogWindows::SaveDialog(PlatformFileDialogOperations operation, std::string* selectedItem, const std::vector<FileDialogFilter>& fileFilters, bool appendExtension)
        {
            IFileSaveDialog* saveDialogHandle;
            HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&saveDialogHandle));
            ASSERT(SUCCEEDED(hr));

            hr = saveDialogHandle->SetOptions(PlatformFileDialogOperationsToFileDialogOptions(operation));
            if (hr != S_OK)
            {
                saveDialogHandle->Release();
                return false;
            }

            if (!fileFilters.empty())
            {
                std::vector<COMDLG_FILTERSPEC> fileTypes;
                fileTypes.resize(fileFilters.size());
                for (size_t i = 0; i < fileTypes.size(); ++i)
                {
                    fileTypes.at(i).pszName = fileFilters.at(i).Name;
                    fileTypes.at(i).pszSpec = fileFilters.at(i).Extension;
                }

                hr = saveDialogHandle->SetFileTypes(static_cast<unsigned int>(fileTypes.size()), fileTypes.data());
                if (hr != S_OK)
                {
                    saveDialogHandle->Release();
                    return false;
                }
            }

            hr = saveDialogHandle->Show(NULL);
            if (hr != S_OK)
            {
                saveDialogHandle->Release();
                return false;
            }

            IShellItem* item;
            hr = saveDialogHandle->GetResult(&item);
            if (hr != S_OK)
            {
                IS_LOG_CORE_ERROR("[PlatformFileDialogWindows::OpenDialog] Unable to get selected item.");
                item->Release();
                saveDialogHandle->Release();
                return false;
            }

            PWSTR filePath;
            hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
            if (hr != S_OK)
            {
                IS_LOG_CORE_ERROR("[PlatformFileDialogWindows::OpenDialog] Unable to get selected item display name.");
                item->Release();
                saveDialogHandle->Release();
                return false;
            }

            *selectedItem = Platform::StringFromWString(filePath);
            if ((*selectedItem).back() == '\0')
            {
                (*selectedItem).pop_back();
            }
            FileSystem::PathToUnix(*selectedItem);

            if (appendExtension)
            {
                ASSERT(fileFilters.size() > 0);
                std::string extension = Platform::StringFromWString(fileFilters.at(0).Extension);
                if (extension.front() == '*')
                {
                    extension = extension.substr(1);
                }

                std::string newPath =FileSystem::ReplaceExtension(*selectedItem, extension);
                if (!newPath.empty())
                {
                    *selectedItem = std::move(newPath);
                }
            }

            item->Release();
            saveDialogHandle->Release();

            return true;
        }
    }
}

#endif // IS_PLATFORM_WINDOWS
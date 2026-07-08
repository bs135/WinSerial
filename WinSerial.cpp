// ---------------------------------------------------------------------------
// MODIFICATION NOTICE:
// This file has been modified by BS (thanhhai135@gmail.com) on 07/02/2026.
//
// Changes made:
// - Translated all comments and string literals to English.
// - Format source code C++ style
// - Add Keyword Highlighting
// - Add Show console help menu
// - Add Show About dialog
// - Add Show portName in the console title
// ---------------------------------------------------------------------------
// WinSerial.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WinSerial.h"
#include "Version.h"
#include <vector>
#include <string>
#include <iostream>
#include <regex>
#include <boost/asio.hpp>
#include <boost/asio/windows/stream_handle.hpp>

// Global variables:
HINSTANCE hInstance;
INT_PTR CALLBACK AboutFunc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingFunc(HWND, UINT, WPARAM, LPARAM);

// Global flag to track Ctrl+A status
bool g_isCtrlAPressed = false;

// Global flag to control program exit
bool g_shouldExit = false;

// Global flag to track the automatic keyword highlighting (Default is enabled)
bool g_enableHighlight = true;

std::vector<uint8_t> g_lineBuffer;
std::vector<uint8_t> g_recvLineBuffer;

using PortsArray = std::vector<std::pair<std::wstring, int>>;

const std::string ANSI_RESET = "\x1b[0m";
const std::string ANSI_RED = "\x1b[31m";
const std::string ANSI_GREEN = "\x1b[32m";
const std::string ANSI_YELLOW = "\x1b[33m";
const std::string ANSI_CYAN = "\x1b[36m";

static PortsArray GetAllPorts(void)
{
    PortsArray ports;
    HKEY hRegKey;
    int nCount = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Hardware\\DeviceMap\\SerialComm", 0, KEY_READ, &hRegKey) == ERROR_SUCCESS)
    {
        while (true)
        {
            TCHAR szName[MAX_PATH] = {0};
            TCHAR szPort[MAX_PATH] = {0};
            DWORD nValueSize = MAX_PATH - 1;
            DWORD nDataSize = MAX_PATH - 1;
            DWORD nType;

            if (::RegEnumValue(hRegKey, nCount, szName, &nValueSize, NULL, &nType, (LPBYTE)szPort, &nDataSize) == ERROR_NO_MORE_ITEMS)
            {
                break;
            }
            std::wstring name(szName);
            auto idx = name.find_last_of('\\');
            if (idx != name.npos)
            {
                name = name.substr(idx + 1);
            }
            name += L" (";
            name += szPort;
            name += L")";
            ports.push_back(std::make_pair(name, (int)std::wcstoul(szPort + 3, nullptr, 10)));
            nCount++;
        }
        ::RegCloseKey(hRegKey);
    }
    return ports;
}

static void UpdatePortControl(HWND hDlg)
{
    auto allPorts = GetAllPorts();
    auto hWndPort = GetDlgItem(hDlg, IDC_COMBO_PORT);
    ComboBox_ResetContent(hWndPort);
    for (auto port : allPorts)
    {
        auto index = ComboBox_AddString(hWndPort, port.first.c_str());
        ComboBox_SetItemData(hWndPort, index, port.second);
    }
}

static void CenterParentWindow(HWND hWnd)
{
    RECT rcDlg;
    ::GetWindowRect(hWnd, &rcDlg);
    RECT rcParent;
    HWND hWndParent = GetParent(hWnd);
    GetClientRect(hWndParent, &rcParent);
    POINT ptParentInScreen;
    ptParentInScreen.x = rcParent.left;
    ptParentInScreen.y = rcParent.top;
    ::ClientToScreen(hWndParent, (LPPOINT)&ptParentInScreen);
    SetWindowPos(
        hWnd,
        NULL,
        ptParentInScreen.x + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2,
        ptParentInScreen.y + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2,
        0,
        0,
        SWP_NOZORDER | SWP_NOSIZE);
}

typedef struct
{
    DWORD Serial;
    DWORD BaudRate;
    DWORD WordLength;
    DWORD StopBit;
    DWORD Parity;
    DWORD FlowControl;
    DWORD EncodingFormat;      // Encoding Format      : 0=UTF8, 1=GBK
    DWORD EchoMode;            // Echo Mode            : 0=Off(Char mode), 1=On(Line mode)
    DWORD KeywordHighlighting; // Keyword Highlighting : 0=Off, 1=On
} SERIAL_CONFIG;

static void WriteSerialConfig(const SERIAL_CONFIG &cfg)
{
    HKEY hKey;
    if (ERROR_SUCCESS == ::RegCreateKey(HKEY_CURRENT_USER, L"SOFTWARE\\WinSerial", &hKey))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        ::RegSetValueEx(hKey, L"Serial", 0, dwType, (CONST LPBYTE)&cfg.Serial, dwSize);
        ::RegSetValueEx(hKey, L"BaudRate", 0, dwType, (CONST LPBYTE)&cfg.BaudRate, dwSize);
        ::RegSetValueEx(hKey, L"WordLength", 0, dwType, (CONST LPBYTE)&cfg.WordLength, dwSize);
        ::RegSetValueEx(hKey, L"StopBit", 0, dwType, (CONST LPBYTE)&cfg.StopBit, dwSize);
        ::RegSetValueEx(hKey, L"Parity", 0, dwType, (CONST LPBYTE)&cfg.Parity, dwSize);
        ::RegSetValueEx(hKey, L"FlowControl", 0, dwType, (CONST LPBYTE)&cfg.FlowControl, dwSize);
        ::RegSetValueEx(hKey, L"EncodingFormat", 0, dwType, (CONST LPBYTE)&cfg.EncodingFormat, dwSize);
        ::RegSetValueEx(hKey, L"EchoMode", 0, dwType, (CONST LPBYTE)&cfg.EchoMode, dwSize);
        ::RegSetValueEx(hKey, L"KeywordHighlighting", 0, dwType, (CONST LPBYTE)&cfg.KeywordHighlighting, dwSize);
        ::RegCloseKey(hKey);
    }
}

static void ToggleEncodingFormat(SERIAL_CONFIG &cfg)
{
    // Toggle encoding format
    cfg.EncodingFormat = (cfg.EncodingFormat == 0) ? 1 : 0;

    // Apply new encoding format
    if (cfg.EncodingFormat == 0) // UTF-8
    {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::cout << std::endl
                  << "\033[32mConsole encoding: UTF-8\033[0m" << std::endl;
    }
    else // GBK
    {
        SetConsoleOutputCP(936);
        SetConsoleCP(936);
        std::cout << std::endl
                  << "\033[32mConsole encoding: GBK\033[0m" << std::endl;
    }

    // Save configuration to registry
    WriteSerialConfig(cfg);
}

static SERIAL_CONFIG ReadSerialConfig()
{
    HKEY hKey;
    SERIAL_CONFIG cfg;
    cfg.Serial = 0;
    cfg.BaudRate = 9600;
    cfg.WordLength = 8;
    cfg.StopBit = ONESTOPBIT;
    cfg.Parity = NOPARITY;
    cfg.FlowControl = 0;
    cfg.EncodingFormat = 0;      // Default to UTF-8 encoding
    cfg.EchoMode = 0;            // Default echo mode off
    cfg.KeywordHighlighting = 1; // Default keyword highlighting on
    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\WinSerial", 0, KEY_READ, &hKey))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        ::RegQueryValueEx(hKey, L"Serial", 0, &dwType, (LPBYTE)&cfg.Serial, &dwSize);
        ::RegQueryValueEx(hKey, L"BaudRate", 0, &dwType, (LPBYTE)&cfg.BaudRate, &dwSize);
        ::RegQueryValueEx(hKey, L"WordLength", 0, &dwType, (LPBYTE)&cfg.WordLength, &dwSize);
        ::RegQueryValueEx(hKey, L"StopBit", 0, &dwType, (LPBYTE)&cfg.StopBit, &dwSize);
        ::RegQueryValueEx(hKey, L"Parity", 0, &dwType, (LPBYTE)&cfg.Parity, &dwSize);
        ::RegQueryValueEx(hKey, L"FlowControl", 0, &dwType, (LPBYTE)&cfg.FlowControl, &dwSize);
        ::RegQueryValueEx(hKey, L"EncodingFormat", 0, &dwType, (LPBYTE)&cfg.EncodingFormat, &dwSize);
        ::RegQueryValueEx(hKey, L"EchoMode", 0, &dwType, (LPBYTE)&cfg.EchoMode, &dwSize);
        ::RegQueryValueEx(hKey, L"KeywordHighlighting", 0, &dwType, (LPBYTE)&cfg.KeywordHighlighting, &dwSize);
        ::RegCloseKey(hKey);
    }
    // Update the global flag
    g_enableHighlight = (cfg.KeywordHighlighting == 1);
    return cfg;
}

static void ShowHelp(SERIAL_CONFIG &cfg)
{
    std::cout << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << "\033[36mWinSerial " << APP_VERSION_FULL << "\033[0m" << std::endl;
    std::cout << std::endl;
    std::cout << "Current configurations:" << std::endl;
    std::cout << "\033[32m  Keyword Highlighting: " << (cfg.KeywordHighlighting == 0 ? "Off" : "On") << "\033[0m" << std::endl;
    std::cout << "\033[32m      Console encoding: " << (cfg.EncodingFormat == 0 ? "UTF-8" : "GBK") << "\033[0m" << std::endl;
    std::cout << "\033[32m             Echo mode: " << (cfg.EchoMode == 0 ? "Off" : "On") << "\033[0m" << std::endl;
    std::cout << std::endl;
    std::cout << "Hotkeys (You must press Ctrl+A first to enter Command Mode," << std::endl;
    std::cout << "         followed by one of the action keys below):" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+F: Toggle automatic Keyword Highlighting (On / Off)\033[0m" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+C: Toggle console Encoding format (UTF-8 / GBK)\033[0m" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+E: Toggle local Echo mode (On / Off)\033[0m" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+I: Display application version and information dialog\033[0m" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+H: Display this help menu\033[0m" << std::endl;
    std::cout << "\033[33m  Ctrl+A, Ctrl+X: Safely disconnect and exit the application\033[0m" << std::endl;
    std::cout << "------------------------" << std::endl;
    std::cout << std::endl;
}

static void ToggleEchoMode(SERIAL_CONFIG &cfg)
{
    // Toggle echo mode
    cfg.EchoMode = (cfg.EchoMode == 0) ? 1 : 0;

    // Clear line buffer
    g_lineBuffer.clear();

    // Display current echo mode
    std::cout << std::endl
              << "\033[32mEcho mode: " << (cfg.EchoMode == 0 ? "Off" : "On") << "\033[0m" << std::endl;

    // Save configuration to registry
    WriteSerialConfig(cfg);
}

static void ToggleKeywordHighlighting(SERIAL_CONFIG &cfg)
{
    // Toggle auto keyword color highlighting
    cfg.KeywordHighlighting = (cfg.KeywordHighlighting == 1) ? 0 : 1;
    g_enableHighlight = (cfg.KeywordHighlighting == 1);

    // Clear line buffer
    g_lineBuffer.clear();

    // Display current highlighting mode
    std::cout << std::endl
              << "\033[32mKeyword Highlighting: " << (g_enableHighlight ? "On" : "Off") << "\033[0m" << std::endl;

    // Save configuration to registry
    WriteSerialConfig(cfg);
}

static std::string ColorizeString(const std::string &input)
{
    std::string output = input;

    // 1. Red (Error/Failures)
    std::regex red_regex("\\b(error|failed|no|down|disabled|unknown|fault|shutdown|disconnected|denied|refused|problem|failure)\\b", std::regex_constants::icase);
    output = std::regex_replace(output, red_regex, ANSI_RED + "$1" + ANSI_RESET);

    // 2.Green (Success/Enabled)
    std::regex green_regex("\\b(enabled|connected|up|yes|ok|success)\\b", std::regex_constants::icase);
    output = std::regex_replace(output, green_regex, ANSI_GREEN + "$1" + ANSI_RESET);

    // 3. Yellow (Network/IP/MAC)
    std::regex yellow_regex("(\\b(?:\\d{1,3}\\.){3}\\d{1,3}\\b|\\b(?:[0-9a-fA-F]{2}[:-]){5}[0-9a-fA-F]{2}\\b|\\blocalhost\\b|\\bvlan\\b)", std::regex_constants::icase);
    output = std::regex_replace(output, yellow_regex, ANSI_YELLOW + "$1" + ANSI_RESET);

    // 4. Cyan (Config)
    std::regex cyan_regex("\\b(ip address|show|interface|route|spanning-tree)\\b", std::regex_constants::icase);
    output = std::regex_replace(output, cyan_regex, ANSI_CYAN + "$1" + ANSI_RESET);

    return output;
}

static boost::system::error_code InitializeSerialPort(boost::asio::serial_port &serialPort, const SERIAL_CONFIG &cfg, boost::system::error_code &ec)
{
    serialPort.set_option(boost::asio::serial_port::baud_rate(cfg.BaudRate), ec);
    if (ec)
        return ec;
    serialPort.set_option(boost::asio::serial_port::character_size(cfg.WordLength), ec);
    if (ec)
        return ec;
    switch (cfg.StopBit)
    {
    case 0:
        serialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), ec);
        break;
    case 1:
        serialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::onepointfive), ec);
        break;
    case 2:
        serialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::two), ec);
        break;
    default:
        serialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), ec);
        break;
    }
    if (ec)
        return ec;

    switch (cfg.Parity)
    {
    case 0:
        serialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), ec);
        break;
    case 1:
        serialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::odd), ec);
        break;
    case 2:
        serialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::even), ec);
        break;
    default:
        serialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), ec);
        break;
    }
    if (ec)
        return ec;

    switch (cfg.FlowControl)
    {
    case 0:
        serialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none), ec);
        break;
    case 1:
        serialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::software), ec);
        break;
    case 2:
        serialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::hardware), ec);
        break;
    default:
        serialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none), ec);
        break;
    }

    return ec;
}

template <class TStream1, class TStream2>
static void DoStreamToStream(TStream1 &stream1, TStream2 &stream2, std::vector<uint8_t> &buffer, SERIAL_CONFIG *pCfg, boost::asio::steady_timer *pTimer)
{
    stream1.async_read_some(
        boost::asio::buffer(buffer.data(), buffer.size()),
        [&stream1, &stream2, &buffer, pCfg, pTimer](const boost::system::error_code &ec, std::size_t bytes_transferred)
        {
            if (ec)
            {
                std::cerr << "\033[31m" << "error : " << ec.message() << "\033[0m" << std::endl;
            }
            else
            {
                // Check if keyboard input needs special handling (only if stream1 is standard input and config pointer is provided)
                bool skipWrite = false;
                if (pCfg != nullptr)
                {
                    for (size_t i = 0; i < bytes_transferred; i++)
                    {
                        uint8_t ch = buffer[i];

                        // Check for Ctrl+A (ASCII 1)
                        if (ch == 1)
                        {
                            g_isCtrlAPressed = true;
                            skipWrite = true;
                            break;
                        }

                        // If Ctrl+A is pressed, check the next key
                        if (g_isCtrlAPressed)
                        {
                            g_isCtrlAPressed = false; // Reset flag
                            skipWrite = true;

                            if (ch == 24) // Ctrl+X: Exit program
                            {
                                if (MessageBoxW(NULL, L"Are you sure you want to exit?", L"Exit Confirmation", MB_YESNO | MB_ICONQUESTION) == IDYES)
                                {
                                    g_shouldExit = true;
                                    // Exit io_service by canceling all asynchronous operations
                                    stream1.cancel();
                                    stream2.cancel();
                                    exit(0);
                                }
                            }
                            else if (ch == 3) // Ctrl+C: Toggle encoding format
                            {
                                ToggleEncodingFormat(*pCfg);
                            }
                            else if (ch == 5) // Ctrl+E: Toggle echo mode
                            {
                                ToggleEchoMode(*pCfg);
                            }
                            else if (ch == 6) // Ctrl+F: Toggle keyword highlighting
                            {
                                ToggleKeywordHighlighting(*pCfg);
                            }
                            else if (ch == 8) // Ctrl+H: Show Help
                            {
                                ShowHelp(*pCfg);
                            }
                            else if (ch == 9) // Ctrl+I: Show About
                            {
                                HWND hWndParent = GetForegroundWindow();
                                DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWndParent, AboutFunc);
                            }
                            break;
                        }
                    }

                    // If in echo mode and not a special key, process line buffering
                    if (!skipWrite && pCfg->EchoMode == 1)
                    {
                        skipWrite = true; // Always skip direct write, handled by line mode

                        for (size_t i = 0; i < bytes_transferred; i++)
                        {
                            uint8_t ch = buffer[i];

                            // Handle Backspace or Delete key
                            if (ch == 8 || ch == 127)
                            {
                                if (!g_lineBuffer.empty())
                                {
                                    g_lineBuffer.pop_back();
                                    // Output backspace, space, backspace to clear character on screen
                                    std::cout << "\b \b";
                                }
                                continue;
                            }

                            // Handle Carriage Return / Line Feed
                            if (ch == '\r' || ch == '\n')
                            {
                                // Add CR LF
                                g_lineBuffer.push_back('\r');
                                g_lineBuffer.push_back('\n');

                                // Write to serial port
                                boost::asio::write(stream2, boost::asio::buffer(g_lineBuffer.data(), g_lineBuffer.size()));

                                // Clear buffer
                                g_lineBuffer.clear();

                                // Display newline to terminal
                                std::cout << "\n";
                                continue;
                            }

                            // Normal character, add to buffer and echo
                            g_lineBuffer.push_back(ch);
                            std::cout << static_cast<char>(ch);
                            std::cout.flush(); // Ensure immediate display
                        }
                    }
                }

                // Perform normal data stream transmission only when no special handling is needed and not in echo mode
                // =====================================================================
                // STREAM 1: DATA FROM SERIAL PORT -> OUTPUT TO CONSOLE (WITH COLORING)
                // =====================================================================
                if (!skipWrite && pCfg == nullptr)
                {
                    for (size_t i = 0; i < bytes_transferred; i++)
                    {
                        uint8_t ch = buffer[i];
                        g_recvLineBuffer.push_back(ch);

                        // Flush buffer on newline, or when buffer is full
                        if (ch == '\n' || ch == '\r' || g_recvLineBuffer.size() > 4096)
                        {
                            // Convert buffer to string
                            std::string line(g_recvLineBuffer.begin(), g_recvLineBuffer.end());

                            // Apply keyword highlighting
                            std::string outputLine = g_enableHighlight ? ColorizeString(line) : line;

                            // Print directly to console
                            std::cout << outputLine;
                            std::cout.flush();

                            // Clear buffer to receive subsequent data
                            g_recvLineBuffer.clear();
                        }
                    }

                    // Process debounce timer to prevent incomplete log lines
                    if (pTimer != nullptr)
                    {
                        pTimer->expires_after(std::chrono::milliseconds(50));
                        pTimer->async_wait([](const boost::system::error_code &error)
                                           {
                            if (!error && !g_recvLineBuffer.empty())
                            {
                                std::string line(g_recvLineBuffer.begin(), g_recvLineBuffer.end());
                                std::string outputLine = g_enableHighlight ? ColorizeString(line) : line;
                                std::cout << outputLine;
                                std::cout.flush();
                                g_recvLineBuffer.clear();
                            } });
                    }

                    // Since synchronous std::cout is used instead of async_write,
                    // DoStreamToStream must be re-invoked immediately to continue the data reception loop
                    if (!g_shouldExit)
                    {
                        DoStreamToStream(stream1, stream2, buffer, pCfg, pTimer);
                    }
                }
                // =====================================================================
                // STREAM 2: DATA FROM KEYBOARD -> SEND TO SERIAL PORT (NO LOCAL ECHO)
                // =====================================================================
                else if (!skipWrite && pCfg != nullptr && pCfg->EchoMode == 0)
                {
                    boost::asio::async_write(
                        stream2,
                        boost::asio::const_buffer(buffer.data(), bytes_transferred),
                        [&stream1, &stream2, &buffer, pCfg, pTimer](const boost::system::error_code &ec, std::size_t bytes_transferred)
                        {
                            if (ec)
                            {
                                std::cerr << "\033[31m" << "error : " << ec.message() << "\033[0m" << std::endl;
                            }
                            else
                            {
                                DoStreamToStream(stream1, stream2, buffer, pCfg, pTimer);
                            }
                        });
                }
                else if (!g_shouldExit)
                {
                    // Continue listening for input
                    DoStreamToStream(stream1, stream2, buffer, pCfg, pTimer);
                }
            }
        });
}

static boost::system::error_code DoWork(boost::asio::io_context &ioctx, boost::asio::serial_port &serialPort, SERIAL_CONFIG &cfg)
{
    boost::system::error_code ec;
    boost::asio::windows::stream_handle stdinput(ioctx);
    boost::asio::windows::stream_handle stdoutput(ioctx);
    const auto kBufferSize = 1024;
    std::vector<uint8_t> serialPortRecvBuffer;
    std::vector<uint8_t> serialPortSendBuffer;
    serialPortRecvBuffer.resize(kBufferSize);
    serialPortSendBuffer.resize(kBufferSize);

    auto conin = CreateFile(L"CONIN$", FILE_GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    auto conout = CreateFile(L"CONOUT$", FILE_GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

    if (stdinput.assign(conin, ec))
        return ec;

    if (stdoutput.assign(conout, ec))
        return ec;

    boost::asio::steady_timer flushTimer(ioctx);

    // No special handling needed for stream from serial port to output
    DoStreamToStream(serialPort, stdoutput, serialPortRecvBuffer, nullptr, &flushTimer);
    // Pass config pointer for input to serial stream to support shortcut keys
    DoStreamToStream(stdinput, serialPort, serialPortSendBuffer, &cfg, nullptr);

    // Reset exit flag
    g_shouldExit = false;

    ioctx.run();
    return ec;
}

int wmain(int argc, const WCHAR *args[])
{
    boost::system::error_code ec;
    boost::asio::io_context ioctx;
    boost::asio::serial_port serialPort(ioctx);
    hInstance = GetModuleHandle(nullptr);

    DWORD consoleMode = 0;
    auto conin = GetStdHandle(STD_INPUT_HANDLE);
    auto conout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(conin, &consoleMode);
    consoleMode |= ENABLE_MOUSE_INPUT;
    consoleMode &= ~ENABLE_ECHO_INPUT;
    consoleMode &= ~ENABLE_PROCESSED_INPUT;
    consoleMode &= ~ENABLE_LINE_INPUT;
    consoleMode |= ENABLE_QUICK_EDIT_MODE;
    consoleMode |= ENABLE_WINDOW_INPUT;
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(conin, consoleMode);

    GetConsoleMode(conout, &consoleMode);
    SetConsoleMode(conout, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);

    while (true)
    {
        auto hWndParent = ::GetForegroundWindow();
        if (hWndParent == nullptr)
            hWndParent = GetConsoleWindow();
        if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SETTING_DIALOG), hWndParent, SettingFunc, 1) == IDOK)
        {
            ioctx.restart();
            auto cfg = ReadSerialConfig();
            auto portName = std::string("COM") + std::to_string(cfg.Serial);
            if (serialPort.open(portName, ec))
            {
                std::cerr << "\033[31m" << "can not open " << portName << "\033[0m" << std::endl;
                std::cerr << "\033[31m" << "error : " << ec.message() << "\033[0m" << std::endl;
                continue;
            }
            if (InitializeSerialPort(serialPort, cfg, ec))
            {
                std::cerr << "\033[31m" << "can not initialize " << portName << "\033[0m" << std::endl;
                std::cerr << "\033[31m" << "error : " << ec.message() << "\033[0m" << std::endl;
                continue;
            }
            else
            {
                // Set console according to user's selected encoding format
                if (cfg.EncodingFormat == 0) // UTF-8
                {
                    SetConsoleOutputCP(CP_UTF8);
                    SetConsoleCP(CP_UTF8);
                }
                else // GBK
                {
                    SetConsoleOutputCP(936); // 936 is the codepage for GBK
                    SetConsoleCP(936);
                }

                // Display current encoding format
                std::cout << "\033[36mWinSerial " << APP_VERSION_STR << "\033[0m" << std::endl;
                std::cout << "\033[33mPress Ctrl+A then Ctrl+H for help\033[0m" << std::endl;
                std::cout << std::endl;

                // Update title
                std::wstring wPortName(portName.begin(), portName.end());
                std::wstring appTitle = L"WinSerial - " + wPortName;
                SetConsoleTitle(appTitle.c_str());

                // Run work loop, passing configuration
                DoWork(ioctx, serialPort, cfg);

                // Check if program needs to exit
                if (g_shouldExit)
                {
                    break;
                }
            }
            break;
        }
        else
        {
            return ERROR_CANCELLED;
        }
    }
    return ERROR_SUCCESS;
}

// Message handler for the "About" box.
INT_PTR CALLBACK AboutFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        CenterParentWindow(hDlg);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK SettingFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        CenterParentWindow(hDlg);
        auto cfg = ReadSerialConfig();
        auto hWndPort = GetDlgItem(hDlg, IDC_COMBO_PORT);
        UpdatePortControl(hDlg);
        auto portCount = ComboBox_GetCount(hWndPort);
        if (cfg.Serial != 0)
        {
            for (int i = 0; i < portCount; ++i)
            {
                auto com = (int)ComboBox_GetItemData(hWndPort, i);
                if (com == cfg.Serial)
                {
                    ComboBox_SetCurSel(hWndPort, i);
                    break;
                }
            }
        }
        else
        {
            ComboBox_SetCurSel(hWndPort, 0);
        }

        auto hWndBaudRate = GetDlgItem(hDlg, IDC_COMBO_SPEED);
        ComboBox_AddString(hWndBaudRate, L"50");
        ComboBox_AddString(hWndBaudRate, L"75");
        ComboBox_AddString(hWndBaudRate, L"100");
        ComboBox_AddString(hWndBaudRate, L"105");
        ComboBox_AddString(hWndBaudRate, L"300");
        ComboBox_AddString(hWndBaudRate, L"600");
        ComboBox_AddString(hWndBaudRate, L"1200");
        ComboBox_AddString(hWndBaudRate, L"2400");
        ComboBox_AddString(hWndBaudRate, L"4800");
        ComboBox_AddString(hWndBaudRate, L"9600");
        ComboBox_AddString(hWndBaudRate, L"19200");
        ComboBox_AddString(hWndBaudRate, L"38400");
        ComboBox_AddString(hWndBaudRate, L"57600");
        ComboBox_AddString(hWndBaudRate, L"115200");
        ComboBox_AddString(hWndBaudRate, L"128000");
        ComboBox_AddString(hWndBaudRate, L"256000");
        ComboBox_SetText(hWndBaudRate, std::to_wstring(cfg.BaudRate).c_str());

        auto hWndWordLength = GetDlgItem(hDlg, IDC_COMBO_WORD);
        ComboBox_AddString(hWndWordLength, L"4");
        ComboBox_AddString(hWndWordLength, L"5");
        ComboBox_AddString(hWndWordLength, L"6");
        ComboBox_AddString(hWndWordLength, L"7");
        ComboBox_AddString(hWndWordLength, L"8");
        ComboBox_AddString(hWndWordLength, L"9");
        ComboBox_AddString(hWndWordLength, L"10");
        ComboBox_SetCurSel(hWndWordLength, (int)(cfg.WordLength - 4));

        auto hWndStopBit = GetDlgItem(hDlg, IDC_COMBO_STOP);
        ComboBox_AddString(hWndStopBit, L"1");
        ComboBox_AddString(hWndStopBit, L"1.5");
        ComboBox_AddString(hWndStopBit, L"2");
        ComboBox_SetCurSel(hWndStopBit, (int)(cfg.StopBit));

        auto hWndParity = GetDlgItem(hDlg, IDC_COMBO_PARITY);
        ComboBox_AddString(hWndParity, L"None");
        ComboBox_AddString(hWndParity, L"Odd");
        ComboBox_AddString(hWndParity, L"Even");
        ComboBox_SetCurSel(hWndParity, (int)(cfg.Parity));

        auto hWndFlowControl = GetDlgItem(hDlg, IDC_COMBO_FLOW_CONTROL);
        ComboBox_AddString(hWndFlowControl, L"None");
        ComboBox_AddString(hWndFlowControl, L"XON/XOFF");
        ComboBox_AddString(hWndFlowControl, L"RTS/CTS");
        ComboBox_SetCurSel(hWndFlowControl, (int)(cfg.FlowControl));

        auto hWndEncoding = GetDlgItem(hDlg, IDC_COMBO_ENCODING);
        ComboBox_AddString(hWndEncoding, L"UTF-8");
        ComboBox_AddString(hWndEncoding, L"GBK");
        ComboBox_SetCurSel(hWndEncoding, (int)(cfg.EncodingFormat));

        auto hWndEchoMode = GetDlgItem(hDlg, IDC_COMBO_ECHO);
        ComboBox_AddString(hWndEchoMode, L"Off");
        ComboBox_AddString(hWndEchoMode, L"On");
        ComboBox_SetCurSel(hWndEchoMode, (int)(cfg.EchoMode));

        auto hWndKeywordHighlighting = GetDlgItem(hDlg, IDC_COMBO_HIGHLIGHT);
        ComboBox_AddString(hWndKeywordHighlighting, L"Off");
        ComboBox_AddString(hWndKeywordHighlighting, L"On");
        ComboBox_SetCurSel(hWndKeywordHighlighting, (int)(cfg.KeywordHighlighting));

        return (INT_PTR)TRUE;
    }
    case WM_DEVICECHANGE:
        UpdatePortControl(hDlg);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
            {
                SERIAL_CONFIG cfg = {0};
                auto hWndPort = GetDlgItem(hDlg, IDC_COMBO_PORT);
                auto hWndBaudRate = GetDlgItem(hDlg, IDC_COMBO_SPEED);
                auto hWndWordLength = GetDlgItem(hDlg, IDC_COMBO_WORD);
                auto hWndStopBit = GetDlgItem(hDlg, IDC_COMBO_STOP);
                auto hWndParity = GetDlgItem(hDlg, IDC_COMBO_PARITY);
                auto hWndFlowControl = GetDlgItem(hDlg, IDC_COMBO_FLOW_CONTROL);
                auto hWndEncoding = GetDlgItem(hDlg, IDC_COMBO_ENCODING);
                auto hWndEchoMode = GetDlgItem(hDlg, IDC_COMBO_ECHO);
                auto hWndKeywordHighlighting = GetDlgItem(hDlg, IDC_COMBO_HIGHLIGHT);

                WCHAR txtBuffer[32] = {0};
                auto curSel = ComboBox_GetCurSel(hWndPort);
                if (curSel >= 0)
                {
                    cfg.Serial = (DWORD)ComboBox_GetItemData(hWndPort, curSel);
                }
                else
                {
                    ComboBox_GetText(hWndPort, txtBuffer, 32);
                    cfg.Serial = std::wcstoul(txtBuffer + 3, nullptr, 10);
                }
                ComboBox_GetText(hWndBaudRate, txtBuffer, 32);
                cfg.BaudRate = std::wcstoul(txtBuffer, nullptr, 10);
                cfg.WordLength = ComboBox_GetCurSel(hWndWordLength) + 4;
                cfg.StopBit = ComboBox_GetCurSel(hWndStopBit);
                cfg.Parity = ComboBox_GetCurSel(hWndParity);
                cfg.FlowControl = ComboBox_GetCurSel(hWndFlowControl);
                cfg.EncodingFormat = ComboBox_GetCurSel(hWndEncoding);
                cfg.EchoMode = ComboBox_GetCurSel(hWndEchoMode);
                cfg.KeywordHighlighting = ComboBox_GetCurSel(hWndKeywordHighlighting);

                WriteSerialConfig(cfg);
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

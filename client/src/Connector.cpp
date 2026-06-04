#include "stdafx.h"
#include "Connector.h"
#include "Application.h"
#include <windows.h>

#include <winhttp.h>
#include <wincrypt.h>

#include "..\Math.h"
#include "..\Shared\Config.h"
#include "..\Shared\SfSocket.h"
#include "..\nlohmann\json.hpp"
#include "..\StringHelpers.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")

Connector::Connector()
{
	m_socket = make_shared<TcpSocketEx>();
}

Connector::~Connector()
{

}

bool Connector::update()
{
	if (m_sfSocket == nullptr)
		return false;

	if (!m_sfSocket->update())
	{
		m_sfSocket = nullptr;
		return false;
	}

	return true;
}

void Connector::cancel()
{
	if (m_sfSocket != nullptr)
		m_sfSocket->cancel();

	m_sfSocket = nullptr;
}

void Connector::sendPacket(StlBuffer& outgoing)
{
	if (m_sfSocket != nullptr)
		m_sfSocket->sendPacket(outgoing);
}

void Connector::popReceived(vector<unique_ptr<StlBuffer>>& output)
{
	if (m_sfSocket != nullptr)
		m_sfSocket->popReceived(output);
}

bool Connector::connect()
{
	cancel();
	m_socket->setBlocking(true);

	if (m_socket->connect(sConfig->getString("Tcp", "Address"), sConfig->getInt("Tcp", "Port"),
		sf::seconds(static_cast<float>(sConfig->getInt("Tcp", "Timeout")))) != sf::Socket::Done)
	{
		return false;
	}

	m_socket->setBlocking(false);
	m_sfSocket = make_unique<SfSocket>(m_socket, SfSocket::Type::SocketType_ClientSide);
	return true;
}

bool Connector::connected() const
{
	return m_sfSocket != nullptr && m_sfSocket->connected();
}

bool Connector::attemptAuth(const std::wstring& host, const std::wstring& path, const std::string& username, const std::string& password,
    std::string& outToken, std::string& outError)
{
    using json = nlohmann::json;

    outToken.clear();
    outError.clear();

    string payload = Util::base64_encode(username + ":" + password);

    // Open session
    HINTERNET hSession = WinHttpOpen(L"GameClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession)
    {
        outError = "Failed to initialize connection";
        return false;
    }

    DWORD timeout = 30000;
    WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    // Connect (port from config; default 80 for the local node auth server)
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), (INTERNET_PORT)sConfig->getInt("Auth", "Port", 80), 0);
    if (!hConnect)
    {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hSession);

        if (err == ERROR_WINHTTP_NAME_NOT_RESOLVED)
            outError = "Could not reach server. Check your internet connection.";
        else
            outError = "Failed to connect to server";
        return false;
    }

    // Create request (TLS only if [Auth] Secure=1; default plain HTTP for the local node auth server)
    DWORD secureFlag = sConfig->getInt("Auth", "Secure", 0) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, NULL, NULL, secureFlag);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        outError = "Failed to create request";
        return false;
    }

    // Send request with User-Agent header (required by LiteSpeed)
    LPCWSTR headers = L"User-Agent: GameClient/1.0\r\nContent-Type: text/plain";
    if (!WinHttpSendRequest(hRequest, headers, (DWORD)-1, (LPVOID)payload.c_str(), (DWORD)payload.size(), (DWORD)payload.size(), 0))
    {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        switch (err)
        {
        case ERROR_WINHTTP_TIMEOUT:
            outError = "Connection timed out. Please try again.";
            break;
        case ERROR_WINHTTP_CANNOT_CONNECT:
            outError = "Could not connect to server. It may be down.";
            break;
        case ERROR_WINHTTP_SECURE_FAILURE:
            outError = "Secure connection failed. Check your system time.";
            break;
        default:
            outError = "Failed to send request";
        }
        return false;
    }

    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (err == ERROR_WINHTTP_TIMEOUT)
            outError = "Server took too long to respond. Please try again.";
        else
            outError = "Failed to receive response from server";
        return false;
    }

    // Check HTTP status
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &statusCode, &statusSize, NULL);

    // Read response body
    char buffer[2048] = { 0 };
    DWORD bytesRead = 0;
    std::string response;

    while (WinHttpReadData(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
    {
        response.append(buffer, bytesRead);
        bytesRead = 0;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Parse JSON response
    json jResponse;
    try
    {
        jResponse = json::parse(response);
    }
    catch (const json::parse_error&)
    {
        // Fallback for non-JSON responses
        switch (statusCode)
        {
        case 429:
            outError = "Too many attempts. Please wait a moment.";
            break;
        case 500:
        case 502:
        case 503:
            outError = "Server is temporarily unavailable. Please try later.";
            break;
        case 0:
            outError = "No response from server";
            break;
        default:
            outError = "Server error (" + std::to_string(statusCode) + ")";
        }
        return false;
    }

    // Handle successful auth
    if (statusCode == 200 && jResponse.value("success", false))
    {
        if (jResponse.contains("token"))
        {
            outToken = jResponse["token"].get<std::string>();
            return true;
        }
        outError = "Invalid response from server";
        return false;
    }

    // Handle errors - extract server's error message
    if (jResponse.contains("error"))
    {
        try
        {
            outError = jResponse["error"].get<std::string>();

            // Append ban expiry if present
            if (jResponse.contains("ban_seconds"))
            {
                outError += "\nBan expires: " + Util::formatTimeBySeconds(jResponse["ban_seconds"].get<int>());
            }
        }
        catch (...)
        {
            outError = "Received invalid response.";
            return false;
        }
    }
    else
    {
        // Fallback if no error field
        switch (statusCode)
        {
        case 200:
            outError = "Received invalid response. Try using a VPN or switching DNS to 1.1.1.1";
            break;
        case 401:
            outError = "Invalid username or password";
            break;
        case 403:
            outError = "Access denied";
            break;
        default:
            outError = "Authentication failed (" + std::to_string(statusCode) + ")";
        }
    }

    return false;
}
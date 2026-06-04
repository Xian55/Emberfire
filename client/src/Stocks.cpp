#include "stdafx.h"
#include "Stocks.h"
#include "ContentMgr.h"
#include "Sprite.h"
#include "Application.h"
#include "TextLines.h"
#include "ScrollBar.h"
#include "Button.h"

#include <Wininet.h>
#include <time.h>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <chrono>

#include "..\Math.h"
#include "..\StringHelpers.h"

#pragma comment (lib,"Wininet.lib")

Stocks::Stocks(RenderObjectHolder& owner, const int id) : 
	RenderObjectHolder(owner),
	m_shutdown(false),
	m_lastScrollVal(-1)
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
	ASSERT(m_bg = sContentMgr->spawnSprite("stockgui.jpg"));
	m_thread = thread(&Stocks::workerThread, this);
		
	// Symbols list	
	m_textSymbols = make_shared<TextLines>(*this, 0, "arial.ttf", Util::GeoBox2d(90, 125, 350, 900));
	m_textSymbols->setClickableLines(true);
	m_textSymbols->setDialogCharacterSize(24);
	m_textSymbols->setMaxLines(256);
		
	// Values list	
	m_textValues = make_shared<TextLines>(*this, 0, "arial.ttf", Util::GeoBox2d(440, 125, 350, 900));
	m_textValues->setClickableLines(true);
	m_textValues->setDialogCharacterSize(24);
	m_textValues->setMaxLines(256);
		
	// Cons list	
	m_textCons = make_shared<TextLines>(*this, 0, "arial.ttf", Util::GeoBox2d(705, 125, 350, 900));
	m_textCons->setClickableLines(true);
	m_textCons->setDialogCharacterSize(24);
	m_textCons->setMaxLines(256);
		
	// Cons list	
	m_textOverall = make_shared<TextLines>(*this, 0, "arial.ttf", Util::GeoBox2d(945, 125, 350, 900));
	m_textOverall->setClickableLines(true);
	m_textOverall->setDialogCharacterSize(24);
	m_textOverall->setMaxLines(256);
	
	m_scrollBar = make_shared<ScrollBar>(*this, "mapeditor_scroll_up", "mapeditor_scroll_down", ScrollBar::ScrollTopDown, "mapeditor_scroll_box");
	m_scrollBar->getScrollUpButton()->setPos({ 328, 115 });
	m_scrollBar->getScrollDownButton()->setPos({ 328, 820 });
	m_scrollBar->getScrollSquare()->setPos({ 328, 820 });
}

Stocks::~Stocks()
{
	m_shutdown = true;
	m_thread.join();
}
	
void Stocks::input() /*final*/
{	
	__super::input();
	m_scrollBar->input();
	m_scrollBar->setMaxOffset(m_symbols.size());
	m_textSymbols->attemptInput();
	m_textValues->attemptInput();
	m_textCons->attemptInput();
	m_textOverall->attemptInput();
}

void Stocks::render() /*final*/
{
	if (m_lastScrollVal != m_scrollBar->getScrollOffset())
	{
		m_lastScrollVal = m_scrollBar->getScrollOffset();
		fillTextData();
	}

	m_bg->renderStretch({ 0, 0 }, { sApplication->defaultSwf(), sApplication->defaultShf() });
	__super::render();
	m_scrollBar->render();
	m_textSymbols->attemptRender();
	m_textValues->attemptRender();
	m_textCons->attemptRender();
	m_textOverall->attemptRender();
}

void Stocks::workerThread()
{
	while (!m_shutdown)
	{
		if (auto data = RestHttp::get("https://scanner.tradingview.com/america/scan", "", "{\"symbols\":{\"tickers\":[],\"query\":{\"types\":[]}},\"columns\":[\"close\"]}"))
		{
			if (data->returnCode == S_OK && data->httpCode == 200)
				parse_tradingviewDotCom(data->response);
		}

		this_thread::sleep_for(5s);
	}
}

void Stocks::fillTextData()
{
	lock_guard<mutex> grd(m_mutex);

	if (m_scrollBar->getScrollOffset() >= int(m_symbols.size()))
		return;

	m_textSymbols->clear();
	m_textValues->clear();
	m_textCons->clear();
	m_textOverall->clear();

	auto itr = m_symbols.begin();
	advance(itr, m_scrollBar->getScrollOffset());

	int i = 0;

	while (itr != m_symbols.end() && i < 30)
	{
		Symbol* ptr = itr->second.get();

		m_textSymbols->addLine(ptr->getName());
		m_textValues->addLine(Util::fmtStr("%.2f", ptr->getValue()));
		m_textCons->addLine(to_string(ptr->getIncrement()));
		m_textOverall->addLine("0");

		++itr;
		++i;
	}
}

void Stocks::parse_tradingviewDotCom(const string& a_jsonResponse)
{
	if (a_jsonResponse.empty())
		return;

	size_t offset = 0;

	while (true)
	{	
		string symbolName;
		string symbolValue;

		// "s": "NASDAQ:DFPHU",
		constexpr auto skey = "\"s\":\"";
		auto s = a_jsonResponse.find(skey, offset);
		
		if (s != string::npos)
			s += strlen(skey);

		while (s != string::npos && s < a_jsonResponse.size())
		{
			if (a_jsonResponse[s] == '"')
				break;
		
			symbolName.push_back(a_jsonResponse[s]);
			++s;
		}
	
		// "d": [10.5]
		constexpr auto dkey = "\"d\":[";
		auto d = a_jsonResponse.find(dkey, s);
		
		if (d != string::npos)
			d += strlen(dkey);

		while (d != string::npos && d < a_jsonResponse.size())
		{
			if (a_jsonResponse[d] == ']')
				break;
		
			symbolValue.push_back(a_jsonResponse[d]);
			++d;
		}

		offset = d;

		if (d == string::npos || s == string::npos || s >= a_jsonResponse.size() || d >= a_jsonResponse.size())
			break; 

		// Critical section
		lock_guard<mutex> grd(m_mutex);

		if (m_symbols[symbolName] == nullptr)
			m_symbols[symbolName] = make_unique<Symbol>(symbolName);

		m_symbols[symbolName]->updateValue(static_cast<float>(atof(symbolValue.c_str())));
	}

	// Will trigger a redraw
	m_lastScrollVal = -1;
}

// Symbol

Symbol::Symbol(const string& name) :
	m_name(name),
	m_value(-1),
	m_curPosIncr(0),
	m_curNegIncr(0)
{

}

void Symbol::updateValue(const float value)
{
	if (m_value != value)
		appendHistory(value);

	m_value = value;
}
	
void Symbol::appendHistory(const float value)
{
	if (m_history.size() > 0)
	{
		float prev = m_history[m_history.size() - 1].second;

		// Negative incr
		if (prev > value)
		{
			if (m_curPosIncr != 0)
			{
				m_incrHistory.push_back(m_curPosIncr);
				m_curPosIncr = 0;
			}

			++m_curNegIncr;
		}

		// Positive incr
		if (prev < value)
		{
			if (m_curNegIncr != 0)
			{
				m_incrHistory.push_back(m_curNegIncr);
				m_curNegIncr = 0;
			}

			--m_curNegIncr;
		}
	}

	m_history.push_back({ chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count(), value });
}

void Symbol::loadHistorical()
{

}

void Symbol::saveHistorical()
{

}

int Symbol::getIncrement() const
{
	if (m_curPosIncr != 0)
		return m_curPosIncr;

	if (m_curNegIncr != 0)
		return m_curNegIncr;

	return 0;
}

// RestHttp

unique_ptr<RestHttp::Response> RestHttp::get(const string& a_uri, const string& a_headers, const string& a_body, const int a_timeout /*= 5000*/)
{
	return move(genericRequest(a_uri, "GET", "application/octet-stream", a_headers, a_body, a_timeout));
}
	
unique_ptr<RestHttp::Response> RestHttp::genericRequest(const string& a_uri, const string& a_method, const string& a_acceptType,
	const string& a_headers, const string& a_body, const int a_timeout)
{
	unique_ptr<RestHttp::Response> result = make_unique<RestHttp::Response>();
	
	// Parse URI
	URL_COMPONENTSA uri;
	uri.dwStructSize = sizeof(uri);
	uri.dwSchemeLength = 1;
	uri.dwHostNameLength = 1;
	uri.dwUserNameLength = 1;
	uri.dwPasswordLength = 1;
	uri.dwUrlPathLength = 1;
	uri.dwExtraInfoLength = 1;
	uri.lpszScheme = NULL;
	uri.lpszHostName = NULL;
	uri.lpszUserName = NULL;
	uri.lpszPassword = NULL;
	uri.lpszUrlPath = NULL;
	uri.lpszExtraInfo = NULL;

	if (::InternetCrackUrlA(a_uri.c_str(), a_uri.length(), 0, &uri))
	{
		// Open HTTP resource
		HINTERNET handle = ::InternetOpenA("GladiatorXjum", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);

		if (handle == NULL)
		{
			result->returnCode = ::GetLastError();
			return result;
		}

		// Setup timeout
		if (InternetSetOptionA(handle, INTERNET_OPTION_CONNECT_TIMEOUT, LPVOID(&a_timeout), sizeof(int)) == false)
		{
			result->returnCode = ::GetLastError();
			return result;
		}

		string scheme(uri.lpszScheme ? uri.lpszScheme : "", uri.dwSchemeLength); 		
		string serverName(uri.lpszHostName ? uri.lpszHostName : "", uri.dwHostNameLength);
		string object(uri.lpszUrlPath ? uri.lpszUrlPath : "", uri.dwUrlPathLength);
		string extraInfo(uri.lpszExtraInfo ? uri.lpszExtraInfo : "", uri.dwExtraInfoLength);
		string username(uri.lpszUserName ? uri.lpszUserName : "", uri.dwUserNameLength);
		string password(uri.lpszPassword ? uri.lpszPassword : "", uri.dwPasswordLength);

		// Connect to server
		if (HINTERNET session = ::InternetConnectA( handle, serverName.c_str(), uri.nPort, username.c_str(), password.c_str(), INTERNET_SERVICE_HTTP, 0, 0 ))
		{
			DWORD dwFlags = INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID | 
							INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS | 
							INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI | INTERNET_FLAG_RELOAD;

			if (scheme == "https" || scheme == "HTTPS")
				dwFlags |= INTERNET_FLAG_SECURE;

			// Define the Accept Types			
			PCSTR rgpszAcceptTypes[] = { a_acceptType.c_str(), NULL };
			
			if (HINTERNET istream = ::HttpOpenRequestA(session, a_method.c_str(), ( object + extraInfo ).c_str(), NULL, NULL, rgpszAcceptTypes, dwFlags, 0))
			{
				bool valid = true;

				// Headers
				if (a_headers.length() > 0)
					valid = ::HttpAddRequestHeadersA(istream, a_headers.c_str(), a_headers.length(), HTTP_ADDREQ_FLAG_ADD);

				if (valid)
				{
					// Ignore certificate verification 
					DWORD nFlags = 0;
					DWORD nBuffLen = sizeof(nFlags);
					::InternetQueryOptionA(istream, INTERNET_OPTION_SECURITY_FLAGS, LPVOID(&nFlags), &nBuffLen);
					nFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
					::InternetSetOptionA(istream, INTERNET_OPTION_SECURITY_FLAGS, &nFlags, sizeof(nFlags));

					// Send
					if (a_body.empty())
						valid = ::HttpSendRequestA(istream, NULL, 0, NULL, 0);
					else
						valid = ::HttpSendRequestA(istream, NULL,0, LPVOID(a_body.c_str()), a_body.size());
				
					if (valid)
					{
						// Http code
						// Microsoft wants an address to the value for some reason...
						DWORD httpcodesize = sizeof(result->httpCode);
						::HttpQueryInfoA(istream, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &result->httpCode, &httpcodesize, NULL);
						
						// Response headers
						LPVOID lpOutBuffer = nullptr;
						DWORD dwSize = 0;
						::HttpQueryInfoA(istream, HTTP_QUERY_RAW_HEADERS_CRLF, LPVOID(lpOutBuffer), &dwSize, NULL);

						if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
						{
							lpOutBuffer = new char[dwSize];
							::HttpQueryInfoA(istream, HTTP_QUERY_RAW_HEADERS_CRLF, LPVOID(lpOutBuffer), &dwSize, NULL);
							result->reponseHeaders = static_cast<char*>(lpOutBuffer);
							delete[] lpOutBuffer;
						}


						DWORD size = 0;
						BYTE buffer[4096];

						// Download
						while (::InternetReadFile(istream, buffer, 4096, &size) && size > 0)
						{
							string data(reinterpret_cast<char*>(buffer), size);
							result->response += data;
						}
					}
					else
					{
						result->returnCode = ::GetLastError();
					}
				}

				::InternetCloseHandle(istream);
			}
			else
			{
				result->returnCode = ::GetLastError();
			}

			::InternetCloseHandle(session);
		}
		else
		{
			result->returnCode = ::GetLastError();
		}		

		// Close internet handle
		::InternetCloseHandle( handle );
	}
	else
	{
		result->returnCode = ::GetLastError();
	}

	return result;
}
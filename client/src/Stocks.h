#pragma once

#include "RenderObjectHolder.h"

#include <map>
#include <thread>
#include <mutex>

class Sprite;
class Symbol;
class RestHttp;
class TextLines;
class ScrollBar;

class Stocks :	public RenderObjectHolder
{
public:
	Stocks(RenderObjectHolder& owner, const int id);
	virtual ~Stocks();

private:
	void input() final;
	void render() final;
	
	void workerThread();
	void parse_tradingviewDotCom(const string& a_jsonResponse);

	void fillTextData();

	bool m_shutdown;
	int m_lastScrollVal;

	mutex m_mutex;
	thread m_thread;

	shared_ptr<Sprite> m_bg;
	map<string, unique_ptr<Symbol>> m_symbols;

	shared_ptr<TextLines> m_textSymbols;
	shared_ptr<TextLines> m_textValues;
	shared_ptr<TextLines> m_textCons;
	shared_ptr<TextLines> m_textOverall;

	shared_ptr<ScrollBar> m_scrollBar;
};

class RestHttp
{
public:
	struct Response
	{
		Response() { returnCode = httpCode = 0; }
		DWORD returnCode;
		DWORD httpCode;
		string response;
		string reponseHeaders;
	};

	static unique_ptr<Response> get(const string& a_uri, const string& a_headers, const string& a_body, const int a_timeout = 5000);

private:
	static unique_ptr<Response> genericRequest(const string& a_uri, const string& a_method, const string& a_acceptType, const string& a_headers, const string& a_body, const int a_timeout = 5000);
};

class Symbol
{
public:
	Symbol(const string& name);

	void updateValue(const float value);

	const string& getName() const { return m_name; }

	float getValue() const { return m_value; }
	
	int getIncrement() const;

private:
	void loadHistorical();
	void saveHistorical();
	void appendHistory(const float val);

	float m_value;

	string m_name;

	int m_curPosIncr;
	int m_curNegIncr;

	vector<int> m_incrHistory;
	vector<pair<uint64_t, float>> m_history;
};
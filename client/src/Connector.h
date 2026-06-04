#pragma once

#include "..\Shared\StlBuffer.h"

#include <SFML/Network.hpp>

class SfSocket;
class TcpSocketEx;

class Connector
{
	public:
		void cancel();
		void sendPacket(StlBuffer& outgoing);
		void popReceived(vector<unique_ptr<StlBuffer>>& output);

		bool update();
		bool connect();
		bool connected() const;
		bool attemptAuth(const std::wstring& host, const std::wstring& path, const std::string& username, const std::string& password,
			std::string& outToken, std::string& outError);

	public:
		static Connector* instance()
		{
			static Connector c;
			return &c;
		}

	private:
		Connector();
		~Connector();

		unique_ptr<SfSocket> m_sfSocket;
		shared_ptr<TcpSocketEx> m_socket;
};

#define sConnector Connector::instance()

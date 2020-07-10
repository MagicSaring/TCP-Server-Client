#ifndef _EasyTcpServer_hpp_
#define _EasyTcpServer_hpp_

using namespace std;
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "MessageHeader.hpp"
#include "CELLTimeStamp.hpp"
#define  PORT 4567
#define  _CELLSERVER_THREAD_COUNT 4

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define FD_SETSIZE      2506
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.LIB")

#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define	SOCKET					int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#endif 

#include <thread>
#include <mutex>
#include <atomic>
//#include <functional>

#ifndef RECV_BUFF_SIZE
#define  RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd()
	{
		return _sockfd;
	}
	char* msgBuf()
	{
		return _szMsgBuf;
	}
	int getLastPos()
	{
		return _lastPos;
	}

	void setLastPos(int pos)
	{
		_lastPos = pos;
	}

private:
	SOCKET _sockfd;	//文件描述符

	//第二缓冲区,消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];

	//消息缓冲区的数据尾部位置
	int _lastPos;
};

class INetEvent
{
public:
	//纯虚函数
	//客户端离开事件
	virtual void OnLeave(ClientSocket *pClient) = 0;
	virtual void OnNetMsg(SOCKET sock, DataHeader* header) = 0;
};

class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		m_hSocket = sock;
		_pThread = nullptr;
		_recvCount = 0;
		_pNetEvent = nullptr;
	}
	~CellServer()
	{
		Close();
		m_hSocket = INVALID_SOCKET;
	}

	void setEventObj(INetEvent *event)
	{
		_pNetEvent = event;
	}

	bool OnRun()
	{
		while (IsRun())
		{
			if (_clientsBuff.size() > 0)
			{
				//从缓冲队里取出客户数据
				lock_guard<mutex> lock(_mutex);
				for (auto pClient : _clientsBuff)
				{
					_clients.push_back(pClient);
				}
				_clientsBuff.clear();
			}

			//如果没有需要处理的客户端,就跳过
			if (_clients.empty())
			{
				chrono::milliseconds t(1);
				this_thread::sleep_for(t);
				continue;
			}


			//伯克利套接字 BSD socket
			fd_set fdRead;

			//FD_ZERO:清空一个文件描述符集合
			FD_ZERO(&fdRead);

			//将描述符(Socket) 加入集合
			SOCKET maxSocket = _clients[0]->sockfd();
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				FD_SET(_clients[i]->sockfd(), &fdRead);
				if (_clients[i]->sockfd() > maxSocket)
				{
					maxSocket = _clients[i]->sockfd();
				}
			}

			/*
			select函数原型:
			int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);

			参数列表:
			nfds:		监听的所有文件描述符的最大描述符 + 1(内核采取轮询的方式)
			readfds:	读文件描述监听集合
			writefds:	写文件描述符集合
			exceptfds:	异常文件描述符集合
			timeout:	大于0:设置监听超时时长;NULL:阻塞监听;0:非阻塞监听

			返回值:
			大于0:所欲监听集合中,满足对应时间的总数
			0:没有满足的
			-1:出错error
			*/

			int ret = select(maxSocket + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0)
			{
				printf("select任务结束\n");
				Close();
				return false;
			}

			for (size_t i = 0; i < _clients.size(); ++i)
			{
				if (FD_ISSET(_clients[i]->sockfd(), &fdRead))
				{
					if (-1 == RecvData(_clients[i]))
					{
						auto iter = find(_clients.begin(), _clients.end(), _clients[i]);
						if (iter != _clients.end())
						{
							if (_pNetEvent)
							{
								_pNetEvent->OnLeave(_clients[i]);
							}
								
							delete _clients[i];
							_clients.erase(iter);
						}
					}
				}
			}
		}
	}

	bool IsRun()
	{
		return m_hSocket != INVALID_SOCKET;
	}

	int Close()
	{
		if (m_hSocket != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				closesocket(_clients[i]->sockfd());
				delete _clients[i];
			}

			closesocket(m_hSocket);

			//清除Windows socket环境
			WSACleanup();
#else
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				close(_clients[i]->sockfd());
				delete _clients[i];
			}

			close(m_hSocket);
#endif // _WIN32

			_clients.clear();
		}

		return 0;
	}

	char _szRecv[RECV_BUFF_SIZE] = { 0 };

	int RecvData(ClientSocket* pClient)
	{
		// 5.接受客户端的请求数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, sizeof(_szRecv), 0);
		if (nLen <= 0)
		{
			printf("客户端<Socket=%d>已退出,任务结束\n", (int)pClient->sockfd());
			return -1;
		}

		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);

		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();

			//判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;

				//处理网络消息
				OnNetMsg(pClient->sockfd(), header);

				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);

				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}

		return 0;
	}

	virtual void OnNetMsg(SOCKET sock, DataHeader* header)
	{
		_recvCount++;
		_pNetEvent->OnNetMsg(sock, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("收到客户端<Socket=%d>命令:CMD_LOGIN, 数据长度;%d\n", (int)sock, login->dataLength);
			//printf("用户登录...用户姓名:%s, 密码:%s\n", login->userName, login->passWord);

			//LoginResult ret;
			//SendData(sock, &ret);
		}break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//printf("收到客户端<Socket=%d>命令:CMD_LOGOUT, 数据长度;%d\n", (int)sock, logout->dataLength);
			//printf("用户登出...用户姓名:%s\n", logout->userName);

			//LogoutResult ret;
			//SendData(sock, &ret);
		}break;
		default:
		{
			printf("收到客户端<Socket=%d>未定义消息, 数据长度:%d\n", (int)sock, header->dataLength);

			//DataHeader ret;
			//SendData(sock, &ret);

		}break;
		}
	}

	void addClient(ClientSocket *pClient)
	{
		lock_guard<mutex> lock(_mutex);
		/*_mutex.lock();*/
		_clientsBuff.push_back(pClient);
		/*_mutex.unlock();*/
	}

	void Start()
	{
		_pThread = new thread(&CellServer::OnRun, this);
	}

	size_t GetClinetCount()
	{
		return _clients.size() + _clientsBuff.size();
	}

private:
	SOCKET m_hSocket;
	//正式客户队列
	vector<ClientSocket*> _clients;
	//缓冲客户队列
	vector<ClientSocket*> _clientsBuff; 
	mutex _mutex;
	thread* _pThread;
	INetEvent* _pNetEvent;
public:
	atomic_int _recvCount;
};

class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer()
	{
		m_hSocket = INVALID_SOCKET;
	}
	virtual ~EasyTcpServer()
	{
		Close();
	}

	//初始化Socket
	SOCKET InitSocket()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);		//生成版本号
		WSADATA data;

		//启动Windows socket 2.x环境
		if (WSAStartup(ver, &data) != 0)
		{
			printf("错误,加载winsock.dll失败...错误代码:%d", WSAGetLastError());
			return INVALID_SOCKET;
		}
#endif // _WIN32

		if (INVALID_SOCKET != m_hSocket)
		{
			printf("<socket=%d>关闭旧连接..\n", m_hSocket);
			Close();
		}

		//创建一个socket套接字
		m_hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_hSocket == INVALID_SOCKET)
		{
#ifdef _WIN32
			printf("错误,建立Socket失败...错误代码:%d", WSAGetLastError());
#else
			printf("错误,建立Socket失败...\n");
#endif // _WIN32
		}
		else
		{
			printf("建立<Socket=%d>成功...\n", (int)m_hSocket);
		}

		return m_hSocket;
	}

	//绑定IP和端口号
	int Bind(const char* ip, unsigned short port)
	{
		struct sockaddr_in server_addr = {};
		int addr_len = sizeof(struct sockaddr_in);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);

		if (ip)
		{
			server_addr.sin_addr.s_addr = inet_addr(ip);
		}
		else
		{
			server_addr.sin_addr.s_addr = INADDR_ANY;
		}

		int ret = bind(m_hSocket, (struct sockaddr*)&server_addr, addr_len);
		if (SOCKET_ERROR == ret)
		{
#ifdef _WIN32
			printf("错误,<Socket=%d>绑定用于接受客户端连接的网络端口<%d>失败...错误代码:%d\n", (int)m_hSocket, port, WSAGetLastError());
#else
			printf("错误,<Socket=%d>绑定用于接受客户端连接的网络接口<%d>失败...\n", (int)m_hSocket, port);
#endif // _WIN32
		}
		else
		{
			printf("<Socket=%d>绑定网络端口<%d>成功\n", (int)m_hSocket, port);
		}

		return ret;
	}

	//监听端口号
	int Listen(int n)
	{
		int ret = listen(m_hSocket, n);
		if (SOCKET_ERROR == ret)
		{
#ifdef _WIN32
			printf("错误,<Socket=%d>监听网络端口失败...错误代码:%d\n", (int)m_hSocket, WSAGetLastError());
#else
			printf("错误,<Socket=%d>监听网络端口失败...\n", (int)m_hSocket);
#endif // _WIN32

		}
		else
		{
			printf("<Socket=%d>监听网络端口成功...\n", (int)m_hSocket);
		}

		return ret;
	}

	//接受客户端连接
	SOCKET Accept()
	{
		struct sockaddr_in client_addr = {};
		SOCKET new_socket = INVALID_SOCKET;
		int addr_len = sizeof(struct sockaddr_in);

#ifdef _WIN32
		new_socket = accept(m_hSocket, (struct sockaddr*)&client_addr, &addr_len);
#else
		new_socket = accept(m_hSocket, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);
#endif // _WIN32

		if (new_socket == INVALID_SOCKET)
		{
#ifdef _WIN32
			printf("错误,<Socket=%d>接受到无效客户端Socket...错误代码:%d\n", (int)m_hSocket, WSAGetLastError());
#else		
			printf("错误,<Socket=%d>接受到无效客户端Socket\n", (int)m_hSocket);
#endif // _WIN32
		}
		else
		{
			//NewUserJoin userJoin;
			//BatchSendData(&userJoin);
			//_clients.push_back(new ClientSocket(new_socket));
			//printf("<Socket=%d>新客户端<%d>加入: Socket = %d, IP = %s\n", (int)m_hSocket, (int)_clients.size(), (int)new_socket, inet_ntoa(client_addr.sin_addr));

			addClientToCellServer(new ClientSocket(new_socket));
		}

		return new_socket;
	}

	//添加客户端
	void addClientToCellServer(ClientSocket* pClient)
	{
		_clients.push_back(pClient);
		//查找客户数量最少的CellServer消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers)
		{
			if (pMinServer->GetClinetCount() > pCellServer->GetClinetCount())
			{
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
	}

	//启动
	void Start()
	{
		for (int i = 0; i < _CELLSERVER_THREAD_COUNT; ++i)
		{
			auto ser = new CellServer(m_hSocket);
			_cellServers.push_back(ser);
			ser->setEventObj(this);
			ser->Start();
		}
	}

	//关闭Socket
	int Close()
	{
		if (m_hSocket != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				closesocket(_clients[i]->sockfd());
				delete _clients[i];
			}

			closesocket(m_hSocket);

			//清除Windows socket环境
			WSACleanup();
#else
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				close(_clients[i]->sockfd());
				delete _clients[i];
			}

			close(m_hSocket);
#endif // _WIN32

			_clients.clear();
		}

		return 0;
	}

	//处理网络消息
	bool OnRun()
	{
		if (IsRun())
		{
			time4msg();

			//伯克利 socket
			fd_set fdRead;

			//FD_ZERO:清空一个文件描述符集合
			FD_ZERO(&fdRead);

			//FD_SET:将监听的文件描述符，添加到监听集合中
			FD_SET(m_hSocket, &fdRead);

			/*
			select函数原型:
			int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);

			参数列表:
			nfds:		监听的所有文件描述符的最大描述符 + 1(内核采取轮询的方式)
			readfds:	读文件描述监听集合
			writefds:	写文件描述符集合
			exceptfds:	异常文件描述符集合
			timeout:	大于0:设置监听超时时长;NULL:阻塞监听;0:非阻塞监听

			返回值:
			大于0:所欲监听集合中,满足对应时间的总数
			0:没有满足的
			-1:出错error
			*/

			timeval t = { 0,10 };
			int ret = select(m_hSocket + 1, &fdRead, 0, 0, &t);
			/*printf("select ret = %d count = %d\n", ret, count ++);*/
			if (ret < 0)
			{
				printf("select任务结束\n");
				Close();
				return false;
			}

			//判断描述符（socket）是否在集合中
			if (FD_ISSET(m_hSocket, &fdRead))
			{
				FD_CLR(m_hSocket, &fdRead);
				Accept();
				return true;
			}
			return true;
		}
		return false;
	}

	//是否工作中
	bool IsRun()
	{
		return m_hSocket != INVALID_SOCKET;
	}

	//接收数据,处理粘包,拆分包
	int RecvData(ClientSocket* pClient)
	{
		char _szRecv[RECV_BUFF_SIZE] = { 0 };

		// 5.接受客户端的请求数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, sizeof(_szRecv), 0);
		if (nLen <= 0)
		{
			printf("客户端<Socket=%d>已退出,任务结束\n", (int)pClient->sockfd());
			return -1;
		}

		//将收取到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);

		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader))
		{
			DataHeader* header = (DataHeader*)pClient->msgBuf();

			//判断消息缓冲区的数据长度大于消息长度
			if (pClient->getLastPos() >= header->dataLength)
			{
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;

				//处理网络消息
				//OnNetMsg(pClient->sockfd(), header);

				//将消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);

				//消息缓冲区的数据尾部位置前移
				pClient->setLastPos(nSize);
			}
			else
			{
				//消息缓冲区剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}

	//响应网络消息
	int time4msg()
	{
		auto t = _tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			int recvCount = 0;
			for (auto ser : _cellServers)
			{
				recvCount += ser->_recvCount;
				ser->_recvCount = 0;
			}

			printf("thread<%d>,time<%lf>,socket<%d>,client<%d>,recvCount:<%d>\n", _cellServers.size(), t, m_hSocket, (int)_clients.size(), (int)(recvCount / t));
			_tTime.update();
		}

		return 0;
	}

	//发送指定数据
	int SendData(SOCKET sock, DataHeader* header)
	{
		if (IsRun() && header)
		{
			return send(sock, (const char*)header, header->dataLength, 0);
		}

		return SOCKET_ERROR;
	}

	//群发数据
	int BatchSendData(DataHeader* header)
	{
		if (IsRun() && header)
		{
			for (size_t i = 0; i < _clients.size(); ++i)
			{
				if (SendData(_clients[i]->sockfd(), header) == SOCKET_ERROR)
				{
					printf("<Socket=%d>发送数据失败\n", _clients[i]->sockfd());
				}
			}
		}
		return SOCKET_ERROR;
	}

	virtual void OnLeave(ClientSocket* pClient)
	{
		for (int i = 0; i < _clients.size(); ++i)
		{
			if (_clients[i] == pClient)
			{
				auto iter = _clients.begin() + i;
				if (iter != _clients.end())
				{
					_clients.erase(iter);
				}
			}
		}
	}

	virtual void OnNetMsg(SOCKET sock, DataHeader *header)
	{

	}

private:
	SOCKET m_hSocket;
	vector<ClientSocket*> _clients;
	vector<CellServer*> _cellServers;

	CELLTimestamp _tTime;
};

#endif

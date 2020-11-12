#ifndef __codeSockv2__
#define __codeSockv2__

#include <winsock2.h>
#include <thread>
#include <chrono>

enum{
	PACKET_SIZE=1024,
	
	SINGLE=0,
	MULTI=1,
	
	NONE=0,
	LOOP=1,
	
	WAIT_NO=0,
	WAIT_YES=1,
	
	ALL=-1
};

namespace TCP{
	WSADATA wsa;
	SOCKADDR_IN addr={0};
		
	SOCKET SINGLE_SOCKET,server,CServer;
	bool clientLock=false,serverLock=false;
	int SockOp=0,ToSetting;
	
	template <typename T> class _dpVec{
	private:
		int size,maxsize;
		T *data=NULL;
	public:
		_dpVec() : size(0) , maxsize(1) , data(0) { }
		~_dpVec(){ size=0,maxsize=1; delete[] data; }
		void push(T t){
			if(size+1>=maxsize){
				maxsize *=2;
				T *tmp = new T[maxsize];
				for(int i=0;i<size;i++) tmp[i]=data[i];
				tmp[size++]=t;
				delete[] data;
				data=tmp;
			}
			else data[size++]=t;
		}
		int length()const{ return this->size; }
		T& operator[](const int index){ return data[index]; }
	};

	typedef struct{
		int hNum;
		int mNum;
		char hMessage[PACKET_SIZE];
	}PacketInfo; PacketInfo SendInfo;

	class dpSock{
		public:
			SOCKET client;
			SOCKADDR_IN client_info={0};
			int client_size = sizeof(client_info);
			int number;
			dpSock(){
				client={0};
				client_info={0};
				client_size = sizeof(client_info);
			}
			~dpSock(){
				client={0};
				client_info={0};
				client_size=-1;
				number=-1;
			}
	}; _dpVec<dpSock>s; _dpVec<PacketInfo>pkInfo;
	
	void acceptClients(SOCKET &lss){
		int number=0;
		char dsf[50]={0};
		while(1){
			s.push(dpSock());
			pkInfo.push(PacketInfo());
			s[number].client = accept(lss,(SOCKADDR*)&s[number].client_info,&s[number].client_size);
	        s[number].number = number;
	        itoa(number,dsf,10);
	        send(s[number].client,dsf,strlen(dsf),0);
			number++;
		}
	}
	
	void SEND__tH(int num, const char *buf){
		if(serverLock&&!clientLock){
			if(SockOp==0){
				if(s.length()<num+2) return;
			}
			else if(SockOp==1) while(s.length()<num+2);
			while(1){
				int size = send(s[num].client,buf,strlen(buf),0);
				if(size!=-1) return;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		if(clientLock&&!serverLock){
			while(1){
				SendInfo.hNum=num;
				strcpy(SendInfo.hMessage,buf);
				int size = send(CServer,(char*)&SendInfo,PACKET_SIZE,0);
				if(size!=-1) return;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	}
	
	void SEND(const char *buf){
		if(serverLock&&!clientLock) send(SINGLE_SOCKET,buf,PACKET_SIZE,0);
		else if(clientLock&&!serverLock) send(CServer,buf,PACKET_SIZE,0);
	}
	void SEND(int num, const char *buf){
		if(num<-1) return;
		else{
			if(num==-1){
				if(s.length()-1>0) for(int i=0;i<s.length()-1;i++) std::thread (SEND__tH,i,buf).detach();
			}
			else std::thread (SEND__tH,num,buf).detach();	
		}
	}
	
	int RECEIVE(char *buf){
		if(serverLock&&!clientLock){
			int size = recv(SINGLE_SOCKET,buf,PACKET_SIZE,0);
			return size;
		}
		else if(clientLock&&!serverLock){
			int size = recv(CServer,buf,PACKET_SIZE,0);
			return size;
		}
	}
	int RECEIVE(int num,char *buf){
		if(num<0) return -1;
		if(SockOp==0){
				if(s.length()<num+2) return -2;
			}
			else if(SockOp==1) while(s.length()<num+2);
		if(serverLock&&!clientLock){
			while(1){
				int size = recv(s[num].client,buf,PACKET_SIZE,0);
				if(size!=-1) return size;
			}
		}
		return -1;
	}
	
	int RESPONSE(int num){
		if(num<0) return -1;
		char buf[PACKET_SIZE]={0},dst[10];
		if(serverLock&&!clientLock){
			while(pkInfo.length()<num+2);
			
			recv(s[num].client,buf,PACKET_SIZE,0);
			memcpy((char*)&pkInfo[num],buf,PACKET_SIZE);
			
			while(s.length()<num+2);
			while(s.length()<pkInfo[num].hNum+2);
			
			while(1){
				int size = send(s[pkInfo[num].hNum].client,pkInfo[num].hMessage,PACKET_SIZE,0);
				if(size!=-1) return size;
				Sleep(1000);
			}
		}
		return -1;
	}
	
	void WAIT(){ if(clientLock) while(!WSAGetLastError()); }
	int CLIENTS(){ return s.length()-1; }
	void OPTIONS(int Options){
		if(Options==0) SockOp=0;
		else if(Options==1) SockOp=1;
	}
	int GETNUMBER(){
		if(clientLock) return SendInfo.mNum;
		else return -1;
	}
	void CLOSE(){
		if(clientLock&&!serverLock) closesocket(CServer);
		else if((serverLock&&!clientLock)&&!ToSetting){
			closesocket(SINGLE_SOCKET);
			closesocket(server);
		}
		else if((serverLock&&!clientLock)&&ToSetting){
			for(int i=0;s.length()-1;i++) closesocket(s[i].client);
			closesocket(server);
		}
		WSACleanup();
	}
	
	const char* SERVER(int Port,int Options){
		if(clientLock) return "ClientSocket Is Using Already";
		else serverLock=true;
		if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) return "WSA Error";
		
		ToSetting=Options;
		
		server = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(server==INVALID_SOCKET) return "ServerSocket Error";
		
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(Port);
		addr.sin_family = AF_INET;
	
		if(bind(server,(SOCKADDR*)&addr,sizeof(addr))==SOCKET_ERROR) return "Bind Error";
		if(listen(server,SOMAXCONN)==SOCKET_ERROR) return "Listen Error";
		
		if(Options==0){
			SOCKADDR_IN client_sock={0};
			int client_size = sizeof(client_sock);
			SINGLE_SOCKET = accept(server,(SOCKADDR*)&client_sock,&client_size);
			if(SINGLE_SOCKET==INVALID_SOCKET) return "SingleClient Cannot Accept";
		}
		else if(Options==1) std::thread (acceptClients,std::ref(server)).detach();
		return "SUCCESS";
	}
	
	const char* CLIENT(const char *Ip,int Port,int Options,int ConnectionOptions){
		if(serverLock) return "ServerSocket Is Using Already";
		else clientLock=true;
		if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) return "WSA Error";
	
		ToSetting=Options;
	
		CServer = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(CServer==INVALID_SOCKET) return "ClientSocket Error";
		
		addr.sin_addr.s_addr = inet_addr(Ip);
		addr.sin_port = htons(Port);
		addr.sin_family = AF_INET;
		
		if(!ConnectionOptions){
			if(connect(CServer,(SOCKADDR*)&addr,sizeof(addr))==SOCKET_ERROR) return "Connect Error";
		}
		else if(ConnectionOptions) while(connect(CServer,(SOCKADDR*)&addr,sizeof(addr)));
		if(Options==0){
			//SINGLE
		}
		else if(Options==1) {
			char mNumc[100]={0};
			recv(CServer,mNumc,100,0);
			SendInfo.mNum=atoi(mNumc);
		}
		return "SUCCESS";
	}
}

#endif

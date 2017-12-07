#pragma once

//#include "core.h"

#define LOCALHOST "sitev.ru"

namespace webserver {

enum ParamType {ptGET, ptPOST, ptCOOKIE};

class ParamItem {
public:
	bool isObject = false;
	string value;
	Memory memory;
};

class ParamMap : public Object {
public:
	map<string, ParamItem*> pars;
	ParamMap();
	virtual ~ParamMap();

	virtual void add(String name, String value);
	virtual void insert(String name, String value);
	virtual void clear();
	virtual int getCount();
	virtual bool parse(String s);
	virtual String getValue(String name);
	virtual String getName(int index);
	virtual String getValue(int index);

	virtual void add(string name, string value);
	virtual void insert(string name, string value);
	virtual string getValue_s(string name);
	virtual string getName_s(int index);
	virtual string getValue_s(int index);

	virtual void add(string name, Memory &memory);
	virtual void getObject(string name, Memory &memory);
	virtual bool isObject(string name);
};


class RequestHeader : public ParamList {
public:
	ParamMap GET;
	ParamMap POST;
	ParamMap COOKIE;
	bool isFileFlag;
	string fileExt;
	RequestHeader();
	virtual bool parse(Memory &request);
	virtual void parsePOSTParams(Memory &memory);
private:
	virtual bool parseParams(String sParams, ParamType pt);
	virtual void parseParamsMultipart(Memory &memory, string boundary);

	virtual bool isFile(string s, string &fileExt);
	virtual string urlDecode(string s);
	virtual int find(Memory &request, char a);
	virtual int find(Memory &request, string s);
	virtual string substr(Memory &request, int pos, int count);
	virtual char getChar(Memory &request, int pos);
	virtual string decodeHtmlTags(string s, string tag, string dst);
public:
	virtual string htmlEntities(string s);
	virtual string htmlEntitiesDecode(string s);
};

class HttpRequest {
public:
	Memory memory;
	RequestHeader header;
	HttpRequest() {}
	virtual ~HttpRequest() {}

	virtual String GET(String name);
	virtual String POST(String name);
	virtual String COOKIE(String name);

	virtual void parse();
};

class HttpResponse {
public:
	Memory memory;
	HttpResponse() {
	}

};

typedef std::function< void(HttpRequest &request, HttpResponse &response) > StepEvent;

class WebServer;

class AbstractWebServerHandler : public Object {
protected:
	WebServer *webServer = NULL;
public:
	AbstractWebServerHandler(WebServer *webServer) {
		this->webServer = webServer;
	}
	virtual void threadStep(Socket *socket) = 0;
};

class WebServerHandler : public AbstractWebServerHandler {
protected:
	HttpRequest request;
	HttpResponse response;
public:
	WebServerHandler(WebServer *webServer);
protected:
	virtual void recvMemory(Socket *socket, Memory &memory);
	virtual bool check2CRLF(Memory &memory);
public:
	virtual void threadStep(Socket *socket);
	virtual void internalStep(HttpRequest &request, HttpResponse &response);
	virtual void step(HttpRequest &request, HttpResponse &response);
	virtual bool isSiteExist(string host) { return true; }
	virtual bool isPageExist(string host) { return true; }
};

class WebServer : public Application {
protected:
	mutex g_mutex1;
	WebServerHandler *handler;
public:

	int socketPort;
	ServerSocket *ss;
//	int epoll_fd;
//	struct epoll_event event;
//	struct epoll_event *events;

	StepEvent onStep;

	WebServer(int port = 80);
	virtual ~WebServer() {}
	virtual void init();
	virtual void step();
	virtual void run();
	virtual void runLight();
	virtual void setHandler(WebServerHandler *handler);
	virtual void threadStep(Socket *socket);
	virtual void threadFunction(Socket *socket);
};

}

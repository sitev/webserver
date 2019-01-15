#define _CRT_SECURE_NO_WARNINGS

#include "core.h"
using namespace core;
#include "network.h"
using namespace network;
#include "logger.h"
using namespace logger;
#include "application.h"
using namespace app;

//#include <sys/stat.h>

#include "webserver.h"

namespace webserver {

	//--------------------------------------------------------------------------------------------------
	//----------          ParamMap          -------------------------------------------------------
	//--------------------------------------------------------------------------------------------------

	ParamMap::ParamMap() {
	}

	ParamMap::~ParamMap() {
	}

	void ParamMap::add(String name, String value) {
		add(name.to_string(), value.to_string());
	}

	void ParamMap::insert(String name, String value) {
		insert(name.to_string(), value.to_string());
	}

	void ParamMap::clear() {
		pars.clear();
	}
	int ParamMap::getCount() {
		return pars.size();
	}
	bool ParamMap::parse(String s) {
		return true;
	}

	String ParamMap::getValue(String name) {
		return getValue_s(name.to_string());
	}

	String ParamMap::getName(int index) {
		auto iter = pars.begin();
		for (int i = 0; i < index; i++) iter++;
		return iter->first;
	}

	String ParamMap::getValue(int index) {
		return getValue_s(index);
	}

	void ParamMap::add(string name, string value) {
		ParamItem *pi = new ParamItem();
		pi->value = value;
		pars.insert(std::pair<string, ParamItem*>(name, pi));
	}
	void ParamMap::insert(string name, string value) {
		ParamItem *pi = new ParamItem();
		pi->value = value;
		pars.insert(std::pair<string, ParamItem*>(name, pi));
	}
	string ParamMap::getValue_s(string name) {
		ParamItem *pi = pars[name];
		if (!pi) return "";
		return pi->value;
	}
	string ParamMap::getName_s(int index) {
		auto iter = pars.begin();
		for (int i = 0; i < index; i++) iter++;
		return iter->first;
	}
	string ParamMap::getValue_s(int index) {
		auto iter = pars.begin();
		for (int i = 0; i < index; i++) iter++;
		ParamItem *pi = iter->second;
		if (!pi) return "";
		return pi->value;
	}

	void ParamMap::add(string name, Memory &memory) {
		ParamItem *pi = new ParamItem();
		pi->isObject = true;
		pi->memory.write(memory.data, memory.getSize());
		pars.insert(std::pair<string, ParamItem*>(name, pi));
	}

	void ParamMap::getObject(string name, Memory &memory) {
		ParamItem *pi = pars[name];
		if (!pi) return;
		memory = pars[name]->memory;
	}

	void ParamMap::getObject(int index, Memory &memory) {
		auto iter = pars.begin();
		for (int i = 0; i < index; i++) iter++;
		ParamItem *pi = iter->second;
		if (!pi) return;
		memory = pi->memory;
	}

	bool ParamMap::isObject(string name) {
		ParamItem *pi = pars[name];
		if (!pi) return false;
		return pi->isObject;
	}

	bool ParamMap::isObject(int index) {
		auto iter = pars.begin();
		for (int i = 0; i < index; i++) iter++;
		ParamItem *pi = iter->second;
		if (!pi) return false;
		return pi->isObject;
	}

	//--------------------------------------------------------------------------------------------------
//----------          class RequestHeader          -------------------------------------------------
//--------------------------------------------------------------------------------------------------

RequestHeader::RequestHeader() {

}

bool RequestHeader::parse(Memory &request) {
	isFileFlag = false;
	clear();

	LOGGER_TRACE("request parse");
	int pos1 = find(request, ' ');
	if (pos1 <= 0) return false;

	string method = substr(request, 0, pos1);
	add("Method", method);

	LOGGER_TRACE("method = " + method);

	pos1++;

	int savePos = request.getPos();
	string httpstr = " HTTP/1.1\r\n";
	int pos2 = find(request, httpstr);
	LOGGER_SCREEN("pos1 = " + (String)pos1 + " pos2 = " + (String)pos2);
	if (pos2 <= pos1) {
		LOGGER_SCREEN("pos2 <= pos1");
		request.setPos(savePos);
		httpstr = " HTTP/1.0\r\n";
		pos2 = find(request, httpstr);
		LOGGER_SCREEN("pos1 = " + (String)pos1 + " pos2 = " + (String)pos2);
		if (pos2 <= pos1) return false;
		add("Version", (string)"1.0");
		LOGGER_OUT("Version", "Version = 1.0");

	}
	else {
		add("Version", (string)"1.1");
		LOGGER_OUT("Version", "Version = 1.1");
	}

	char ch = getChar(request, pos1);
	if (ch != '/') return false;
	ch = getChar(request, pos1 + 1);
	if (ch == '?') pos1++;

	string sParams = substr(request, pos1 + 1, pos2 - pos1 - 1);
	//string sDecode = decodeCp1251(sParams);
	string sDecode = sParams;
	add("Params", sDecode);
	LOGGER_OUT("Params", "Params = " + sDecode);


	if (isFile(sParams, fileExt)) {
		isFileFlag = true;
	}
	else 
		parseParams(sParams, ptGET);


	while (true) {
		pos1 = request.getPos();
		pos2 = find(request, ":");
		request.setPos(pos1);
		int pos3 = find(request, "\r\n");

		if (pos2 < 0) {
			request.setPos(pos1);
			int pos3 = find(request, "\r\n");
			break;
		}
		if (pos3 <= 0) 
			break;
		if (pos2 > pos3) 
			break;

		string name = substr(request, pos1, pos2 - pos1);
		string value = substr(request, pos2 + 2, pos3 - pos2 - 2);
		add(name, value);


		if (name == "Cookie") {
			parseParams(value, ptCOOKIE);
		}
	}
	/******
	if (method == "POST") {
		pos1 = pos1 + 2;
		sParams = substr(request, pos1, request.getSize() - pos1);
//		string sDecode = decodeCp1251(sParams);
		string sDecode = sParams;
		parseParams(sDecode, ptPOST);
	}
	*/

	pos1 = request.getPos();
	return true;
}

void RequestHeader::parsePOSTParams(Memory &memory) {
	int pos = memory.getPos();

	int count = this->getCount();
	string contype = this->getValue_s("Content-Type");
	int pos1 = contype.find("; ");
	string s1 = contype.substr(0, pos1);
	string s2 = contype.substr(pos1 + 2);
	if (s1 == "multipart/form-data") {
		int pos2 = s2.find("=");
		string boundary = s2.substr(pos2 + 1);
		parseParamsMultipart(memory, boundary);
		return;
	}

	string sParams = substr(memory, pos, memory.getSize() - pos);
	string sDecode = sParams;

	parseParams(sDecode, ptPOST);
}



bool RequestHeader::parseParams(String sParams, ParamType pt) {
	ParamMap *params;
	if (pt == ptGET) params = &GET;
	else if (pt == ptPOST) 
		params = &POST;
	else 
		params = &COOKIE;

	params->clear();
	if (sParams == "") return true;

	string s = sParams.to_string();

	int pos = 0;
	string tmp = s.substr(pos, 1);
	if (tmp == "/") {
		pos++;
	}

	tmp = s.substr(pos, 1);
	if (tmp == "?") {
		pos++;
	}

	string path = s.substr(pos);

	//string path = "param1/param2/name1=value1&name2=value2";

	pos = 0;
	int posEnd = path.length();
	int mode = 1;
	int index = 1;
	string name = "", value = "";

	while (true) {
		if (pos >= posEnd || path[pos] == '/' || path[pos] == '&' || path[pos] == '?' || path[pos] == ';') {
			if (mode == 1) {
				if (name != "") {
					name = urlDecode(name);
					name = htmlEntities(name);
					value = name;
					name = "p" + to_string(index);
					index++;
					params->insert(name, value);
					name = "";
					value = "";
				}
			}
			else {
				name = urlDecode(name);
				value = urlDecode(value);
				name = htmlEntities(name);
				value = htmlEntities(value);
				params->insert(name, value);
				name = "";
				value = "";
				mode = 1;
			}
			if (pos >= posEnd) break;
		}
		else if (path[pos] == '=') {
			mode = 2;
		}
		else if (path[pos] == ' ') {
			//null
		}
		else {
			if (mode == 1) name += path[pos]; else value += path[pos];
		}
		pos++;
	}

	return true;
}

void RequestHeader::parseParamsMultipart(Memory &memory, string boundary) {
	ParamMap *params = &POST;
	int pos = memory.getPos();
	int size = memory.getSize();

	//File *f = new File((String)"c:\\temp\\fff.fff", "wb");
	//f->write(memory.data, size);
	//delete f;

	int len = boundary.length();
	int pos1 = find(memory, boundary);
	while (true) {
//		int pos1 = find(memory, boundary + "--");
//		if (pos1 <= 0) return;
		char ch;
		memory.readChar(ch);
		if (ch == '-') {
			memory.readChar(ch);
			if (ch == '-') return;
		}
		int pos2 = find(memory, "name=\"");
		int pos3 = find(memory, "\"");
		string name = substr(memory, pos2 + 6, pos3 - pos2 - 6);

		memory.readChar(ch);
		if (ch == ';') {
			int pos4 = find(memory, "\r\n\r\n");
			int pos5 = find(memory, "\r\n--" + boundary);
			Memory m;
			char *p = memory.data;
			p += pos4 + 4;
			m.write(p, pos5 - pos4 - 4);
			params->add(name, m);
		}
		else {
			int pos4 = find(memory, "\n\r\n");
			int pos5 = find(memory, "\r\n--" + boundary);

			string value = substr(memory, pos4 + 3, pos5 - pos4 - 3);
			params->add(name, value);
		}
	}
}

bool RequestHeader::isFile(string s, string &fileExt) {
	int pos = s.find_last_of('.');
	if (pos < 0) return false;
	fileExt = s.substr(pos + 1);
	return true;
}

string RequestHeader::urlDecode(string src) {
	string ret;
	int len = src.length();
	for (int i = 0; i < len; i++) {
		if (int(src[i]) == 37) {
			int ichar;
			sscanf(src.substr(i + 1, 2).c_str(), "%x", &ichar);
			char ch = static_cast<char>(ichar);
			ret += ch;
			i = i + 2;
		}
		else if (int(src[i]) == 43) {
			ret += ' ';
		}
		else {
			ret += src[i];
		}
	}
	return ret;
}

int RequestHeader::find(Memory &request, char a) {
	while (!request.eof()) {
		char ch;
		request.read(&ch, 1);
		if (ch == a) return request.getPos() - 1;
	}

	return -1;
}

int RequestHeader::find(Memory &request, string s) {
	int index;
	int len = s.length();
	if (len <= 0) return -1;
	bool flag = false;
	int save_pos;
	while (!request.eof()) {
		if (!flag) {
			flag = true;
			if (find(request, s[0]) < 0) flag = false;
			index = 1;
			if (flag) save_pos = request.getPos();
		}
		else {
			char ch;
			request.read(&ch, 1);
			char a = s[index];
			index++;
			if (ch != a) {
				flag = false;
				index = 0;
				request.setPos(save_pos);
			}
		}
		if (len <= index) {
			if (flag) return request.getPos() - len;
		}
	}
	return -1;
}

string RequestHeader::substr(Memory &request, int pos, int count) {
	char *s = (char*)malloc(count + 1);
	memcpy(s, (char*)request.data + pos, count);
	s[count] = 0;
	string ret = s;
	free(s);
	return ret;
}

char RequestHeader::getChar(Memory &request, int pos) {
	return ((char*)request.data)[pos];
}

string RequestHeader::htmlEntities(string s) {
	string r = "";
	int len = s.length();
	for (int i = 0; i < len; i++) {
		char ch = s[i];
		switch (ch) {
		case '&': {
			r = r + "&amp";
			break;
		}
		case '<': {
			r = r + "&lt";
			break;
		}
		case '>': {
			r = r + "&gt";
			break;
		}
		case '"': {
			r = r + "&guot";
			break;
		}
		case '\'': {
			r = r + "&apos";
			break;
		}
		default:
			r = r + ch;
		}
	}
	return r;
}

string RequestHeader::decodeHtmlTags(string s, string tag, string dst) {
	while (true) {
		int pos = s.find(tag);
		if (pos < 0) break;
		s.replace(pos, tag.size(), dst);
	}
	return s;
}

string RequestHeader::htmlEntitiesDecode(string s) {
	s = decodeHtmlTags(s, "&lt!--", "<!--");
	s = decodeHtmlTags(s, "&ltp&gt", "<p>");
	s = decodeHtmlTags(s, "&ltb", "<b");
	s = decodeHtmlTags(s, "&lta", "<a");
	s = decodeHtmlTags(s, "&lti", "<i");
	s = decodeHtmlTags(s, "&ltblockquote&gt", "<blockquote>");
	s = decodeHtmlTags(s, "&ltstrong&gt", "<strong>");
	s = decodeHtmlTags(s, "&ltspan", "<span");
	s = decodeHtmlTags(s, "&ltli&gt", "<li>");
	s = decodeHtmlTags(s, "&lte", "<e");
	s = decodeHtmlTags(s, "&lth", "<h");
	s = decodeHtmlTags(s, "&ltu", "<u");
	s = decodeHtmlTags(s, "&lto", "<o");
	s = decodeHtmlTags(s, "&ltfont", "<font");
	s = decodeHtmlTags(s, "&ltdiv", "<div");
	s = decodeHtmlTags(s, "&ltt", "<t");
	s = decodeHtmlTags(s, "&ltc", "<c");

	s = decodeHtmlTags(s, "&ltbr", "<br");

	s = decodeHtmlTags(s, "-->&gt", "-->");
	s = decodeHtmlTags(s, "&lt/p&gt", "</p>");
	s = decodeHtmlTags(s, "&lt/b", "</b");
	s = decodeHtmlTags(s, "&lt/a", "</a");
	s = decodeHtmlTags(s, "&lt/i", "</i");
	s = decodeHtmlTags(s, "&lt/blockquote&gt", "</blockquote>");
	s = decodeHtmlTags(s, "&lt/strong&gt", "</strong>");
	s = decodeHtmlTags(s, "&lt/span&gt", "</span>");
	s = decodeHtmlTags(s, "&lt/ul&gt", "</ul>");
	s = decodeHtmlTags(s, "&lt/li&gt", "</li>");
	s = decodeHtmlTags(s, "&lt/e", "</e");
	s = decodeHtmlTags(s, "&lt/h", "</h");
	s = decodeHtmlTags(s, "&lt/u", "</u");
	s = decodeHtmlTags(s, "&lt/o", "</o");
	s = decodeHtmlTags(s, "&lt/font", "</font");
	s = decodeHtmlTags(s, "&lt/div", "</div");
	s = decodeHtmlTags(s, "&lt/t", "</t");
	s = decodeHtmlTags(s, "&lt/c", "</c");

	s = decodeHtmlTags(s, "&guot", "\"");
	s = decodeHtmlTags(s, "&gt", ">");
	s = decodeHtmlTags(s, "&ampnbsp;", "&nbsp;");

	return s;
}



//--------------------------------------------------------------------------------------------------
//----------          class HttpRequest          ---------------------------------------------------
//--------------------------------------------------------------------------------------------------

String HttpRequest::GET(String name) {
	return header.GET.getValue(name);
}

String HttpRequest::POST(String name) {
	return header.POST.getValue(name);
}

String HttpRequest::COOKIE(String name) {
	return header.COOKIE.getValue(name);
}

void HttpRequest::parse() {
	LOGGER_TRACE("Start parse");

	memory.setPos(0);
	header.parse(memory);

	LOGGER_TRACE("Finish parse");
}



//--------------------------------------------------------------------------------------------------
//----------          class WebServerHandler          ----------------------------------------------
//--------------------------------------------------------------------------------------------------
WebServerHandler::WebServerHandler(WebServer *webServer) : AbstractWebServerHandler(webServer) {
}

void WebServerHandler::recvMemory(Socket *socket, Memory &memory) {
	while (true) {
		try {
			int len = socket->recv(memory);
			if (len <= 0) break;
		}
		catch (...) {
			int len = socket->recv(memory);
			if (len <= 0) break;
		}
	}
}

bool WebServerHandler::check2CRLF(Memory &memory) {
	int pos = memory.getPos();
	int posEnd = memory.getSize();
	int step = 0;
	while (true) {
		if (pos >= posEnd) 
			return false;

		char u = *(((char*)memory.data) + pos);
		//printf("%c", u);
		if (step == 0 && u == '\015')
			step++;
		else if (step == 1 && u == '\012')
			step++;
		else if (step == 2 && u == '\015')
			step++;
		else if (step == 3 && u == '\012') {
			pos++;
			return true;
		}
		pos++;
	}
}

void WebServerHandler::threadStep(Socket *socket) {
	try {
		LOGGER_OUT("MUTEX", "application->g_mutex.lock(); {");
		
		application->g_mutex.lock();
		LOGGER_OUT("MUTEX", "application->g_mutex.lock(); }");
		request.memory.setPos(0);
		request.memory.setSize(0);
		response.memory.setPos(0);
		response.memory.setSize(0);

		application->g_mutex.unlock();

		while (true) {
			while (true) {
				application->g_mutex.lock();
				recvMemory(socket, request.memory);
				int size = request.memory.getSize();
				int pos = request.memory.getPos();
				bool flag = check2CRLF(request.memory);
				pos = request.memory.getPos();
				application->g_mutex.unlock();
				if (flag) break;
				usleep(1000);
			}

			application->g_mutex.lock();

			request.parse();
			int pos = request.memory.getPos();
			String method = request.header.getValue("Method");
			int conLen = request.header.getValue("Content-Length").toInt();
			application->g_mutex.unlock();

			if (conLen > 0) {
				while (true) {
					application->g_mutex.lock();
					recvMemory(socket, request.memory);
					int size = request.memory.getSize();
					int delta = size - pos;
					application->g_mutex.unlock();
					if (delta >= conLen) break;
					usleep(1000);
				}
				application->g_mutex.lock();
				if (method == "POST" || method == "PUT")
					request.header.parsePOSTParams(request.memory);
				application->g_mutex.unlock();
			}

			LOGGER_OUT("MUTEX", "application->g_mutex.unlock();");

			internalStep(request, response);

			LOGGER_OUT("MUTEX", "application->g_mutex.lock(); {");
			application->g_mutex.lock();
			LOGGER_OUT("MUTEX", "application->g_mutex.lock(); }");
			if (socket->sendAll(response.memory)) LOGGER_TRACE("sendAll OK!"); else LOGGER_TRACE("sendAll error ...");
			application->g_mutex.unlock();

			break;
		}

		socket->close();
		delete socket;
		//application->g_mutex.unlock();
	}
	catch(...) {
		LOGGER_ERROR("Error in threadStep try catch");
	}
}

void WebServerHandler::internalStep(HttpRequest &request, HttpResponse &response) {
	string host = request.header.getValue("Host").to_string();
	if (host == "127.0.0.1:8080") host = LOCALHOST;

	if (!request.header.isFileFlag)
	{
		bool flagSiteExist = isSiteExist(host);
		bool flagPageExist = isPageExist(host);
		if (!flagSiteExist || !flagPageExist) {
			string fn;
			if (!flagSiteExist) fn = "/var/www/common/404site_tpl.html";
			else fn = "/var/www/common/404page_tpl.html";
			File *f = new File(fn, "rb");
			bool flag = f->isOpen();
			if (flag) {
				string version = request.header.getValue_s("Version");
				LOGGER_SCREEN("version = " + version);
				string s = "HTTP/" + version + " 404 Not Found\r\nContent-Type: text/html";
				int sz = f->getSize();
				s = s + "\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5, max=100\r\nContent-Length: " + to_string(sz) + "\r\n\r\n";
				response.memory.write((void*)(s.c_str()), s.length());

				Memory mem;
				mem.setSize(sz);
				f->read(mem.data, sz);
				response.memory.write(mem.data, sz);
			}
			delete f;
		}
		else {
			step(request, response);
		}
	}
	else {
		string fn1 = request.header.getValue_s("Params");
		if (fn1 == "") {
			fn1 = "index_tpl.html";
			request.header.fileExt = "html";
		}
		string fn = "/var/www/" + host + "/" + fn1;
		if (!fstream(fn.c_str()).good()) {
			fn = "/var/www/common/" + fn1;
		}

		LOGGER_SCREEN("filename = " + fn);
		File *f = new File(fn, "rb");
		bool flag = f->isOpen();
		if (flag) {
			string version = request.header.getValue_s("Version");
			LOGGER_SCREEN("version = " + version);
			string s = "HTTP/" + version + " 200 OK\r\nContent-Type: ";
			if (request.header.fileExt == "html") s = s + "text/html";
			else if (request.header.fileExt == "ico") s = s + "image/ico";
			else if (request.header.fileExt == "png") s = s + "image/png";
			else if (request.header.fileExt == "jpg") s = s + "image/jpeg";
			else if (request.header.fileExt == "js") s = s + "text/javascript";
			else if (request.header.fileExt == "css") s = s + "text/css";
			else if (request.header.fileExt == "gif") s = s + "image/gif";
			else if (request.header.fileExt == "apk") s = s + "application/vnd.android.package-archive";
			else if (request.header.fileExt == "jar") s = s + "application/java-archive";
			else if (request.header.fileExt == "jad") s = s + "text/vnd.sun.j2me.app-descriptor";
			else s = s + "application/octet-stream";

			int sz = f->getSize();
			s = s + "\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5, max=100\r\nContent-Length: " + to_string(sz) + "\r\n\r\n";
			//s = s + "\r\nConnection: keep-alive\r\nKeep-Alive: timeout=5, max=100\r\nSet-Cookie: name=newvalue\r\nContent-Length: " + to_string(sz) + "\r\n\r\n";

			LOGGER_OUT("HTTP", s);

			response.memory.write((void*)(s.c_str()), s.length());

			Memory mem;
			mem.setSize(sz);
			f->read(mem.data, sz);
			response.memory.write(mem.data, sz);
		}
		delete f;
	}
}

void WebServerHandler::step(HttpRequest &request, HttpResponse &response) {
	if (webServer != NULL) {
		if (webServer->onStep != NULL) {
			webServer->onStep(request, response);
			//string s = "hello123";
			//s = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(s.length()) + "\r\n\r\n" + s + "\r\n";
			//response.memory.write((void*)(s.c_str()), s.length());
			return;
		}
	}
	int count = request.header.getCount();

	string s = "";
	for (int i = 0; i < count; i++) {
		string name = request.header.getName_s(i);
		string value = request.header.getValue_s(i);
		s = s + name + " = " + value + "\r\n";
	}

	count = request.header.GET.getCount();
	s = s + to_string(count) + "\r\n";

	for (int i = 0; i < count; i++) {
		string name = request.header.GET.getName(i).to_string();
		string value = request.header.GET.getValue(i).to_string();
		s = s + name + " = " + value + "\r\n";
	}

	s = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(s.length()) + "\r\n\r\n" + s + "\r\n";
	response.memory.write((void*)(s.c_str()), s.length());
}

//--------------------------------------------------------------------------------------------------
//----------          class WebServer          -----------------------------------------------------
//--------------------------------------------------------------------------------------------------

WebServer::WebServer(int port) {
	ss = new ServerSocket();
	socketPort = port;
	LOGGER_SCREEN("create WebServer, port = " + (String)port);
}

void WebServer::threadFunction(Socket *socket) {
	//g_mutex1.lock();
	WebServerHandler *handler = new WebServerHandler(this);
//	cout << "new " << handler << endl;
	//g_mutex1.unlock();

	handler->threadStep(socket);

	//g_mutex1.lock();
//	cout << "del " << handler << endl;
	delete handler;
	//g_mutex1.unlock();
}

void WebServer::init() {
	isRunning = true;
	bool flag = ss->create(AF_INET, SOCK_STREAM, 0);
	if (!flag) {
		exit(1);
	}
	if (!ss->bind(socketPort)) {
		exit(2);
	}

	ss->setNonBlocking(true);
	ss->listen();

	LOGGER_SCREEN("webserver init");

}

void WebServer::step() {
	if (isRunning) {
		ss->accept();

		int index = 0;
		int count = ss->lstSocket.getCount();

		for (int i = 0; i < count; i++) {
			Socket *socket = (Socket*)ss->lstSocket.getItem(index);
			index++;

			int size = socket->getCurSize();

			if (size > 0) {
				index--;
				ss->lstSocket.del(index);

				///LOGGER_DEBUG("----- i = " + (String)i + " size = " + (String)size);

				//std::thread *thr = new std::thread(&WebServer::threadStep, this, socket);
				std::thread *thr = new std::thread(&WebServer::threadFunction, this, socket);
				thr->detach();
				//lstThread.push_back(thr);
			}
		}
		usleep(1000);
	}
}

void WebServer::run() {
	try {
		isRunning = true;
		bool flag = ss->create(AF_INET, SOCK_STREAM, 0);
		if (!flag)
		{
			exit(1);
		}
		if (!ss->bind(socketPort))
		{
			exit(2);
		}

		ss->setNonBlocking(true);
		ss->listen();

		int mycnt = 0;
		while (isRunning)
		{
			ss->accept();

			mycnt++;

			int index = 0;
			int count = ss->lstSocket.getCount();

			if (mycnt % 1000 == 0) {
				String s = "Accept. Count = " + (String)count;
				LOGGER_DEBUG(s);
			}

			for (int i = 0; i < count; i++) {
				Socket *socket = (Socket*)ss->lstSocket.getItem(index);
				index++;

				int size = socket->getCurSize();
				//				LOGGER_OUT("SIZE", "Size of Socket buffer = " + (String)size);

				if (size > 0) {
					index--;
					ss->lstSocket.del(index);

					LOGGER_DEBUG("----- i = " + (String)i + " size = " + (String)size);

					//std::thread *thr = new std::thread(&WebServer::threadStep, this, socket);
					std::thread *thr = new std::thread(&WebServer::threadFunction, this, socket);
					thr->detach();
					//lstThread.push_back(thr);
				}
			}
			usleep(1000);
		}
	}
	catch (...) {
		printf("Error in run() try catch(...)\n");
	}
}

void WebServer::runLight() {
	try {
		isRunning = true;
		bool flag = ss->create(AF_INET, SOCK_STREAM, 0);
		if (!flag) exit(1);
		if (!ss->bind(socketPort)) exit(2);

		ss->setNonBlocking(true);
		ss->listen();

		while (isRunning) {
			Socket *socket = ss->acceptLight();
			if (socket) {
				for (int i = 0; i < 1000000; i++) {
					int size = socket->getCurSize();
					if (size > 0) {
						std::thread *thr = new std::thread(&WebServer::threadFunction, this, socket);
						thr->detach();
						break;
					}
				}
			}
			//usleep(100);
		}
	}
	catch (...) {
		printf("Error in run() try catch(...)\n");
	}
}


void WebServer::setHandler(WebServerHandler *handler) {
	this->handler = handler;
}

void WebServer::threadStep(Socket *socket) {
	if (handler) handler->threadStep(socket);
}

}

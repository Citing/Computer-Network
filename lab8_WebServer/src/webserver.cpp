#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <regex>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int max_buffer_size = 2048;
const std::string rootPath = "../root";

void connection_handle(int cfd);
std::string replyRequest(const std::string& request);
std::string get(const std::string& path, const std::string& http);
std::string post(const std::string& path, const std::string& http, const std::string& data);
std::string makePacket(const std::string& fileData = "", const int& code = 200);

class webServer
{
private:
    int sockfd;
    sockaddr_in sin;
    const int connection_max = 16;

    static void* thread_handle(void* cfd)
    {
        connection_handle(*(int*)cfd);
        return nullptr;
    }

public:
    webServer();
    ~webServer();
    void run();
};

int main()
{
    webServer server;
    server.run();
}

webServer::webServer()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(4848); // magic number: my student ID
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sockfd, (sockaddr*)&sin, sizeof(sin));
    listen(sockfd, connection_max);
}

webServer::~webServer()
{
    close(sockfd);
}

void webServer::run()
{
    std::cout<<"Listening\n";
    while (true)
    {
        sockaddr_in client;
        unsigned int clientAddrLength = sizeof(client);
        int connection_fd = accept(sockfd, (sockaddr*)&client, (socklen_t*)&clientAddrLength);
        std::cout<<inet_ntoa(client.sin_addr)<<":"<<ntohs(client.sin_port)<<" connected.\n";
        pthread_t connection_thread;
        pthread_create(&connection_thread, nullptr, thread_handle, &connection_fd);
    }
}

void connection_handle(int cfd)
{
    char buffer[max_buffer_size] = {0};
    std::string request;
	while (true)
    {
		int realSize = recv(cfd, buffer, max_buffer_size, 0);
		request.append(buffer, buffer + realSize);
        if (buffer[0] == 'P')
        {
            memset(buffer, 0, max_buffer_size);
            realSize = recv(cfd, buffer, max_buffer_size, 0);
		    request.append(buffer, buffer + realSize);
        }
        if (realSize < max_buffer_size)
        {
            break;
        }
	}
    std::string reply = replyRequest(request);
	send(cfd, reply.c_str(), reply.size(), 0);
	close(cfd);
}

std::string replyRequest(const std::string& request)
{
    std::string reply;
    std::regex whitespace("\\s+");
    std::vector<std::string> words(std::sregex_token_iterator(request.begin(), request.end(), whitespace, -1),
                                        std::sregex_token_iterator());
    for (auto it: words)
    {
        std::cout<<it<<' ';
    }
    std::cout<<'\n';

	std::string type = words[0];
	std::string path = words[1];
	std::string http = words[2];
    if (type == "GET")
    {
		reply = get(path, http);
	}
	else if (type == "POST")
    {
		reply = post(path, http, request.substr(request.find("\r\n\r\n", 0) + 4));
	}
    return reply;
}

std::string get(const std::string& path, const std::string& http)
{
    std::regex jpgPattern("\\.(jpg|JPG)$");
    std::regex txtPattern("\\.(txt|TXT)$");
    std::fstream file;
    if (std::regex_search(path, jpgPattern))
    {
        file.open(rootPath + path, std::ios::in | std::ios::binary);
    }
    else
    {
        file.open(rootPath + path, std::ios::in);
    }
    if (!file)
    {
        return makePacket("", 404);
    }
    std::istreambuf_iterator<char> begin(file), end; 
    std::string fileData(begin, end);
    file.close();

    std::string reply;
    std::string content_type;
    if (std::regex_search(path, jpgPattern))
    {
        content_type = "image/jpeg";
    }
    else if (std::regex_search(path, txtPattern))
    {
        content_type = "text/plain";
    }
    else
    {
        content_type = "text/html";
    }
    std::string content_length = std::to_string(fileData.size());
    reply.append("HTTP/1.1 200 OK\n");
    reply.append("Content-Type: " + content_type + "\n");
    reply.append("Content-Length: " + content_length + "\n\n");
    reply.append(fileData);
    reply.append("\n");

	return reply;
}

std::string post(const std::string& path, const std::string& http, const std::string& data)
{
    std::map<std::string, std::string> body_data;
    int index = 0;
    while (index < (int)data.size())
    {
        std::string tmp_key, tmp_value;
        while (index < (int)data.size() && data.at(index) != '=')
        {
            tmp_key.push_back(data.at(index++));
        }
        if (index >= (int)data.size())
        {
            break;
        }
        index++;
        
        while (index < (int)data.size() && data.at(index) != '&')
        {
            tmp_value.push_back(data.at(index++));
        }
        index++;
        body_data.insert(std::pair<std::string, std::string>(tmp_key, tmp_value));
    }
    if (body_data["login"] != "Winnie" || body_data["pass"] != "19260817")
    {
        return makePacket("<html><body>Checkin fail</body></html>");
    }
    else
    {
        return makePacket("<html><body>Checkin pass</body></html>");
    }
}

std::string makePacket(const std::string& fileData, const int& code)
{
	std::string reply;
	reply.append("HTTP/1.1 ");
    reply.append((code == 200) ? "200 OK\n" : "404 Not Found\n");
	reply.append("Server: Winnie\n");
	reply.append("Content-Type: text/html\n");
	int length = sizeof(fileData);
	std::string content_length = std::to_string(length);
	reply.append("Content-Length: " + content_length + "\n\n");
	if(length > 0)
    {
        reply.append(fileData + "\n");
    }
	return reply;
}

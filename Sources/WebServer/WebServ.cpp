/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atomatoe <atomatoe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/16 14:02:33 by atomatoe          #+#    #+#             */
/*   Updated: 2021/02/17 18:58:57 by atomatoe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WebServ.hpp"

WebServer::WebServer() : _port(-1), _autoIndex(false) { }

WebServer::~WebServer()
{
    
}

void WebServer::create_socket()
{
    this->_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->_address_len = (sizeof(this->_socket_addr));
    this->_socket_addr.sin_family = AF_INET;
    this->_socket_addr.sin_addr.s_addr = inet_addr(this->_ip.c_str()); // содержит адрес (номер) узла сети.
    this->_socket_addr.sin_port = htons(this->_port); // 80 - port с конфига  что делает htons (80 >> 8 | 80 << 8)
}

//int Server::createSocket(const std::string &host, const int port, int const & i) {
//	struct sockaddr_in addr;
//	int listen_sock;
//
//	ft_bzero(&addr, sizeof(addr));
//	addr.sin_family = AF_INET;
//	addr.sin_port = ft_htons(port);
//	addr.sin_addr.s_addr = inet_addr(host.c_str());
//
//	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
//		std::cerr << "listen_sock" << std::endl;
//		return (500);
//	}
//	fcntl(listen_sock, F_SETFL, O_NONBLOCK);
//	int opt = 1;
//	setsockopt(listen_sock, SOL_SOCKET,  SO_REUSEADDR, &opt, sizeof(opt));
//	if ((bind(listen_sock, (struct sockaddr*)& addr, sizeof(addr))) < 0) {
//		std::cerr << "bind error" << std::endl;
//		return (500);
//	}
//	if ((listen(listen_sock, 2048)) < 0){
//		std::cerr << "listen_sock error" << std::endl;
//		return (500);
//	}
//	std::cout << "listening " << port << "..." << std::endl;
//	virt_serv[i].setFd(listen_sock);
//	return (0);
//}

int WebServer::build_server() {
    if (setsockopt(this->_server_fd, SOL_SOCKET, SO_REUSEADDR, &this->_yes, sizeof(int))) // дает возможность повторно использовать сокет (повторять bind)
    {
        std::cerr << "setsockopt FAILED" << std::endl;
        return(-1);
    }
    if (bind(this->_server_fd,(const struct sockaddr *) &this->_socket_addr, this->_address_len)) // Аргумент address_len задает размер (в байтах) структуры данных, указываемой аргументом addr.
    {
        std::cerr << "bind FAILED" << std::endl;
        return(-1);
    }
    if (fcntl(this->_server_fd, F_SETFL, O_NONBLOCK)) // делаем сокет неблокирующемся
    {
        std::cerr << "fcntl FAILED" << std::endl;
        return(-1);
    }
    if (listen(this->_server_fd, 2048)) {
        std::cerr << "listen FAILED" << std::endl;
        return(-1);
    }
    return(0);
}

int WebServer::get_server_fd() { return(this->_server_fd); }
sockaddr_in *WebServer::get_socket_addr() { return(&this->_socket_addr); }
socklen_t *WebServer::get_address_len() { return(&this->_address_len); }
sockaddr_in *WebServer::get_out() { return(&this->_out); }



////////////////* from Timur */
/* get */
std::string                         WebServer::getIp() const { return _ip; }
int                                 WebServer::getPort() const { return _port; }
std::string                         WebServer::getServerName() const { return _serverName; }
std::string                         WebServer::getRootPath() const { return _rootPath; }
std::map<std::string, std::string>  WebServer::getErrorPage() const { return _errorPage; };
std::vector<Location>               WebServer::getLocations() const {
    return _locations;
}

/* set */
void            WebServer::setIp(std::string ip) {
    if (ip[0] == '.' || ip[ip.size() - 1] == '.')
        exitError("Incorrect value of ip: \"" + ip + "\"");
    char **splitted = ft_splitTim(ip.c_str(), '.');
    if (ft_strstrlen(splitted) != 4)
        exitError("Incorrect value of ip: \"" + ip + "\"");
    for (int i = 0; i < 4 ; i ++)
        if (!ft_str_is_num(splitted[i]))
            exitError("Incorrect symbols in ip value: \"" + ip + "\"");
    ft_free_strstr(splitted);
    this->_ip = ip;
}
void            WebServer::setPort(int port) {
    this->_port = port;
}
void            WebServer::setServerName(std::string serverName) {
    this->_serverName = serverName;
}
void            WebServer::setRootPath(std::string rootPath) {
    if (!isDirectory(rootPath))
        exitError("Smth wrong with root value: \"" + rootPath + "\"");
    this->_rootPath = rootPath;
}

/* add */
void            WebServer::addErrorPage(std::string error, std::string path) {
    if (!ft_str_is_num(error.c_str()))
        exitError("Incorrect value in error_page \"" + error + "\"");
    if (!isFileRead(path))
        exitError("Incorrect value in error_page \"" + path + "\"");
    _errorPage.insert(std::pair<std::string, std::string>(error, path));
}
void            WebServer::addLocation(Location location) {
    if (location.getRoot().empty())
        exitError("Location should be with root");
    for (std::vector<Location>::iterator it = _locations.begin(); it != _locations.end(); it++) {
        if (location == (*it))
            exitError("Locations should be with different url");
    }
    this->_locations.push_back(location);
}

bool  WebServer::getAutoIndex() const { return _autoIndex; }
void  WebServer::setAutoIndex(bool autoIndex) { _autoIndex = autoIndex; }


bool					operator==(const WebServer& server1, const WebServer& server2) {
    if (server1.getPort() == server2.getPort() && server1.getIp() == server2.getIp())
        return (true);
    return (false);
}
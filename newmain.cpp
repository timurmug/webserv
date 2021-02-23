//
// Created by Wynn Elease on 2/22/21.
//

#include "WebServ.hpp"
#include "./Request_Response/Request.hpp"
#include "./Request_Response/Response.hpp"
//#include "includes/includes.hpp"
#include "parse/ParseConfig.hpp"

void fd_init(std::vector<WebServer> &servers, fd_set &fd_write, fd_set &fd_read, int &max_fd)
{
	FD_ZERO(&fd_write); // зануление fd
	FD_ZERO(&fd_read); // зануление fd
	for(std::vector<WebServer>::iterator it = servers.begin(); it != servers.end(); it++) {
		FD_SET(it->get_server_fd(), &fd_read);
		max_fd = it->get_server_fd();
		//		FD_ZERO(&it->getFdRead());
		//		FD_ZERO(&it->getFdWrite());
		//FD_SET(it->get_server_fd(), &it->getFdWrite());
	}
}

void endOfReadingRequest(std::map<int, t_client>::iterator i, fd_set fd_write, fd_set fd_read,
						 WebServer server){
	close(i->first);
	i->second.receivedData->clear();
	i->second.toSendData->clear();
	FD_CLR(i->first, &fd_read);
	FD_CLR(i->first, &fd_write);
	server.get_list().erase(i);
}

void init_servers(std::vector<WebServer> & servers, size_t count_server)
{
	size_t i = 0;
	while(i != count_server)
	{
		servers[i].create_socket();
		servers[i].build_server();
		i++;
	}
}

void start_servers(std::vector<WebServer> servers)
{
	int max_fd;
	fd_set fd_write;
	fd_set fd_read;
	char *buf;
	int fd;
	Bytes tmp;
	size_t len;
	int ret;
	ssize_t ret2;
	if (!(buf = (char *) malloc(sizeof(char) * 2000001)))
		exit(1);

	int i = 1;
	while (i)
	{
		if (i++ == 0)
			break;
		fd_init(servers, fd_write, fd_read, max_fd);
		for (std::vector<WebServer>::iterator it = servers.begin(); it != servers.end(); ++it) {
			for (std::map<int, t_client>::iterator i = it->getClients().begin(); i != it->getClients().end(); ++i) {
				if (i->second.isHeadersEnded < 2)
					FD_SET(i->first, &fd_read);
				else
					FD_SET(i->first, &fd_write);
				if (i->first > max_fd)
					max_fd = i->first;
			}
		}

		select(max_fd + 1, &fd_read, &fd_write, NULL, NULL);

		for (std::vector<WebServer>::iterator it = servers.begin(); it != servers.end(); ++it) {
			if (FD_ISSET(it->get_server_fd(), &fd_read)) {
				fd = accept(it->get_server_fd(), 0, 0);
				if (fd > 0)
					it->addClient(fd);
			}
		}

		for (std::vector<WebServer>::iterator it = servers.begin(); it != servers.end(); ++it)
		{
			for (std::map<int, t_client>::iterator i = it->getClients().begin(); i != it->getClients().end(); ++i)
			{
				if (FD_ISSET(i->first, &fd_read))
				{
					ret = recv(i->first, buf, 2000000, 0);
					if (!i->second.isHeadersEnded && ret > 0)
					{
						i->second.receivedData->addData(buf, ret);
						if ((len = i->second.receivedData->findMemoryFragment(doubleCRLF, 4)) != (size_t) -1)
						{
							tmp = i->second.receivedData->cutData(len + 4);
							i->second.request = new Request(i->second.receivedData->toPointer());
							i->second.request->getReqBody().addData(tmp.toPointer(), tmp.getDataSize());
							i->second.isHeadersEnded = 1;
							ret = recv(i->first, buf, 0, 0);
							if (ret == 0)
							{
								i->second.isHeadersEnded = 2;
								goto tmp;
							}
						}
					}
					else if (ret > 0 && i->second.isHeadersEnded == 1)
					{
						i->second.request->setReqBody(buf, ret);
						if (strncmp(i->second.request->getTransferEncoding(), "chunked", 7) == 0 && (len = i->second.request->getReqBody().findMemoryFragment("0\r\n\r\n", 5) != (size_t) -1))
						{
							i->second.request->getReqBody().cutData(len + 5);
							i->second.request->ChunkedBodyProcessing();
							i->second.isHeadersEnded = 2;
							endOfReadingRequest(i, fd_write, fd_read, *it);
						} else if (ret < 2000000 || i->second.request->getReqBody().findMemoryFragment(doubleCRLF, 4) != (size_t) -1)
						{
							i->second.request->getReqBody().addData((char *) "", 1);
							i->second.isHeadersEnded = 2;
							endOfReadingRequest(i, fd_write, fd_read, *it);
						}
					}
					if (ret == 0)
					{
						endOfReadingRequest(i, fd_write, fd_read, *it);
						break;
					}
				}
				tmp:
				if (i->second.isHeadersEnded == 2)
				{
					i->second.response = new Response();
					i->second.toSendData->addData(i->second.response->give_me_response(*(i->second.request), *it), i->second.response->getLenOfResponse());
					ret2 = 0;
					while (ret2 != i->second.toSendData->getDataSize())
						ret2 += write(i->first, i->second.toSendData->toPointer() + ret2, i->second.toSendData->getDataSize() - ret2);
					close(i->first); // как отправили все данные клиенту, отсоединяемся
					i->second.isHeadersEnded = 0;
					delete i->second.response;
					delete i->second.request;
					i->second.receivedData->clear();
					i->second.toSendData->clear();
					FD_CLR(i->first, &fd_read);
					FD_CLR(i->first, &fd_write);
				}
			}
		}
	}
}

int main(int ac, char **av)
{
	ParseConfig parseConfig((ac == 2) ? av[1] : "configs/atomatoe/default.conf");
	std::vector<WebServer> servers =  parseConfig.parse();
	std::map<std::string, std::string> error_page;
	size_t count_server = servers.size();

	init_servers(servers, count_server);
	start_servers(servers);
	return(0);
}
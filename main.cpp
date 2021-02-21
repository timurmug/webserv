/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: atomatoe <atomatoe@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/02/16 18:15:55 by atomatoe          #+#    #+#             */
/*   Updated: 2021/02/18 16:16:11 by atomatoe         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WebServ.hpp"
#include "./Request_Response/Request.hpp"
#include "./Request_Response/Response.hpp"
#include "includes/includes.hpp"
#include "parse/ParseConfig.hpp"

void start_servers(std::vector<WebServer> servers)
{
    fd_set                  fd_write;
    fd_set                  fd_read;
    fd_set                  fd_write_tmp; // fd_write_tmp - нужен для работы select. При вызове макроса FD_ISSET портится fdшник, поэтому мы будем делать копию в fd_read_tmp, чтобы портилась она
    fd_set                  fd_read_tmp; // fd_read_tmp - нужен для работы select. При вызове макроса FD_ISSET портится fdшник, поэтому мы будем делать копию в fd_read_tmp, чтобы портилась она
    int max_fd = 0; // потом нужно найти самый большой fdшник и передевать его первым аргументом в select.
    FD_ZERO(&fd_write); // зануление fd
    FD_ZERO(&fd_read); // зануление fd
    FD_ZERO(&fd_write_tmp); // зануление fd
    FD_ZERO(&fd_read_tmp); // зануление fd
    for(size_t it = 0; it != servers.size(); it++)
    {
        FD_SET(servers[it].get_server_fd(), &fd_read); // заносим fd сокета(то есть сервера) в список fdшников
        FD_SET(servers[it].get_server_fd(), &fd_write); // заносим fd сокета(то есть сервера) в список fdшников
        max_fd = servers[it].get_server_fd();
        std::cout << "fd servers = " << max_fd << std::endl;
    }
    while(1)
    {
        fd_write_tmp = fd_write;
        fd_read_tmp = fd_read;
        select(max_fd + 1, &fd_read_tmp, &fd_write_tmp, NULL, NULL);
        for(size_t it = 0; it != servers.size(); it++)
        {
            if(FD_ISSET(servers[it].get_server_fd(), &fd_read_tmp)) // 1 блок = проверка на пришел кто то или нет
            {
                int fd = accept(servers[it].get_server_fd(), (struct sockaddr *) servers[it].get_out(), servers[it].get_address_len()); // fd - новый клиент
                if(fd > 0)
                {
                    FD_SET(fd, &fd_read); // заносим клиента в fd для чтения
                    FD_SET(fd, &fd_write); // заносим клиента в fd для записи
                    if(fd > max_fd)
                        max_fd = fd;
                    (servers[it].get_list())[fd].buff_read = 0;
                    (servers[it].get_list())[fd].buff_write = 0;
                }
            }
            for(std::map<int, t_client>::iterator it2 = (servers[it].get_list()).begin(); it2 != (servers[it].get_list()).end(); it2++) // 2 блок = на получение данных (отправил или нет)
            {
                if(FD_ISSET(it2->first, &fd_read_tmp))
                {
                    char* newbuf = (char *)malloc(sizeof(char) * 4096);
                    int ret = recv(it2->first, newbuf, 4096, 0);
                    if(ret == 0)
                    {
                        close(it2->first);
                        if(it2->second.buff_read)
                            free(it2->second.buff_read);
                        if(it2->second.buff_write)
                            free(it2->second.buff_write);
                        FD_CLR(it2->first, &fd_read);
                        FD_CLR(it2->first, &fd_write);
                        servers[it].get_list().erase(it2);
                        free(newbuf);
                        break ; // делаем break чтобы не наебнулась it++ 
                    }
                    else if(ret > 0)
                    {
                        newbuf[ret] = '\0';
                        it2->second.buff_read = str_join(it2->second.buff_read, newbuf);
                        std::cout << "запрос:\n" << it2->second.buff_read << std::endl;
                        // it2->second.buff_write = str_join(it2->second.buff_write, (char *)"HTTP/1.1 200 OK\n\n");
                        Request request(it2->second.buff_read, it2->first);
                        Response response;
                        it2->second.buff_write = str_join(it2->second.buff_write, response.give_me_response(request, servers[it]));
                    }
                    free(newbuf);
                }
                if(it2->second.buff_write)
                {
                    int ret = send(it2->first, it2->second.buff_write, strlen(it2->second.buff_write), 0);
                    if(ret != strlen(it2->second.buff_write))
                    {
                        char *newbuf = NULL;
                        newbuf = str_join(newbuf, it2->second.buff_write + ret);
                        free(it2->second.buff_write);
                        it2->second.buff_write = newbuf;
                    }
                    else
                    {
                        close(it2->first); // как отправили все данные клиенту, отсоединяемся
                        free(it2->second.buff_write);
                        it2->second.buff_write = NULL;
                    }
                }
            }
        }
    }
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
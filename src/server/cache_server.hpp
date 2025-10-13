#pragma once
#include <cstring>
#include <csignal>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "cache_controller.hpp"

class CacheServer
{
public:
    explicit CacheServer(CacheController& controller) :
    _controller(controller), _running(false) {};
    
    void run();
    string stop();

private:
    CacheController& _controller;
    bool _running = true;
    const string _socketPath = "/tmp/resolver.sock";

    void check_existing_instance() const;
    void daemonize() const;
    
    void accept_connections(int server_fd);
    void handle_client(int client_fd);
    void cleanup(int server_fd) const;

    int setup_socket() const;
};
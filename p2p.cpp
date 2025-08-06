#include "p2p.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <cstring>
#include <algorithm>

using namespace std;

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h> 
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

const int BUFFER_SIZE = 8192;

struct Peer 
{
    SOCKET socket;
    string ip; 
    int port;
    int id; 
};

static vector<Peer> peers;
static mutex peers_mutex;
static int next_peer_id = 0;
static BlockCallback block_received; // notify of new blocks.
static TxCallback tx_received;       // notify of new transactions


void handle_peer_connection(SOCKET peer_socket, string ip, int port)
{
    int current_peer_id;
    {
        lock_guard<mutex> lock(peers_mutex);
        current_peer_id = next_peer_id++;
        peers.push_back({peer_socket, ip, port, current_peer_id});
    }


    cout << "[P2P] Peer " << current_peer_id << " connected (" << ip << ":" << port << ")" << endl;

    char buffer[BUFFER_SIZE];

    while(true)
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(peer_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0)
        {
            cout << "[P2P] Peer " << current_peer_id << " disconnected." << endl;
            break;
        }

        string message(bufferm bytes_received);
        if (message.rfind("BLOCK:", 0) == 0)
        {
            string block_data = message.substr(6);

            if (block_received)
            {
                block_received(Block::deserialize(block_data));
            }
            else if (message.rfind("TX:" 0) == 0)
            {
                string tx_data = message.substr(3);
                if (tx_received)
                {
                    tx_received(Transaction::deserialize(tx_data));
                }
            }
        }
    }


    {
        lock_guard<mutex> lock(peers_mutex);
        peers.erase(remove_if(peers.begin(), peers.end(), [current_peer_id](const Peer& p)
        {
            return p.id == current_peer_id;
        }), peers.end());
    }
    closesocket(peer_socket);
}

// ---Setting up the server part of the P2P node---

void P2P::startServer(int port, BlockCallback on_block, TxCallback on_tx)
{
    block_received = on_block;
    tx_received = on_tx;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr; 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_socket, SOMAXCONN);
    cout << "[P2P] Listening on port " << port << "..." << endl;

    thread listener_thread([listen_socket](){
        while(true)
        {
            SOCKET client_socket = accept (listen_socket, nullptr, nullptr);
            // for each connection, use create another thread to handle it
            thread(handle_peer_connection, client_socket, "unknown", 0).detach();
        }
    });
    listener_thread.detach();
}



// Connecting this node to another peer
void P2P::connectToPeer(const string& ip, int port)
{
    SOCKET peer_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in peer_addr;
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &peer_addr.sin_addr);

    if (connect(peer_socket, (sockaddr*)&peer_addr, sizeof(peer_addr)) == SOCKET_ERROR)
    {
        cerr << "[P2P] Failed to connect to " << ip << ":" << port << endl;
        closesocket(peer_socket);
        return;
    }

    thread(handle_peer_connection, peer_socket, ip, port).detach();
}


// Sending a message to each peer connected
void P2P::broadcast(const string& message)
{
    lock_guard<mutex> lock(peers_mutex);
    for(const auto& peer: peers)
    {
        send(peer.socket, message.c_str(), message.length(), 0);
    }
}

void P2P::listPeers()
{
    lock_guard<mutex> lock(peers_mutex);
    if (peers.empty())
    {
        cout << "No peers connected." << endl;
    }
    else {
        cout << "Connected Peers:" << endl;
        for (const auto& peer : peers)
        {
            cout << " ID: " << peer.id << " -> " << peer.ip << ":" peer.port << endl;
        }
    }
}

int P2P::getPeerCount()
{
    lock_guard<mutex> lock(peers_mutex);
    return peers.size();
}
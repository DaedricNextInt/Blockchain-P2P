#ifndef P2P_H
#define P2P_H

#include <string>
#include <vector>
#include <functional>
#include "blockchain.h"
using namespace std;

// Block          - chunk of code
// callback       - code that gets called later
// Block callback - you pass a block as the callback.

/*  Call back functions*/
using BlockCallback = function<void(const Block&, int)>;
using TxCallback = function<void(const Transaction&)>;
using ChainCallback = function<void(const string&)>;

/*Namespaces - a namspace in C++ is a way to group related functions, classes, variables, under a scope.
               it helps avoid name conflicts.*/

namespace P2P 
{
    void startServer(int port, BlockCallback on_block, TxCallback on_tx, ChainCallback chain);
    void connectToPeer(const string& ip, int port);
    void broadcast(const string& message);
    void sendToPeer(int peer_id, const string& message);
    void sendFile(int peer_id, const string& filepath, const string& recipient_pub_key);
    void listPeers();
    int getPeerCount();
}

#endif
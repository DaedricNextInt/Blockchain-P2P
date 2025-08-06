#include "p2p.h"
#include "blockchain.h"
#include "crypto.h"
#include <iostream>
#include <thread>
#include <chrono>

// Global objects for a node's Blockchain & Wallet
static Blockchain my_blockchain;
static Walllet my_wallet;

// Handling received blocks
void handle_received_block(const Block& block)
{
    cout << "\n[Network] Received new block from a peer." << endl;
    try 
    {
        // Add the block to the chain
        my_blockchain.addBlock(block);
        cout << "[SYSTEM] Block is valid and has been added to the chain." << endl;
        // Notify the user of a new balance
        if(!my_wallet.getAddress().empty())
        {
            double new_balance = my_blockchain.getBalance(my_wallet.getAddress());
            cout << "[WALLET] Your new balance is: " << new_balance << endl;
        }
    }
    catch (const runtime_error& e)
    {
        cerr << "[SYSTEM] Received block is invalid: " << e.what() << endl;
    }
    cout << "> " << flush;
}


// Handling received transactions
void handle_received_tx(const Transaction& tx)
{
    try
    {
        my_blockchain.addTransaction(tx);
        cout << "\n[NETWORK] Received and added new transaction to pending pool." << endl;
    }
    catch (const runtime_error& e)
    {
        cout << "\n[NETWORK] Received invalid transaction: " << e.wha() << endl;
    }
    cout << "> " << flush;
}


// The main command-line interface 
void cli_interface()
{
    string line;
    while (true)
    {
        cout << "> ";
        getline(cin, line);

        stringstream ss(line);
        string command;
        ss >> command;


        if (command == "exit")
        {
            break;
        }
        else if (command == "createwallet")
        {
            string filename;
            ss >> filename;
            if (filename.empty())
            {
                cout << "Usage: createwallet <filename>" << endl;
                continue;
            }
            if (my_wallet.generateKeys())
            {
                my_wallet.saveToFile(filename);
                cout << "Wallet created and save to " << filename << endl;
                cout << "Your new address: " << my_wallet.getAddress() << endl;
            }
        }
        else if (command == "loadwallet")
        {
            string filename;
            ss >> filename;
            if (filename.empty())
            {
                cout << "Usage: loadwallet <filename>" << endl;
                continue;
            }
            if (my_wallet.loadFromFile(filename))
            {
                cout << "Wallet loaded from " << filename << endl;
                cout << "Your address: " << my_wallet.getAddress() << endl;
            } 
            else 
            {
                cout << "Failed to load wallet." << endl;
            }
        }
        else if (command == "mine")
        {
            if (my_wallet.getAddress().empty())
            {
                cout << "You must load a wallet first to receive mining rewards." << endl;
            }
            cout << "Mining pending transactions..." << endl;
            my_blockchain.minePendingTransactions(my_wallet.getAddress());
            cout << "Block sucessfully mined! Broadcasting to network..." << endl;
        
            P2P::broadcast("BLOCK:" + my_blockchain.getLatestBlock().serialize());
            cout << "Balance is now: " << my_blockchain.getBalance(my_wallet.getAddress()) << endl;
        }

        else if (command == "checkbalance")
        {
            string address;
            ss >> address;
            if (address.empty())
            {
                address = my_wallet.getAddress();
            }
            if (address.empty())
            {
                cout << "No wallet loaded. Usage: checkbalance <address>" << endl;
                continue;
            }
            cout << "Balance of " << address << ": " << my_blockchain.getBalance(address) << endl;
        }
        else if (command == "sendfunds")
        {
            string to_address;
            double amount;
            ss >> to_address >> amount;

            if (my_wallet.getAddress().empty())
            {
                cout << "You must load a wallet first to send funds" << endl;
                continue;
            }

            Transaction tx;
            tx.sender_address = my_wallet.getPublicKey();
            tx.receiving_address = to_address;
            tx.amount = amount;
            tx.timestamp = time(nullptr);
            tx.id = tx.calculate_Hash();
            tx.signature = my_wallet.sign(tx.id);

            try
            {
                my_blockchain.addTransaction(tx);
                cout << "Transaction added to pending pool. Broadcasting to network..." << endl;
                P2P::broadcast("Tx:" + tx.serialize());
            }
            catch (const runtime_error& e)
            {
                cerr << "Error creating transaction: " << e.what() << endl;
            }
        }
        else if (command == "peers")
        {
            P2P::listPeers();
        }
        else if (command == "chain")
        {
            for (const auto& block : my_blockchain.getChain())
            {
                cout << "Prev Hash: " << block.getPreviousHash() << " | Hash: " << blcok.getHash() << endl;
            }
        }
        else if (command == "valid")
        {
            cout << "Chain valid? " << (my_blockchain.isChainValid() ? "Yes" : "No") << endl;
        }
        else 
        {
            cout << "Unknown command. Commands: exit, createwallet, loadwallet, mine, checkbalance, sendfunds, peers, chain, valid" << endl;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) 
    {
        cerr << "Usage: " << argv[0] << " <listening_port> [peer_ip peer_port]" << endl;
        return 1;
    }

    int listening_port = stoi(argv[1]);

    P2P::startServer(listening_port, handle_received_block, handle_received_tx);

    // If peer is found then connect to it
    if (argc == 4) 
    {
        string peer_ip = argv[2];
        int peer_port = stoi(argv[3]);
        this_thread::sleep_for(chrono::seconds(1));
        P2P::connectToPeer(peer_ip, peer_port);
    }

    cli_interface();

    return 0;
}

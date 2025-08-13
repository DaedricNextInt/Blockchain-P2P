#include "p2p.h"
#include "blockchain.h"
#include "crypto.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>

// Global objects for a node's Blockchain & Wallet
static Blockchain my_blockchain;
static Wallet my_wallet;

// Handling received blocks
/**
 * @brief Handles the logic for processing a new block received from another peer on the network.
 * @param block A constant reference to the Block object that was received.
 * @param peer_id The unique identifier for the peer that sent this block.
 */
void handle_received_block(const Block& block, int peer_id)
{
    // --- Start of debugging output ---
    cout << "\n\n--- DEBUG: ENTERING handle_received_block ---" << endl;
    cout << "DEBUG: Received a block from peer_id: " << peer_id << endl;
    cout << "DEBUG: Hash of received block: " << block.getHash() << endl;
    cout << "DEBUG: Previous hash of received block: " << block.getPreviousHash() << endl;
    // --- End of debugging output ---

    cout << "\n[Network] Received new block from a peer." << endl;
    
    const Block& latest_block = my_blockchain.getLatestBlock();

    // --- Start of debugging output ---
    cout << "DEBUG: Hash of our current latest block: " << latest_block.getHash() << endl;
    // --- End of debugging output ---

    if (block.getPreviousHash() == latest_block.getHash())
    {
        // --- Start of debugging output ---
        cout << "DEBUG: CONDITION MET - Previous hash matches. Attempting to append block." << endl;
        // --- End of debugging output ---
        try
        {
            my_blockchain.addBlock(block);
            cout << "\n[SYSTEM] Appended new block to chain." << endl;
            // --- Start of debugging output ---
            cout << "DEBUG: Successfully added block. New chain height: " << my_blockchain.getChain().size() << endl;
            // --- End of debugging output ---
        }
        catch (const runtime_error& e) 
        {
            // --- Start of debugging output ---
            cout << "DEBUG: CATCH - Error while adding block to the chain." << endl;
            // --- End of debugging output ---
            cerr << "\n[SYSTEM] Error appending block: " << e.what() << endl;
        }
    }
    else if (block.getPreviousHash() > latest_block.getHash())
    {
        // --- Start of debugging output ---
        cout << "DEBUG: CONDITION MET - Fork detected. Received block's prev_hash is greater." << endl;
        // --- End of debugging output ---
        cout << "\n[SYSTEM] Blockchain fork detected. Requesting chain from " 
        << peer_id << " for synchronization." << endl;
        
        // --- Start of debugging output ---
        cout << "DEBUG: Sending 'GET_CHAIN' message to peer " << peer_id << endl;
        // --- End of debugging output ---
        P2P::sendToPeer(peer_id, "GET_CHAIN");
    }
    else 
    {
        // --- Start of debugging output ---
        cout << "DEBUG: NO CONDITIONS MET - Block is old or from an irrelevant fork. Ignoring." << endl;
        // --- End of debugging output ---
        cout << "\n[SYSTEM] Received block is older than current head. Ignoring." << endl;
    }
    
    // --- Start of debugging output ---
    cout << "--- DEBUG: EXITING handle_received_block ---" << endl;
    // --- End of debugging output ---

    cout << "> " << flush;
}


void handle_received_chain(const string& chain_data)
{
    cout << "\n[SYSTEM] Received full chain response from peer." << endl;
    Blockchain received_chain;
    received_chain.deserialize(chain_data);

    if (received_chain.isChainValid() && received_chain.getChain().size() > my_blockchain.getChain().size())
    {
        my_blockchain.replaceChain(received_chain.getChain());
        cout << "Sucessfully synchronized to longer chain." << endl;
    }
    else 
    {
        cout << "[SYSTEM] Received chain is not valid. Keeping current chain." << endl;
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
        cout << "\n[NETWORK] Received invalid transaction: " << e.what() << endl;
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
            my_blockchain.minePendingTransaction(my_wallet.getAddress());
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
            tx.sending_address = my_wallet.getPublicKey();
            tx.receiving_address = to_address;
            tx.amount = amount;
            tx.timestamp = time(nullptr);
            tx.id = tx.calculate_Hash();
            tx.signature = my_wallet.sign(tx.id);

            try
            {
                my_blockchain.addTransaction(tx);
                cout << "Transaction added to pending pool. Broadcasting to network..." << endl;
                P2P::broadcast("Tx:" + tx.serializer());
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
                cout << "Prev Hash: " << block.getPreviousHash() << " | Hash: " << block.getHash() << endl;
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

    P2P::startServer(listening_port, handle_received_block, handle_received_tx, handle_received_chain);

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

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <string>
#include <vector>
#include <ctime>
#include "crypto.h"
#include <string>

using namespace std;

/* Transaction*/
struct Transaction
{
    string id; 
    string sending_address;
    string receiving_address;
    double amount;
    string file_metadata; // For file transfers
    time_t timestamp;
    string signature;
    string calculate_Hash() const;
    string serializer() const;
    static Transaction deserializer(const string& data);
    bool isValid() const;
};

// Class - [Block] for representation of a block within a blockchain
class Block {
public: 
    Block(int index, time_t timestamp, vector<Transaction> transactions, string previous_hash);

    int getIndex() const {return index; }
    string getHash() const {return hash;}
    string calculateHash() const;
    string getPreviousHash() const {return previous_hash;}
    vector<Transaction> getTransactions() const { return transactions;}
    void mineBlock(int difficulty);
    bool isValidTransaction() const;
    string serialize() const;
    static Block deserialize(const string& data);
private:
    int index;
    time_t timestamp;
    vector<Transaction> transactions;
    string previous_hash;
    string hash;
    int nonce;
    //string calculateHash();
};

// Class - [Blockchain] represents the entire blockchain
class Blockchain {
public: 
    Blockchain();
    void addBlock(const Block& new_block);
    Block getLatestBlock() const;
    bool isChainValid();
    void minePendingTransaction(const string& miner_addr);
    void addTransaction(const Transaction& tx);
    double getBalance(const string& addr);
    bool hasSufficientFunds(const string& sender_address, double amount);

    void deserialize(const string& data);
    string block_serialize() const;

    vector<Block> getChain() const;
    void replaceChain(const vector<Block>& new_chain);
private:
    vector<Block> chain;
    vector<Transaction> pending_transactions;
    int difficulty;
    double mining_reward;

};

#endif
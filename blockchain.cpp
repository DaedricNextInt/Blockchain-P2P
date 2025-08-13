#include "blockchain.h"
#include <iostream>
#include <sstream> 

using namespace std;

/*Transaction*/
string Transaction::calculate_Hash() const
{
    return Crypto::sha256(sending_address + receiving_address +
         to_string(amount) + to_string(timestamp));
}

// A transaction is only valid if its signature can be 
// verified with the sender's [public key].
bool Transaction::isValid() const
{
    if (sending_address == "0") // Reward for Proof of Work (Mining)
    {
        return true;
    }

    if (signature.empty())
    {
        return false;
    }

    // Verifying the signature against the transaction's hash
    return Wallet::verify(sending_address, this->calculate_Hash(), signature);
}


// Converts each transaction to a string to be sent over the network
string Transaction::serializer() const
{
    stringstream ss;
    ss << id << "," << sending_address << "," << receiving_address 
    << "," << amount << "," << timestamp << "," << signature;
    return ss.str();
}

// Converts a string back into a Transaction object.
Transaction Transaction::deserializer(const string& data)
{
    stringstream ss(data);
    string item;
    Transaction tx;
    getline(ss, tx.id, ',');
    getline(ss, tx.sending_address, ',');
    getline(ss, tx.receiving_address, ',');
    getline(ss, item, ',');
    tx.amount = stod(item);
    getline(ss, item, ','); 
    tx.timestamp = stoll(item);
    getline(ss, tx.signature, ',');
    return tx;
}

/*Block Implementation*/

Block::Block(time_t timestamp, vector<Transaction> transactions, string previous_hash) 
: timestamp(timestamp), transactions(transactions), previous_hash(previous_hash), nonce(0)
{
    hash = calculateHash(); 
}

// The block's hashed info is all of the data from this 
// a block and previous blocks. This is what forms the 
// blockchain as it's a hash of all linked blocks [blockchain]
string Block::calculateHash() const
{
    string tx_hashes;
    for (const auto& tx : transactions) 
    {
        tx_hashes += tx.id;
    }
    return Crypto::sha256(previous_hash + to_string(timestamp) 
    + tx_hashes + to_string(nonce));
}

// Proof of Work (Mining) algorithm
void Block::mineBlock(int difficulty) 
{
    // Starting the hash with a string of zeros
    string target(difficulty, '0');
    // Recalcuting the hash until it meets the target
    while (hash.substr(0, difficulty) != target) 
    {
        nonce++;
        hash = calculateHash();
    }
    cout << "Block mined: " << hash << endl;
}

// Validating all transactions within the Block
bool Block::isValidTransaction() const
{
    for(const auto& tx : transactions)
    {
        if (!tx.isValid()) 
        {
            cout << "Invalid transaction: " << tx.id << endl;
            return false;
        }
    }
    return true;
}


// Converting a block to a string to be sent over network
 string Block::serialize() 
 {
    stringstream ss;
    ss << timestamp << "|" << previous_hash << "|" << nonce << "|"
     << hash << "|" << transactions.size() << "|";
    for (const auto& tx : transactions)
    {
        ss << tx.serializer() << ";";
    }
    return ss.str();
 }
 

 // Converting a string back into a block.
 Block Block::deserialize(const string& data)
 {
    stringstream ss(data);
    string item;

    getline(ss, item, '|'); 
    time_t timestamp = stoll(item);
    getline(ss, item, '|'); 
    string prev_hash = item;
    getline(ss, item, '|');
    int nonce_i = stoi(item);
    getline(ss, item, '|');
    string hash_i = item;
    getline(ss, item, '|');
    int tx_count = stoi(item);

    vector<Transaction> transactions;
    for (int i =0; i < tx_count; i++)
    {
        getline(ss, item, ';');
        transactions.push_back(Transaction::deserializer(item));
    }

    Block block(timestamp, transactions, prev_hash);
    block.nonce =  nonce_i;
    block.hash = hash_i;
    return block;
 }


/* Blockchain Implementation*/



//  Serializing the entire blockchain into a single string.
//  [block_count] | [serialized_block_1] | [serialized_block_2] || ... 
string Blockchain::block_serialize() const
{
    stringstream ss;

    ss << chain.size() << "|";

    for (const auto& block : chain)
    {
        ss << block.serializer() << "||";
    }

    return ss.str();
}

// Deserializing a string to reconstruct the blockchain. 
void Blockchain::deserialize(const string& data)
{
    stringstream ss(data);
    string item;

    getline(ss, item, '|');
    int chain_size = 0;

    if (!item.empty())
    {
        chain_size = stoi(item);
    }

    vector<Block> new_chain;

    for (int i = 0; i < chain_size; i++)
    {
        getline(ss, item, '|');
        getline(ss, item, '|');

        new_chain.push_back(Block::deserialize(item));
    }

    this->chain = new_chain;
}

// Creating the first block in the chain by using the constructor 
// "Genesis Block"
Blockchain::Blockchain() : difficulty(4), mining_reward(100.0)
{
    vector<Transaction> genesis_txs;
    chain.emplace_back(time(nullptr), genesis_txs, "0");
}

Block Blockchain::getLatestBlock() const
{
    return chain.back();
}


// Getter method for grabbing the chain
vector<Block> Blockchain::getChain() const
{
    return chain;
}

// Adding a pre-mined block to the chain. This will be used
// when adding a block from the network.

/* Whats happening is a race condition. Both systems that are mining the
   block aren't adopting the same block. 

   In order to fix you would need to just update to the longest block.
*/
void Blockchain::addBlock(const Block& new_block)
{
    if(new_block.getPreviousHash() == getLatestBlock().getHash())
    {
        chain.push_back(new_block);
    }
    else
    {
        throw runtime_error("Cannot add block: Previous hash didn't match.");
    }
}

/* Creating a new block with all pending transactions and then will mine it*/
void Blockchain::minePendingTransaction(const string& miner_address)
{
    //Creating the reward transaction for the miner of the block
    Transaction reward_tx;
    reward_tx.sending_address = "0"; // system generated reward
    reward_tx.receiving_address = miner_address;
    reward_tx.amount = mining_reward;
    reward_tx.id = reward_tx.calculate_Hash();
    pending_transactions.push_back(reward_tx);

    //Creating a newer block to mine
    Block new_block(time(nullptr), pending_transactions, getLatestBlock().getHash());
    new_block.mineBlock(difficulty);
    chain.push_back(new_block);

    //Clearing each pending transaction
    pending_transactions.clear();
}


// Validating a transaction before adding it the pending pool
void Blockchain::addTransaction(const Transaction& tx)
{
    if (tx.sending_address.empty() || tx.receiving_address.empty())
    {
        throw runtime_error("Transaction must include sender and receiver address.");
    }
    if (!tx.isValid())
    {
        throw runtime_error("Cannot add invalid transaction to chain.");
    }
    if (tx.amount > 0 && !hasSufficientFunds(tx.sending_address, tx.amount))
    {
        throw runtime_error("Insufficient funds for transaction.");
    }
    pending_transactions.push_back(tx);
}

// Calculating a wallet's balance by summing up all transactions within each block
double Blockchain::getBalance(const string& address)
{
    if (address.empty())
    {
        return 0.0;
    }

    double balance = 0.0;
    for (const auto& block : chain)
    {
        for (const auto& tx : block.getTransactions())
        {
            if (tx.sending_address == address)
            {
                balance += tx.amount;
            }
        }
    }

    return balance;
}

bool Blockchain::hasSufficientFunds(const string & sending_address, double amount)
{
    return getBalance(sending_address) >= amount;
}

// Verifying the integrity of the chain
bool Blockchain::isChainValid()
{
    for (size_t i = 1; i < chain.size(); i++)
    {
        const Block& current_block = chain[i];
        const Block& previous_block = chain[i - 1];

       if (!current_block.isValidTransaction())
       {
        return false;
       } 

       if (current_block.getHash() != current_block.calculateHash())
       {
        return false;
       }

       if (current_block.getPreviousHash() != previous_block.getHash())
       {
        return false;
       }
    }
    return true;
}


// If a peer sends a longer or valid chain then adopt it
void Blockchain::replaceChain(const vector<Block>& new_chain)
{
    if (new_chain.size() > chain.size())
    {
        cout << "Longer chain detected. Replacing current chain" << endl;
        chain = new_chain;
    }
}
#include "blockchain.h"
#include <iostream>
#include <sstream>> 

using namespace std;

/*Transaction*/
string Transaction::calculate_Hash()
{
    return Crypto::sha256(sending_address + receiving_address +
         to_string(amount) + to_string(timestamp));
}

// A transaction is only valid if its signature can be 
// verified with the sender's [public key].
bool Transaction::isValid()
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
string Transaction::serializer()
{
    stringstream ss;
    ss << id << "," << sending_address << "," << receiving_address << "," << amount << "," << timestamp << "," << signature;
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

// The block's hashed info is all of the data from thisa block and previous blocks
// This is what forms the blockchain as it's a hash of all linked blocks [blockchain]
string Block::calculateHash() 
{
    string tx_hashes;
    for (const auto& tx : transactions) 
    {
        tx_hashes += tx.id;
    }
    return Crypto::sha256(previous_hash + to_string(timestamp) + tx_hashes + to_string(nonce));
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
bool Block::isValidTransaction()
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


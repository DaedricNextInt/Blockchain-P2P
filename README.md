# Blockchain-P2P

# -- How to run -- 

### 1. Start the first peer
./p2p_wallet 8080

### 2. In a new terminal or separate PC, start a second peer and connect to the first one. 
./p2p_wallet 8081 127.0.0.1 8080

### 3. Use the CLI commands on either peer terminal.
> createwallet my_wallet.txt
> loadwallet my_wallet.txt
> mine
> checkbalance <wallet_address>
> sendfunds <receiving_address> 100.5
>  
cmake_minimum_required(VERSION 3.10)

project(P2P_Wallet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)

add_executable(p2p_wallet
    main.cpp
    blockchain.cpp
    crypto.cpp
    p2p.cpp
)


target_link_libraries(p2p_wallet PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)
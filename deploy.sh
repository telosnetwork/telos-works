#! /bin/bash

# contract
if [[ "$1" == "works" ]]; then
    contract=works
else
    echo "need contract"
    exit 0
fi

# account
account=$2

#network
if [[ "$3" == "mainnet" ]]; then 
    url=http://api.tlos.goodblock.io # Telos Mainnet
    network=Mainnet
elif [[ "$3" == "testnet" ]]; then
    url=https://testnet.telosusa.io/ # Basho Testnet
    network=Testnet
elif [[ "$3" == "local" ]]; then
    url=http://127.0.0.1:8888
    network=Local
else
    echo "need network"
    exit 0
fi

echo ">>> Deploying $contract contract to $account on Telos $network..."

# eosio v1.8.0
cleos -u $url set contract $account ./build/$contract/ $contract.wasm $contract.abi -p $account
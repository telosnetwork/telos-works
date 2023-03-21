# Telos Works
Telos Works is a Worker Proposal System for the Telos Blockchain Network.

## Setup

To begin, navigate to the project directory: `telos-works/`

    mkdir build && mkdir build/example

    chmod +x build.sh

    chmod +x deploy.sh

## Build

    ./build.sh works

## Test

    cd build/tests

    ./unit_test -l all -r detailed -t works_tests -- --verbose

## Deploy

    ./deploy.sh works { account } { mainnet | testnet | local }

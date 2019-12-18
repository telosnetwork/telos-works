#! /bin/bash

# new contract name
new_contract="$1"

# make contract folders
mkdir contracts/$new_contract &&
mkdir contracts/$new_contract/include &&
mkdir contracts/$new_contract/resources &&
mkdir contracts/$new_contract/src &&

# make contract files
touch contracts/$new_contract/include/$new_contract.hpp &&
touch contracts/$new_contract/resources/$new_contract.clauses.md &&
touch contracts/$new_contract/resources/$new_contract.contracts.md &&
touch contracts/$new_contract/src/$new_contract.cpp &&

## make build folders
mkdir build/$new_contract

# TODO: add new contract to build and deploy scripts

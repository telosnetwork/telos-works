#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <fc/variant_object.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <iostream>
#include <boost/container/map.hpp>
#include <map>

#include <Runtime/Runtime.h>
#include <iomanip>

#include "works_tester.hpp"

using namespace eosio;
using namespace eosio::testing;
using namespace eosio::chain;
using namespace fc;
using namespace std;
using namespace trail::testing;
using namespace testing;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(works_tests)

    BOOST_FIXTURE_TEST_CASE( configuration_setting, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( transfer_protection, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_setup, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
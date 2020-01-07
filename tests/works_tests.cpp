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

        //initialize
        string app_name = "Telos Works";
        string app_version = "v2.0.0";
        name initial_admin = name("telos.works");
        asset available_funds = asset::from_string("0.0000 TLOS");
        asset reserved_funds = asset::from_string("0.0000 TLOS");
        asset deposited_funds = asset::from_string("0.0000 TLOS");
        asset paid_funds = asset::from_string("0.0000 TLOS");
        double quorum_thresh = 5.0;
        double yes_thresh = 50.0;
        double quorum_refund_thresh = 3.0;
        double yes_refund_thresh = 35.0;
        asset min_fee = asset::from_string("30.0000 TLOS");
        double fee_percent = 5.0;
        uint16_t min_milestones = 1;
        uint16_t max_milestones = 12;
        uint32_t milestone_length = 300;
        asset min_requested = asset::from_string("1000.0000 TLOS");
        asset max_requested = asset::from_string("500000.0000 TLOS");

        //get config table
        fc::variant works_config = get_works_config();

        //assert table values
        BOOST_REQUIRE_EQUAL(works_config["app_name"], app_name);
        BOOST_REQUIRE_EQUAL(works_config["app_version"], app_version);
        BOOST_REQUIRE_EQUAL(works_config["admin"].as<name>(), initial_admin);
        BOOST_REQUIRE_EQUAL(works_config["available_funds"].as<asset>(), available_funds);
        BOOST_REQUIRE_EQUAL(works_config["reserved_funds"].as<asset>(), reserved_funds);
        BOOST_REQUIRE_EQUAL(works_config["deposited_funds"].as<asset>(), deposited_funds);
        BOOST_REQUIRE_EQUAL(works_config["paid_funds"].as<asset>(), paid_funds);
        BOOST_REQUIRE_EQUAL(works_config["quorum_threshold"].as<double>(), quorum_thresh);
        BOOST_REQUIRE_EQUAL(works_config["yes_threshold"].as<double>(), yes_thresh);
        BOOST_REQUIRE_EQUAL(works_config["quorum_refund_threshold"].as<double>(), quorum_refund_thresh);
        BOOST_REQUIRE_EQUAL(works_config["yes_refund_threshold"].as<double>(), yes_refund_thresh);
        BOOST_REQUIRE_EQUAL(works_config["min_fee"].as<asset>(), min_fee);
        BOOST_REQUIRE_EQUAL(works_config["fee_percent"].as<double>(), fee_percent);
        BOOST_REQUIRE_EQUAL(works_config["min_milestones"].as<uint16_t>(), min_milestones);
        BOOST_REQUIRE_EQUAL(works_config["max_milestones"].as<uint16_t>(), max_milestones);
        BOOST_REQUIRE_EQUAL(works_config["milestone_length"].as<uint32_t>(), milestone_length);
        BOOST_REQUIRE_EQUAL(works_config["min_requested"].as<asset>(), min_requested);
        BOOST_REQUIRE_EQUAL(works_config["max_requested"].as<asset>(), max_requested);

        //initialize
        string new_version = "v2.0.1";

        //send setversion trx
        works_setversion(new_version, initial_admin);
        produce_blocks();

        //get new config table
        works_config = get_works_config();

        //check app version updated
        BOOST_REQUIRE_EQUAL(works_config["app_version"], new_version);

        //initialize
        name new_works_admin = name("testaccounta");

        //send setadmin trx
        works_setadmin(new_works_admin, initial_admin);
        produce_blocks();

        //get new config table
        works_config = get_works_config();

        //check admin updated
        BOOST_REQUIRE_EQUAL(works_config["admin"].as<name>(), new_works_admin);

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( deposit_withdraw, works_tester ) try {

        //send transfer to works
        base_tester::transfer(testa, works_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //withdraw half of tlos back to eosio.token balance
        works_withdraw(testa, asset::from_string("50.0000 TLOS"));
        produce_blocks();

        //get new account table
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert new balance
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("50.0000 TLOS"));
        

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_draft_editing, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_accept, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_fail, works_tester ) try {
        cout << "it works" << endl;
    } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
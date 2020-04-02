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
using namespace decidetesting::testing;
using namespace testing;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(works_tests)

    BOOST_FIXTURE_TEST_CASE( configuration_setting, works_tester ) try {

        //======================== check initial config ========================

        //initialize
        string app_name = "Telos Works";
        string app_version = "v2.0.0";
        name initial_admin = name("works.decide");
        asset available_funds = asset::from_string("100000.0000 TLOS");
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
        uint32_t milestone_length = 2505600; //29 days in seconds
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

        //======================== change version ========================

        //initialize
        string new_version = "v2.0.1";

        //send setversion trx
        works_setversion(new_version, initial_admin);
        produce_blocks();

        //get new config table
        works_config = get_works_config();

        //check app version updated
        BOOST_REQUIRE_EQUAL(works_config["app_version"], new_version);

        //======================== change admin ========================

        //initialize
        name new_works_admin = name("testaccounta");

        //send setadmin trx
        works_setadmin(new_works_admin, initial_admin);
        produce_blocks();

        //get new config table
        works_config = get_works_config();

        //check admin updated
        BOOST_REQUIRE_EQUAL(works_config["admin"].as<name>(), new_works_admin);

        //======================== change thresholds ========================

        //initialize
        double new_quorum = 15.00;
        double new_yes = 70.00;
        double new_quorum_refund = 5.00;
        double new_yes_refund = 55.00;

        //send setadmin trx
        works_setthresh(new_quorum, new_yes, new_quorum_refund, new_yes_refund, new_works_admin);
        produce_blocks();

        //get new config table
        works_config = get_works_config();

        //check admin updated
        BOOST_REQUIRE_EQUAL(works_config["quorum_threshold"].as<double>(), new_quorum);
        BOOST_REQUIRE_EQUAL(works_config["yes_threshold"].as<double>(), new_yes);
        BOOST_REQUIRE_EQUAL(works_config["quorum_refund_threshold"].as<double>(), new_quorum_refund);
        BOOST_REQUIRE_EQUAL(works_config["yes_refund_threshold"].as<double>(), new_yes_refund);

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( deposit_withdraw, works_tester ) try {

        //send transfer to works
        base_tester::transfer(testa, works_name, "100.0000 TLOS", "", token_name);
        produce_blocks();

        //get tables
        fc::variant works_conf = get_works_config();
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("100.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(works_conf["deposited_funds"].as<asset>(), asset::from_string("100.0000 TLOS"));

        //withdraw half of tlos back to eosio.token balance
        works_withdraw(testa, asset::from_string("50.0000 TLOS"));
        produce_blocks();

        //get udpated tables
        testa_works_acct = get_works_account(testa, tlos_sym);
        works_conf = get_works_config();

        //assert new balance
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("50.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(works_conf["deposited_funds"].as<asset>(), asset::from_string("50.0000 TLOS"));

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_draft_editing, works_tester ) try {

        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym);

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== add milestone ========================

        //intilalize
        uint64_t new_ms_id = milestones + 1;
        asset new_ms_requested = asset::from_string("500.0000 TLOS");
        name ballot_name = name(0);
        map<name, asset> blank_results;

        //push addmilestone trx
        works_addmilestone(prop_name, new_ms_requested, testa);
        produce_blocks();

        //get new milestone
        fc::variant ms = get_works_milestone(prop_name, new_ms_id);
        blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), new_ms_id);
        BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
        BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), new_ms_requested);
        BOOST_REQUIRE_EQUAL(ms["report"], "");
        BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //get updated proposal
        prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested + ms["requested"].as<asset>());

        //======================== remove milestone ========================

        //push rmvmilestone trx
        works_rmvmilestone(prop_name, testa);
        produce_blocks();

        //TODO: check for milestone removal
        // BOOST_REQUIRE(ms.is_null());

        //======================== edit milestone amount ========================

        //initialize
        uint64_t edited_ms = 2;

        //edit milestone 2
        works_editms(prop_name, edited_ms, asset::from_string("500.0000 TLOS"), testa);
        produce_blocks();

        //get edited milestone and proposal
        ms = get_works_milestone(prop_name, edited_ms);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), edited_ms);
        BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
        BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), new_ms_requested);
        BOOST_REQUIRE_EQUAL(ms["report"], "");
        BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //get proposal
        prop = get_works_proposal(prop_name);

        //initialize
        asset new_total_requested = total_requested - per_milestone + ms["requested"].as<asset>();

        //assert updated table values
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), new_total_requested);

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        prop = get_works_proposal(prop_name);
        fc::variant conf = get_works_config();
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_works_acct["balance"].as<asset>();
        asset proposal_fee = asset(prop["total_requested"].as<asset>().get_amount() * conf["fee_percent"].as<double>() / 100, tlos_sym);
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //TODO: pull from decide config

        //enforce min fee
        if (proposal_fee < conf["min_fee"].as<asset>()) {
            proposal_fee = conf["min_fee"].as<asset>();
        }

        //calculate
        asset new_proposer_balance = testa_works_acct["balance"].as<asset>() - proposal_fee - decide_ballot_fee;

        //launch proposal trx
        works_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), proposal_fee);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), asset::from_string("1300.0000 TLOS"));

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), new_proposer_balance);



    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_complete, works_tester ) try {
        
        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym); //400 TLOS per
        name ms1_ballot_name = prop_name;

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms1_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        prop = get_works_proposal(prop_name);
        fc::variant conf = get_works_config();
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_works_acct["balance"].as<asset>();
        asset proposal_fee = asset(prop["total_requested"].as<asset>().get_amount() * conf["fee_percent"].as<double>() / 100, tlos_sym);
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //TODO: pull from decide config

        //enforce min fee
        if (proposal_fee < conf["min_fee"].as<asset>()) {
            proposal_fee = conf["min_fee"].as<asset>();
        }

        //calculate
        asset testa_running_balance = testa_works_acct["balance"].as<asset>() - proposal_fee - decide_ballot_fee;

        //launch proposal trx
        works_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), proposal_fee);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== vote on ms 1 ========================

        //initialize
        name yes_name = name("yes");
        vector<name> yes_vote = { yes_name };

        //cast votes
        cast_vote(testa, prop_name, yes_vote);
        cast_vote(testb, prop_name, yes_vote);
        cast_vote(testc, prop_name, yes_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms1_vote = get_vote(ms1_ballot_name, testa);
        fc::variant testb_ms1_vote = get_vote(ms1_ballot_name, testb);
        fc::variant testc_ms1_vote = get_vote(ms1_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms1_weighted_votes_map = variant_to_map<name, asset>(testa_ms1_vote["weighted_votes"]);
        map<name, asset> testb_ms1_weighted_votes_map = variant_to_map<name, asset>(testb_ms1_vote["weighted_votes"]);
        map<name, asset> testc_ms1_weighted_votes_map = variant_to_map<name, asset>(testc_ms1_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms1_weighted_votes_map[yes_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms1_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms1_weighted_votes_map[yes_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 1 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        fc::variant ms1 = get_works_milestone(prop_name, 1);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        map<name, asset> ms1_results = variant_to_map<name, asset>(ms1["ballot_results"]);
        testa_running_balance += proposal_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("60.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), true);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(ms1["status"].as<name>(), name("passed"));
        BOOST_REQUIRE_EQUAL(ms1["report"], "");
        BOOST_REQUIRE_EQUAL(ms1["ballot_name"].as<name>(), prop_name);
        validate_map(ms1_results, name("yes"), asset::from_string("560.0000 VOTE"));
        validate_map(ms1_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(ms1_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance); //refund

        //======================== submit ms 1 report ========================

        //initialize
        string ms1_report = "Milestone 1 report ...";

        //send submitreport trx
        works_submitreport(prop_name, ms1_report, testa);
        produce_blocks();

        //get tables
        ms1 = get_works_milestone(prop_name, 1);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms1["report"], ms1_report);

        //======================== claim ms 1 funds ========================

        //initialize
        testa_running_balance += per_milestone;

        //send claimfunds trx
        works_claimfunds(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("800.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== next ms ========================

        //initialize
        name ms2_ballot_name = name("prop1ms2");

        //send nextms trx
        works_nextms(prop_name, ms2_ballot_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms1 = get_works_milestone(prop_name, 1);
        fc::variant ms2 = get_works_milestone(prop_name, 2);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        map<name, asset> blank_results = variant_to_map<name, asset>(ms2["ballot_results"]);
        testa_running_balance -= decide_ballot_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms2_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 2);

        BOOST_REQUIRE_EQUAL(ms1["status"].as<name>(), name("paid"));

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("voting"));
        BOOST_REQUIRE_EQUAL(ms2["requested"].as<asset>(), per_milestone);
        BOOST_REQUIRE_EQUAL(ms2["report"], "");
        BOOST_REQUIRE_EQUAL(ms2["ballot_name"].as<name>(), ms2_ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== vote on ms 2 ========================

        //cast votes
        cast_vote(testa, ms2_ballot_name, yes_vote);
        cast_vote(testb, ms2_ballot_name, yes_vote);
        cast_vote(testc, ms2_ballot_name, yes_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms2_vote = get_vote(ms2_ballot_name, testa);
        fc::variant testb_ms2_vote = get_vote(ms2_ballot_name, testb);
        fc::variant testc_ms2_vote = get_vote(ms2_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms2_weighted_votes_map = variant_to_map<name, asset>(testa_ms2_vote["weighted_votes"]);
        map<name, asset> testb_ms2_weighted_votes_map = variant_to_map<name, asset>(testb_ms2_vote["weighted_votes"]);
        map<name, asset> testc_ms2_weighted_votes_map = variant_to_map<name, asset>(testc_ms2_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms2_weighted_votes_map[yes_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms2_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms2_weighted_votes_map[yes_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 2 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms2 = get_works_milestone(prop_name, 2);

        //initialize
        map<name, asset> ms2_results = variant_to_map<name, asset>(ms2["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms2_ballot_name);

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("passed"));
        BOOST_REQUIRE_EQUAL(ms2["ballot_name"].as<name>(), ms2_ballot_name);
        validate_map(ms2_results, name("yes"), asset::from_string("560.0000 VOTE"));
        validate_map(ms2_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(ms2_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== submit ms 2 report ========================

        //initialize
        string ms2_report = "Milestone 2 report ...";

        //send submitreport trx
        works_submitreport(prop_name, ms2_report, testa);
        produce_blocks();

        //get tables
        ms2 = get_works_milestone(prop_name, 2);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms2["report"], ms2_report);

        //======================== claim ms 2 funds ========================

        //initialize
        testa_running_balance += per_milestone;

        //send claimfunds trx
        works_claimfunds(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("400.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== next ms ========================

        //initialize
        name ms3_ballot_name = name("prop1ms3");

        //send nextms trx
        works_nextms(prop_name, ms3_ballot_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms2 = get_works_milestone(prop_name, 2);
        fc::variant ms3 = get_works_milestone(prop_name, 3);

        //initialize
        blank_results = variant_to_map<name, asset>(ms3["ballot_results"]);
        testa_running_balance -= decide_ballot_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms3_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 3);

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("paid"));

        BOOST_REQUIRE_EQUAL(ms3["status"].as<name>(), name("voting"));
        BOOST_REQUIRE_EQUAL(ms3["requested"].as<asset>(), per_milestone);
        BOOST_REQUIRE_EQUAL(ms3["report"], "");
        BOOST_REQUIRE_EQUAL(ms3["ballot_name"].as<name>(), ms3_ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== vote on ms 3 ========================

        //cast votes
        cast_vote(testa, ms3_ballot_name, yes_vote);
        cast_vote(testb, ms3_ballot_name, yes_vote);
        cast_vote(testc, ms3_ballot_name, yes_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms3_vote = get_vote(ms3_ballot_name, testa);
        fc::variant testb_ms3_vote = get_vote(ms3_ballot_name, testb);
        fc::variant testc_ms3_vote = get_vote(ms3_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms3_weighted_votes_map = variant_to_map<name, asset>(testa_ms3_vote["weighted_votes"]);
        map<name, asset> testb_ms3_weighted_votes_map = variant_to_map<name, asset>(testb_ms3_vote["weighted_votes"]);
        map<name, asset> testc_ms3_weighted_votes_map = variant_to_map<name, asset>(testc_ms3_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms3_weighted_votes_map[yes_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms3_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms3_weighted_votes_map[yes_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 3 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms3 = get_works_milestone(prop_name, 3);

        //initialize
        map<name, asset> ms3_results = variant_to_map<name, asset>(ms3["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms3_ballot_name);

        BOOST_REQUIRE_EQUAL(ms3["status"].as<name>(), name("passed"));
        BOOST_REQUIRE_EQUAL(ms3["ballot_name"].as<name>(), ms3_ballot_name);
        validate_map(ms3_results, name("yes"), asset::from_string("560.0000 VOTE"));
        validate_map(ms3_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(ms3_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== submit ms 3 report ========================

        //initialize
        string ms3_report = "Milestone 3 report ...";

        //send submitreport trx
        works_submitreport(prop_name, ms3_report, testa);
        produce_blocks();

        //get tables
        ms3 = get_works_milestone(prop_name, 3);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms3["report"], ms3_report);

        //======================== claim ms 3 funds ========================

        //initialize
        testa_running_balance += per_milestone;

        //send claimfunds trx
        works_claimfunds(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("completed"));
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== delete completed proposal ========================

        //send deleteprop trx
        works_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_fail, works_tester ) try {
        
        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym); //400 TLOS per
        name ms1_ballot_name = prop_name;

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms1_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        prop = get_works_proposal(prop_name);
        fc::variant conf = get_works_config();
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_works_acct["balance"].as<asset>();
        asset proposal_fee = asset(prop["total_requested"].as<asset>().get_amount() * conf["fee_percent"].as<double>() / 100, tlos_sym);
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //TODO: pull from decide config

        //enforce min fee
        if (proposal_fee < conf["min_fee"].as<asset>()) {
            proposal_fee = conf["min_fee"].as<asset>();
        }

        //calculate
        asset testa_running_balance = testa_works_acct["balance"].as<asset>() - proposal_fee - decide_ballot_fee;

        //launch proposal trx
        works_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), proposal_fee);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== vote on ms 1 ========================

        //initialize
        name yes_name = name("yes");
        vector<name> yes_vote = { yes_name };
        name no_name = name("no");
        vector<name> no_vote = { no_name };

        //cast votes
        cast_vote(testa, prop_name, no_vote);
        cast_vote(testb, prop_name, yes_vote);
        cast_vote(testc, prop_name, no_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms1_vote = get_vote(ms1_ballot_name, testa);
        fc::variant testb_ms1_vote = get_vote(ms1_ballot_name, testb);
        fc::variant testc_ms1_vote = get_vote(ms1_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms1_weighted_votes_map = variant_to_map<name, asset>(testa_ms1_vote["weighted_votes"]);
        map<name, asset> testb_ms1_weighted_votes_map = variant_to_map<name, asset>(testb_ms1_vote["weighted_votes"]);
        map<name, asset> testc_ms1_weighted_votes_map = variant_to_map<name, asset>(testc_ms1_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms1_weighted_votes_map[no_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms1_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms1_weighted_votes_map[no_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 1 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        fc::variant ms1 = get_works_milestone(prop_name, 1);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        map<name, asset> ms1_results = variant_to_map<name, asset>(ms1["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("failed"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("60.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(ms1["status"].as<name>(), name("failed"));
        BOOST_REQUIRE_EQUAL(ms1["report"], "");
        BOOST_REQUIRE_EQUAL(ms1["ballot_name"].as<name>(), prop_name);
        validate_map(ms1_results, name("yes"), asset::from_string("70.0000 VOTE"));
        validate_map(ms1_results, name("no"), asset::from_string("490.0000 VOTE"));
        validate_map(ms1_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance); //no refund

        //attempt exceptions
        BOOST_REQUIRE_EXCEPTION(works_nextms(prop_name, name("prop1ms2"), testa),
            eosio_assert_message_exception, eosio_assert_message_is( "cannot advance a failed proposal" )
        );

        //======================== delete failed proposal ========================

        //send deleteprop trx
        works_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_double_ms_fail, works_tester ) try {
        
        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym); //400 TLOS per
        name ms1_ballot_name = prop_name;

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms1_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        prop = get_works_proposal(prop_name);
        fc::variant conf = get_works_config();
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_works_acct["balance"].as<asset>();
        asset proposal_fee = asset(prop["total_requested"].as<asset>().get_amount() * conf["fee_percent"].as<double>() / 100, tlos_sym);
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //TODO: pull from decide config

        //enforce min fee
        if (proposal_fee < conf["min_fee"].as<asset>()) {
            proposal_fee = conf["min_fee"].as<asset>();
        }

        //calculate
        asset testa_running_balance = testa_works_acct["balance"].as<asset>() - proposal_fee - decide_ballot_fee;

        //launch proposal trx
        works_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), proposal_fee);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== vote on ms 1 ========================

        //initialize
        name yes_name = name("yes");
        vector<name> yes_vote = { yes_name };

        //cast votes
        cast_vote(testa, prop_name, yes_vote);
        cast_vote(testb, prop_name, yes_vote);
        cast_vote(testc, prop_name, yes_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms1_vote = get_vote(ms1_ballot_name, testa);
        fc::variant testb_ms1_vote = get_vote(ms1_ballot_name, testb);
        fc::variant testc_ms1_vote = get_vote(ms1_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms1_weighted_votes_map = variant_to_map<name, asset>(testa_ms1_vote["weighted_votes"]);
        map<name, asset> testb_ms1_weighted_votes_map = variant_to_map<name, asset>(testb_ms1_vote["weighted_votes"]);
        map<name, asset> testc_ms1_weighted_votes_map = variant_to_map<name, asset>(testc_ms1_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms1_weighted_votes_map[yes_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms1_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms1_weighted_votes_map[yes_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 1 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        fc::variant ms1 = get_works_milestone(prop_name, 1);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        map<name, asset> ms1_results = variant_to_map<name, asset>(ms1["ballot_results"]);
        testa_running_balance += proposal_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("60.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), true);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(ms1["status"].as<name>(), name("passed"));
        BOOST_REQUIRE_EQUAL(ms1["report"], "");
        BOOST_REQUIRE_EQUAL(ms1["ballot_name"].as<name>(), prop_name);
        validate_map(ms1_results, name("yes"), asset::from_string("560.0000 VOTE"));
        validate_map(ms1_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(ms1_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance); //refund

        //======================== submit ms 1 report ========================

        //initialize
        string ms1_report = "Milestone 1 report ...";

        //send submitreport trx
        works_submitreport(prop_name, ms1_report, testa);
        produce_blocks();

        //get tables
        ms1 = get_works_milestone(prop_name, 1);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms1["report"], ms1_report);

        //======================== claim ms 1 funds ========================

        //initialize
        testa_running_balance += per_milestone;

        //send claimfunds trx
        works_claimfunds(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("800.0000 TLOS"));

        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== next ms ========================

        //initialize
        name ms2_ballot_name = name("prop1ms2");

        //send nextms trx
        works_nextms(prop_name, ms2_ballot_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms1 = get_works_milestone(prop_name, 1);
        fc::variant ms2 = get_works_milestone(prop_name, 2);

        //initialize
        map<name, asset> blank_results = variant_to_map<name, asset>(ms2["ballot_results"]);
        testa_running_balance -= decide_ballot_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms2_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 2);

        BOOST_REQUIRE_EQUAL(ms1["status"].as<name>(), name("paid"));

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("voting"));
        BOOST_REQUIRE_EQUAL(ms2["requested"].as<asset>(), per_milestone);
        BOOST_REQUIRE_EQUAL(ms2["report"], "");
        BOOST_REQUIRE_EQUAL(ms2["ballot_name"].as<name>(), ms2_ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== vote on ms 2 ========================

        //initialize
        name no_name = name("no");
        vector<name> no_vote = { no_name };

        //cast votes
        cast_vote(testa, ms2_ballot_name, no_vote);
        cast_vote(testb, ms2_ballot_name, yes_vote);
        cast_vote(testc, ms2_ballot_name, no_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms2_vote = get_vote(ms2_ballot_name, testa);
        fc::variant testb_ms2_vote = get_vote(ms2_ballot_name, testb);
        fc::variant testc_ms2_vote = get_vote(ms2_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms2_weighted_votes_map = variant_to_map<name, asset>(testa_ms2_vote["weighted_votes"]);
        map<name, asset> testb_ms2_weighted_votes_map = variant_to_map<name, asset>(testb_ms2_vote["weighted_votes"]);
        map<name, asset> testc_ms2_weighted_votes_map = variant_to_map<name, asset>(testc_ms2_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms2_weighted_votes_map[no_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms2_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms2_weighted_votes_map[no_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 2 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms2 = get_works_milestone(prop_name, 2);

        //initialize
        map<name, asset> ms2_results = variant_to_map<name, asset>(ms2["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms2_ballot_name);

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("failed"));
        BOOST_REQUIRE_EQUAL(ms2["ballot_name"].as<name>(), ms2_ballot_name);
        validate_map(ms2_results, name("yes"), asset::from_string("70.0000 VOTE"));
        validate_map(ms2_results, name("no"), asset::from_string("490.0000 VOTE"));
        validate_map(ms2_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== submit ms 2 report ========================

        //initialize
        string ms2_report = "Milestone 2 report ...";

        //send submitreport trx
        works_submitreport(prop_name, ms2_report, testa);
        produce_blocks();

        //get tables
        ms2 = get_works_milestone(prop_name, 2);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms2["report"], ms2_report);

        //attempt exceptions
        BOOST_REQUIRE_EXCEPTION(works_claimfunds(prop_name, testa),
            eosio_assert_message_exception, eosio_assert_message_is( "milestone must be passed to claim funds" )
        );

        //======================== next ms ========================

        //initialize
        name ms3_ballot_name = name("prop1ms3");

        //send nextms trx
        works_nextms(prop_name, ms3_ballot_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms2 = get_works_milestone(prop_name, 2);
        fc::variant ms3 = get_works_milestone(prop_name, 3);

        //initialize
        blank_results = variant_to_map<name, asset>(ms3["ballot_results"]);
        testa_running_balance -= decide_ballot_fee;

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms3_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 3);

        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("failed"));

        BOOST_REQUIRE_EQUAL(ms3["status"].as<name>(), name("voting"));
        BOOST_REQUIRE_EQUAL(ms3["requested"].as<asset>(), per_milestone);
        BOOST_REQUIRE_EQUAL(ms3["report"], "");
        BOOST_REQUIRE_EQUAL(ms3["ballot_name"].as<name>(), ms3_ballot_name);
        validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
        validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        //======================== vote on ms 3 ========================

        //cast votes
        cast_vote(testa, ms3_ballot_name, no_vote);
        cast_vote(testb, ms3_ballot_name, yes_vote);
        cast_vote(testc, ms3_ballot_name, no_vote);
        produce_blocks();

        //get vote tables
        fc::variant testa_ms3_vote = get_vote(ms3_ballot_name, testa);
        fc::variant testb_ms3_vote = get_vote(ms3_ballot_name, testb);
        fc::variant testc_ms3_vote = get_vote(ms3_ballot_name, testc);

        //initialize
        map<name, asset> testa_ms3_weighted_votes_map = variant_to_map<name, asset>(testa_ms3_vote["weighted_votes"]);
        map<name, asset> testb_ms3_weighted_votes_map = variant_to_map<name, asset>(testb_ms3_vote["weighted_votes"]);
        map<name, asset> testc_ms3_weighted_votes_map = variant_to_map<name, asset>(testc_ms3_vote["weighted_votes"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_ms3_weighted_votes_map[no_name], asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_ms3_weighted_votes_map[yes_name], asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_ms3_weighted_votes_map[no_name], asset::from_string("370.0000 VOTE"));

        //======================== close ms 3 ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(29));
        produce_blocks();

        //close milestone
        works_closems(prop_name, testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);
        ms2 = get_works_milestone(prop_name, 2);
        ms3 = get_works_milestone(prop_name, 3);

        //initialize
        map<name, asset> ms3_results = variant_to_map<name, asset>(ms3["ballot_results"]);

        //assert table values
        BOOST_REQUIRE_EQUAL(ms2["status"].as<name>(), name("failed"));

        BOOST_REQUIRE_EQUAL(ms3["status"].as<name>(), name("failed"));
        BOOST_REQUIRE_EQUAL(ms3["ballot_name"].as<name>(), ms3_ballot_name);
        validate_map(ms3_results, name("yes"), asset::from_string("70.0000 VOTE"));
        validate_map(ms3_results, name("no"), asset::from_string("490.0000 VOTE"));
        validate_map(ms3_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("failed"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms3_ballot_name);

        //======================== delete failed proposal ========================

        //send deleteprop trx
        works_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_draft_delete, works_tester ) try {
        
        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym); //400 TLOS per
        name ms1_ballot_name = prop_name;

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms1_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== delete drafting proposal ========================

        //send deleteprop trx
        works_deleteprop(prop_name, testa);
        produce_blocks();

    } FC_LOG_AND_RETHROW()

    BOOST_FIXTURE_TEST_CASE( proposal_cancel, works_tester ) try {
        
        //======================== fund works account ========================
        
        //send transfer to works
        base_tester::transfer(testa, works_name, "300.0000 TLOS", "", token_name);
        produce_blocks();

        //get account tables
        fc::variant testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), asset::from_string("300.0000 TLOS"));

        //======================== draft proposal ========================

        //initialize
        string title = "Proposal 1";
        string desc = "Telos Works Proposal 1";
        string content = "";
        name prop_name = name("worksprop1");
        name category = name("apps");
        asset total_requested = asset::from_string("1200.0000 TLOS");
        uint16_t milestones = 3;
        asset per_milestone = asset(total_requested.get_amount() / milestones, tlos_sym); //400 TLOS per
        name ms1_ballot_name = prop_name;

        //push draftprop trx
        works_draftprop(title, desc, content, prop_name, testa, category, total_requested, milestones);
        produce_blocks();

        //get proposal
        fc::variant prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["title"], title);
        BOOST_REQUIRE_EQUAL(prop["description"], desc);
        BOOST_REQUIRE_EQUAL(prop["content"], content);
        BOOST_REQUIRE_EQUAL(prop["proposal_name"].as<name>(), prop_name);
        BOOST_REQUIRE_EQUAL(prop["proposer"].as<name>(), testa);
        BOOST_REQUIRE_EQUAL(prop["category"].as<name>(), category);
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("drafting"));
        BOOST_REQUIRE_EQUAL(prop["current_ballot"].as<name>(), ms1_ballot_name);
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["refunded"].as<bool>(), false);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), total_requested);
        BOOST_REQUIRE_EQUAL(prop["remaining"].as<asset>(), asset::from_string("0.0000 TLOS"));
        BOOST_REQUIRE_EQUAL(prop["milestones"].as<uint16_t>(), milestones);
        BOOST_REQUIRE_EQUAL(prop["current_milestone"].as<uint16_t>(), 1);

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["milestone_id"].as<uint64_t>(), i);
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("queued"));
            BOOST_REQUIRE_EQUAL(ms["requested"].as<asset>(), per_milestone);
            BOOST_REQUIRE_EQUAL(ms["report"], "");
            BOOST_REQUIRE_EQUAL(ms["ballot_name"].as<name>(), ballot_name);
            validate_map(blank_results, name("yes"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("no"), asset::from_string("0.0000 VOTE"));
            validate_map(blank_results, name("abstain"), asset::from_string("0.0000 VOTE"));

        }

        //======================== launch proposal ========================

        //get proposal, config, and proposer account
        prop = get_works_proposal(prop_name);
        fc::variant conf = get_works_config();
        testa_works_acct = get_works_account(testa, tlos_sym);

        //initialize
        asset old_balance = testa_works_acct["balance"].as<asset>();
        asset proposal_fee = asset(prop["total_requested"].as<asset>().get_amount() * conf["fee_percent"].as<double>() / 100, tlos_sym);
        asset decide_ballot_fee = asset::from_string("10.0000 TLOS"); //TODO: pull from decide config

        //enforce min fee
        if (proposal_fee < conf["min_fee"].as<asset>()) {
            proposal_fee = conf["min_fee"].as<asset>();
        }

        //calculate
        asset testa_running_balance = testa_works_acct["balance"].as<asset>() - proposal_fee - decide_ballot_fee;

        //launch proposal trx
        works_launchprop(prop_name, testa);
        produce_blocks();

        //get new tables
        prop = get_works_proposal(prop_name);
        testa_works_acct = get_works_account(testa, tlos_sym);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("inprogress"));
        BOOST_REQUIRE_EQUAL(prop["fee"].as<asset>(), proposal_fee);
        BOOST_REQUIRE_EQUAL(prop["total_requested"].as<asset>(), asset::from_string("1200.0000 TLOS"));

        //assert table values
        BOOST_REQUIRE_EQUAL(testa_works_acct["balance"].as<asset>(), testa_running_balance);

        //======================== cancel open proposal ========================

        //advance time
        produce_blocks();
        produce_block(fc::days(5));
        produce_blocks();

        //send cancelprop trx
        works_cancelprop(prop_name, "cancel works proposal", testa);
        produce_blocks();

        //get tables
        prop = get_works_proposal(prop_name);

        //assert table values
        BOOST_REQUIRE_EQUAL(prop["status"].as<name>(), name("cancelled"));

        //check each milestone
        for (int i = 1; i <= milestones; i++) {

            //get milestone
            fc::variant ms = get_works_milestone(prop_name, i);

            //initialize
            name ballot_name = name(0);
            map<name, asset> blank_results = variant_to_map<name, asset>(ms["ballot_results"]);

            if (i == 1) {
                ballot_name = prop_name;
            }

            //assert table values
            BOOST_REQUIRE_EQUAL(ms["status"].as<name>(), name("failed"));

        }
        
    } FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
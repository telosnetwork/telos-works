#include <decide_tester.hpp>

#include <contracts.hpp>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
using namespace decidetesting::testing;

using mvo = fc::mutable_variant_object;

class works_tester : public decide_tester {
    
    public:
    
    abi_serializer works_abi_ser;

    const name works_name = name("works.decide"); // NOTE: this name may not be perm yet

    works_tester(setup_mode mode = setup_mode::full): decide_tester(mode) {

        //setup telos decide
        // set_config("v2.0.0", true);
        // produce_blocks();
        
        //initialize
        asset max_vote_supply = asset::from_string("1000000000.0000 VOTE");
        asset ram_amount = asset::from_string("400.0000 TLOS");
        asset liquid_amount = asset::from_string("10000.0000 TLOS");

        //create accounts
        create_account_with_resources(works_name, eosio_name, ram_amount, false);
        
        produce_blocks();
        
        //setcode, setabi, init
        setup_works_contract();
        produce_blocks();

        //create VOTE treasury
        base_tester::transfer(eosio_name, decide_name, "3000.0000 TLOS", "", token_name);
        new_treasury(eosio_name, max_vote_supply, name("public"));
        produce_blocks();

        //register works as voter on telos decide
        reg_voter(works_name, max_vote_supply.get_symbol(), {});
        produce_blocks();

        //delegate bw for test accounts
        delegate_bw(testa, testa, asset::from_string("10.0000 TLOS"), asset::from_string("90.0000 TLOS"), false);
        delegate_bw(testb, testb, asset::from_string("10.0000 TLOS"), asset::from_string("40.0000 TLOS"), false);
        delegate_bw(testc, testc, asset::from_string("10.0000 TLOS"), asset::from_string("340.0000 TLOS"), false);
        produce_blocks();

        //register voters on telos decide
        reg_voter(testa, max_vote_supply.get_symbol(), {});
        reg_voter(testb, max_vote_supply.get_symbol(), {});
        reg_voter(testc, max_vote_supply.get_symbol(), {});
        produce_blocks();

        //get voter balances
        fc::variant testa_voter = get_voter(testa, vote_sym);
        fc::variant testb_voter = get_voter(testb, vote_sym);
        fc::variant testc_voter = get_voter(testc, vote_sym);

        //assert VOTE balances
        BOOST_REQUIRE_EQUAL(testa_voter["staked"].as<asset>(), asset::from_string("120.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testb_voter["staked"].as<asset>(), asset::from_string("70.0000 VOTE"));
        BOOST_REQUIRE_EQUAL(testc_voter["staked"].as<asset>(), asset::from_string("370.0000 VOTE"));

        //fund telos works
        base_tester::transfer(eosio_name, works_name, "100000.0000 TLOS", "fund", token_name);
        produce_blocks();

    }

    //======================== setup functions ========================

    void setup_works_contract() {

        //setcode, setabi
        set_code( works_name, contracts::works_wasm());
        set_abi( works_name, contracts::works_abi().data() );
        {
            const auto& accnt = control->db().get<account_object,by_name>( works_name );
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            works_abi_ser.set_abi(abi, abi_serializer_max_time);
        }

        //push init action to works
        works_init("Telos Works", "v2.0.0", works_name);
        produce_blocks();

        //initialize new auth
        authority auth = authority( get_public_key(works_name, "active") );
        auth.accounts.push_back( permission_level_weight{ {works_name, config::eosio_code_name}, static_cast<weight_type>(auth.threshold) } );

        //set eosio.code
        set_authority(works_name, name("active"), auth, name("owner"));
        produce_blocks();
    }

    //======================== works actions ===========================

    transaction_trace_ptr works_init(string app_name, string app_version, name initial_admin) {
        signed_transaction trx;
        vector<permission_level> permissions { { works_name, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("init"), permissions, 
            mvo()
                ("app_name", app_name)
                ("app_version", app_version)
                ("initial_admin", initial_admin)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(works_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_setversion(string new_version, name admin_name) {
        signed_transaction trx;
        vector<permission_level> permissions { { admin_name, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("setversion"), permissions, 
            mvo()
                ("new_version", new_version)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(admin_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_setadmin(name new_admin, name old_admin) {
        signed_transaction trx;
        vector<permission_level> permissions { { old_admin, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("setadmin"), permissions, 
            mvo()
                ("new_admin", new_admin)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(old_admin, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_draftprop(string title, string desc, string content, name proposal_name, 
        name proposer, name category, asset total_requested, uint16_t milestones) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("draftprop"), permissions, 
            mvo()
                ("title", title)
                ("description", desc)
                ("content", content)
                ("proposal_name", proposal_name)
                ("proposer", proposer)
                ("category", category)
                ("total_requested", total_requested)
                ("milestones", milestones)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_launchprop(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("launchprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_cancelprop(name proposal_name, string memo, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("cancelprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("memo", memo)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_deleteprop(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("deleteprop"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_addmilestone(name proposal_name, asset requested, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("addmilestone"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("requested", requested)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_rmvmilestone(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("rmvmilestone"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_editms(name proposal_name, uint64_t milestone_id, asset new_requested, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("editms"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("milestone_id", milestone_id)
                ("new_requested", new_requested)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_closems(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("closems"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_submitreport(name proposal_name, string report, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("submitreport"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("report", report)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_claimfunds(name proposal_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("claimfunds"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_nextms(name proposal_name, name ballot_name, name proposer) {
        signed_transaction trx;
        vector<permission_level> permissions { { proposer, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("nextms"), permissions, 
            mvo()
                ("proposal_name", proposal_name)
                ("ballot_name", ballot_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(proposer, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_withdraw(name account_name, asset quantity) {
        signed_transaction trx;
        vector<permission_level> permissions { { account_name, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("withdraw"), permissions, 
            mvo()
                ("account_name", account_name)
                ("quantity", quantity)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(account_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    transaction_trace_ptr works_deleteacct(name account_name) {
        signed_transaction trx;
        vector<permission_level> permissions { { account_name, name("active") } };
        trx.actions.emplace_back(get_action(works_name, name("deleteacct"), permissions, 
            mvo()
                ("account_name", account_name)
        ));
        set_transaction_headers( trx );
        trx.sign(get_private_key(account_name, "active"), control->get_chain_id());
        return push_transaction( trx );
    }

    //======================== works getters ========================

    fc::variant get_works_config() { 
        vector<char> data = get_row_by_account(works_name, works_name, name("config"), name("config"));
        return data.empty() ? fc::variant() : works_abi_ser.binary_to_variant("config", data, abi_serializer_max_time);
    }

    fc::variant get_works_account(name account_owner, symbol acct_sym) { 
        vector<char> data = get_row_by_account(works_name, account_owner, name("accounts"), acct_sym.to_symbol_code());
        return data.empty() ? fc::variant() : works_abi_ser.binary_to_variant("account", data, abi_serializer_max_time);
    }

    fc::variant get_works_proposal(name proposal_name) { 
        vector<char> data = get_row_by_account(works_name, works_name, name("proposals"), proposal_name);
        return data.empty() ? fc::variant() : works_abi_ser.binary_to_variant("proposal", data, abi_serializer_max_time);
    }

    fc::variant get_works_milestone(name proposal_name, uint64_t milestone_id) { 
        vector<char> data = get_row_by_account(works_name, proposal_name, name("milestones"), milestone_id);
        return data.empty() ? fc::variant() : works_abi_ser.binary_to_variant("milestone", data, abi_serializer_max_time);
    }

    //======================== works helpers ========================



};
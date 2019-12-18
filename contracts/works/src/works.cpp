#include "../include/example.hpp"

//======================== admin actions ========================

ACTION works::init(string app_name, string app_version, name initial_admin) {

    //authenticate
    require_auth(get_self());
    
    //open config singleton
    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    config initial_conf = {
        app_name, //app_name
        app_version, //app_version
        initial_admin, //admin
        asset(300000, TLOS_SYM), //min_fee
        asset(5000000000, TLOS_SYM), //max_requested
        uint64_t(0) //last_proposal_id
    };

    //set initial config
    configs.set(initial_conf, get_self());

}

ACTION works::setversion(string new_version) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change version
    conf.app_version = new_version;

    //set new config
    configs.set(conf, get_self());

}

ACTION works::setadmin(name new_admin) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //change version
    conf.admin = new_admin;

    //set new config
    configs.set(conf, get_self());

}

//======================== proposal actions ========================

ACTION works::draftprop(string title, string description, string content, name proposer, 
    name category, asset total_requested, optional<uint16_t> milestones) {

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    uint64_t new_proposal_id = conf.last_proposal_id + 1;
    
    asset proposal_fee = asset(int64_t(total_requested.amount * double(0.05)), TLOS_SYM);
    if (proposal_fee < conf.min_fee) {
        proposal_fee = conf.min_fee;
    }

    uint16_t milestone_count = 1;
    if (milestones) {
        milestone_count = *milestones;
    }

    asset per_milestone = asset(int64_t(per_milestone.amount / milestone_count), TLOS_SYM);

    //charge proposal fee
    require_fee(proposer, proposal_fee);

    //validate
    check(requested <= conf.max_requested, "requested amount exceeds maximum");
    check(milestone_count > 0, "milestones must be greater than 0");
    check(milestone_count <= conf.max_milestones, "too many milestones");
    check(valid_category(category), "invalid category");

    //update last proposal id, set new config
    conf.last_proposal_id = new_proposal_id;
    configs.set(conf, get_self());

    //open proposals table
    proposals_table proposals(get_self(), get_self().value;

    //emplace new proposal
    proposals.emplace(proposer, [&](auto& col) {
        col.proposal_id = new_proposal_id;
        col.proposer = proposer;
        col.category = category;
        col.status = name("draft");
        col.current_ballot = name(0);
        col.fee = proposal_fee;
        col.total_requested = total_requested;
        col.remaining = asset(0, TLOS_SYM);
        col.milestones = milestone_count;
        col.current_milestone = uint16_t(0);
    });
        
    //open milestones table
    milestones_table milestones(get_self(), get_self().value);

    //intialize
    map<name, asset> blank_results;

    //emplace milestone
    milestones.emplace(proposer, [&](auto& col) {
        col.milestone_id = uint64_t(1);
        col.ballot_name = name(0);
        col.requested = per_milestone;
        col.report = "";
        col.ballot_results = blank_results;
        col.status = name("undecided");
        col.paid = false;
    });

    //TODO?: send transfer inline from self to trailservice to pay for newballot fee?

    //intialize
    vector<name> ballot_options = { name("yes"), name("no"), name("abstain") };
    time_point_sec ballot_end_time = time_point_sec(current_time_point()) + conf.proposal_length;

    //send inline newballot
    action(permission_level{get_self(), name("active")}, name("trailservice"), name("newballot"), make_tuple(
        ballot_name, //ballot_name
        name("proposals"), //category
        get_self(), //publisher
        VOTE_SYM, //treasury_symbol
        name("1token1vote"), //voting_method
        ballot_options //initial_options
    )).send();

    //send inline editdetails
    action(permission_level{get_self(), name("active")}, name("trailservice"), name("editdetails"), make_tuple(
        ballot_name, //ballot_name
        title, //title
        description, //description
        content //content
    )).send();

    //send inline openvoting
    // action(permission_level{get_self(), name("active")}, name("trailservice"), name("openvoting"), make_tuple(
    //     ballot_name, //ballot_name
    //     ballot_end_time //end_time
    // )).send();

}

ACTION works::advanceprop(uint64_t proposal_id) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_id, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status == "voting"_n, "proposal must be funded to advance");

    

}

ACTION works::cancelprop(name proposal_id, string memo) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_id, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status == "voting"_n, "proposal must be voting to cancel");

    //modify proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("cancelled");
    });

    //send inline cancelballot
    action(permission_level{get_self(), name("active")}, name("trailservice"), name("cancelballot"), make_tuple(
        prop.current_ballot, //ballot_name
        "Telos Works Cancel Ballot" //memo
    )).send();

}

//======================== account actions ========================

ACTION works::withdraw(account_name, asset quantity) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(quantity.symbol.code().raw(), "account not found");

    //authenticate
    require_auth(acct.account_name);

    //validate
    check(quantity.symbol == TLOS_SYM, "must withdraw TLOS");
    check(quantity.amount > 0, "must withdraw positive amount");
    check(quantity.amount <= acct.balance.amount, "insufficient funds");

    //inline transfer
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        account_name, //to
        quantity, //quantity
        std::string("Telos Works Withdrawal") //memo
    )).send();

}

ACTION works::delaccount(name account_name) {
    
    //open accounts table, get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(quantity.symbol.code().raw(), "account not found");

    //authenticate
    require_auth(acct.account_name);

    //validate
    check(acct.balance.amount == 0, "account must be empty to delete");

    //delete account
    accounts.erase(acct);

}

//======================== notification functions ========================

void works::catch_transfer(name from, name to, asset quantity, string memo) {

    //get initial receiver contract
    name rec = get_first_receiver();

    //validate
    if (rec == name("eosio.token") && from != get_self() && quantity.symbol == TLOS_SYM) {
        
        //parse memo
        //skips emplacement if memo is skip
        if (memo == std::string("skip")) { 
            return;
        }
        
        //open accounts table, search for account
        accounts_table accounts(get_self(), from.value);
        auto acct = accounts.find(TLOS_SYM.code().raw());

        //emplace account if not found, update if exists
        if (acct == accounts.end()) {

            //make new account
            accounts.emplace(get_self(), [&](auto& col) {
                col.balance = quantity;
            });

        } else {

            //update existing account
            accounts.modify(*acct, same_payer, [&](auto& col) {
                col.balance += quantity;
            });

        }

    }

}

void works::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
    
    //get initial receiver contract
    name rec = get_first_receiver();

    if (rec == name("trailservice")) {
        
        //open proposals table, get by ballot index, find prop
        proposals_table proposals(get_self(), get_self().value);
        auto props_by_ballot = proposals.get_index<"byballot"_n>();
        auto by_ballot_itr = props_by_ballot.lower_bound(ballot_name.value);

        if (by_ballot_itr != props_by_ballot.end()) {

            //initialize
            uint64_t prop_id = by_ballot_itr->proposal_id;
            asset most_votes = asset(0, VOTE_SYM);
            name winner = name(0);

            for (auto res_itr = final_results.begin(); res_itr != final_results.end(); res_itr++) {
                
                if (final_results[res_itr->first] > most_votes) {
                    most_votes = res_itr->second;
                }

            }

            //TODO: quorum check
            if (most_votes == "yes"_n) {
                winner = name("passed");
            } else {
                winner = name("failed");
            }
            
            //open milestones table, find milestone
            milestones_table milestones(get_self(), );
            auto ms_itr = milestones.find(uint64_t(by_ballot_itr->current_milestone));

            if (ms_itr != milestones.end()) {

                //update milestone
                milestones.modify(*ms_itr, same_payer, [&](auto& col) {
                    col.ballot_results = final_results;
                    col.status = winner;
                });

            }

        }

    }

}

//======================== functions ========================

void works::require_fee(name account_owner, asset fee) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "require_fee: account not found");

    //validate
    check(acct.balance >= fee, "insufficient funds");

    //charge fee
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= fee;
    });

}

bool works::valid_category(name category) {

    //check category
    switch (category.value) {
        case (name("marketing").value):
            return true;
        case (name("apps").value):
            return true;
        case (name("developers").value):
            return true;
        case (name("education").value):
            return true;
        default:
            return false;
    }

}

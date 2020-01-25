#include "../include/works.hpp"

using namespace decidespace;

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
        asset(0, TLOS_SYM), //available_funds
        asset(0, TLOS_SYM), //reserved_funds
        asset(0, TLOS_SYM), //deposited_funds
        asset(0, TLOS_SYM), //paid_funds
        double(5.0), //quorum_threshold
        double(50.0), //yes_threshold
        double(3.0), //quorum_refund_threshold
        double(35.0), //yes_refund_threshold
        asset(300000, TLOS_SYM), //min_fee
        double(5.0), //fee_percent
        uint16_t(1), //min_milestones
        uint16_t(12), //max_milestones
        uint32_t(2505600), //milestone_length
        asset(10000000, TLOS_SYM), //min_requested
        asset(5000000000, TLOS_SYM) //max_requested
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

ACTION works::draftprop(string title, string description, string content, name proposal_name, name proposer, name category, asset total_requested, optional<uint16_t> milestones) {

    //authenticate
    require_auth(proposer);

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    uint16_t milestone_count = 1;

    if (milestones) {
        milestone_count = *milestones;
    }

    asset per_milestone = asset(int64_t(total_requested.amount / milestone_count), TLOS_SYM);

    //validate
    check(total_requested <= conf.max_requested, "requested amount exceeds allowed maximum");
    check(milestone_count > 0, "milestones must be greater than 0");
    check(milestone_count <= conf.max_milestones, "too many milestones");
    check(valid_category(category), "invalid works category");

    //open proposals table
    proposals_table proposals(get_self(), get_self().value);

    //emplace new proposal
    proposals.emplace(proposer, [&](auto& col) {
        col.title = title;
        col.description = description;
        col.content = content;
        col.proposal_name = proposal_name;
        col.proposer = proposer;
        col.category = category;
        col.status = name("drafting");
        col.current_ballot = proposal_name;
        col.fee = asset(0, TLOS_SYM);
        col.refunded = false;
        col.total_requested = total_requested;
        col.remaining = asset(0, TLOS_SYM);
        col.milestones = milestone_count;
        col.current_milestone = uint16_t(1);
    });

    //initialize
    map<name, asset> blank_results;
    blank_results["yes"_n] = asset(0, VOTE_SYM);
    blank_results["no"_n] = asset(0, VOTE_SYM);
    blank_results["abstain"_n] = asset(0, VOTE_SYM);

    //emplace each milestone
    for (uint16_t i = 1; i <= milestone_count; i++) {

        //open milestones table
        milestones_table milestones(get_self(), proposal_name.value);

        //initialize
        name ms_ballot_name = name(0);

        if (i == 1) {
            ms_ballot_name = proposal_name;
        }

        //emplace milestone
        milestones.emplace(proposer, [&](auto& col) {
            col.milestone_id = uint64_t(i);
            col.status = name("queued");
            col.requested = per_milestone;
            col.report = "";
            col.ballot_name = ms_ballot_name;
            col.ballot_results = blank_results;
        });

    }

}

ACTION works::launchprop(name proposal_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(uint64_t(1), "milestone not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    asset proposal_fee = asset(int64_t(prop.total_requested.amount * conf.fee_percent / 100), TLOS_SYM);
    asset newballot_fee = asset(300000, TLOS_SYM); //TODO: get from telos.decide config table
    time_point_sec now = time_point_sec(current_time_point());
    time_point_sec ballot_end_time = now + conf.milestone_length;
    vector<name> ballot_options = { name("yes"), name("no"), name("abstain") };

    if (proposal_fee < conf.min_fee) {
        proposal_fee = conf.min_fee;
    }

    //charge proposal fee and newballot_fee
    sub_balance(prop.proposer, proposal_fee + newballot_fee);

    //validate
    check(prop.status == "drafting"_n, "proposal must be in drafting mode to launch");
    check(prop.total_requested <= conf.max_requested, "total requested is more than maximum allowed");
    check(prop.milestones <= conf.max_milestones, "milestones is more than maximum allowed");
    check(prop.current_milestone == 1, "proposal must be in its first milestone to launch");
    check(ms.status == "queued"_n, "milestone must be queued to start");
    check(conf.deposited_funds >= proposal_fee + newballot_fee, "not enough deposited funds");

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("inprogress");
        col.fee = proposal_fee;
    });

    //update milestone
    milestones.modify(ms, same_payer, [&](auto& col) {
        col.status = name("voting");
    });

    //update and set config
    conf.deposited_funds -= (proposal_fee + newballot_fee);
    configs.set(conf, get_self());

    //send transfer inline to pay for newballot fee
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        name("telos.decide"), //to
        newballot_fee, //quantity
        string("Telos Works Ballot Fee Payment") //memo
    )).send();

    //send inline newballot
    decide::newballot_action newballot_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    newballot_act.send(proposal_name, name("proposal"), get_self(), VOTE_SYM, name("1token1vote"), ballot_options);
    
    //send inline editdetails
    decide::editdetails_action editdetails_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    editdetails_act.send(proposal_name, prop.title, prop.description, prop.content);

    //toggle ballot votestake on (default is off)
    decide::togglebal_action togglebal_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    togglebal_act.send(proposal_name, name("votestake"));
    
    //send inline openvoting
    decide::openvoting_action openvoting_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    openvoting_act.send(proposal_name, ballot_end_time);

}

ACTION works::cancelprop(name proposal_name, string memo) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //validate
    check(prop.status == "inprogress"_n, "proposal must be in progress to cancel");
    
    //fail each remaining milestone
    for (auto i = prop.current_milestone; i <= prop.milestones; i++) {

        //open milestones table
        milestones_table milestones(get_self(), proposal_name.value);
        auto& ms = milestones.get(i, "milestone not found");

        //update milestone
        milestones.modify(ms, same_payer, [&](auto& col) {
            col.status = "failed"_n;
        });

    }

    //send inline cancelballot
    decide::cancelballot_action cancelballot_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    cancelballot_act.send(prop.current_ballot, string("Telos Works Cancel Ballot"));

    //modify proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = name("cancelled");
    });

}

ACTION works::deleteprop(name proposal_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //validate
    check(prop.status == "drafting"_n || prop.status == "failed"_n || prop.status == "cancelled"_n || prop.status == "completed"_n, 
        "proposal must be drafting, failed, cancelled, or completed to delete");

    //authenticate
    if (prop.status == "drafting"_n) {
        require_auth(prop.proposer);
    }

    //return remaining funds back to available funds
    if (prop.remaining.amount > 0) {

        conf.reserved_funds -= prop.remaining;
        conf.available_funds += prop.remaining;
        configs.set(conf, get_self());

    }
    
    //delete each milestone
    for (auto i = 1; i <= prop.milestones; i++) {

        //open milestones table, get milestone
        milestones_table milestones(get_self(), proposal_name.value);
        auto& ms = milestones.get(i, "milestone not found");

        //erase milestone
        milestones.erase(ms);

    }

    //delete proposal
    proposals.erase(prop);

}

//======================== milestone actions ========================

ACTION works::addmilestone(name proposal_name, asset requested) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open milestones table
    milestones_table milestones(get_self(), proposal_name.value);

    //validate
    check(prop.status == "drafting"_n, "proposal must be in drafting mode to add milestone");
    check(requested.amount > 0, "must request a positive amount");
    check(prop.total_requested + requested <= conf.max_requested, "requested funds too high");
    check(prop.milestones + 1 <= conf.max_milestones, "proposal has reached the max allowed milestones");

    //initialize
    uint64_t new_ms_id = prop.milestones + 1;
    map<name, asset> blank_results;
    blank_results["yes"_n] = asset(0, VOTE_SYM);
    blank_results["no"_n] = asset(0, VOTE_SYM);
    blank_results["abstain"_n] = asset(0, VOTE_SYM);

    //add new milestone
    milestones.emplace(prop.proposer, [&](auto& col) {
        col.milestone_id = new_ms_id;
        col.status = name("queued");
        col.requested = requested;
        col.report = "";
        col.ballot_name = name(0);
        col.ballot_results = blank_results;
    });

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.total_requested += requested;
        col.milestones += 1;
    });

}

ACTION works::rmvmilestone(name proposal_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get last milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(uint64_t(prop.milestones), "milestone not found");

    //validate
    check(prop.status == "drafting"_n, "proposal must be in drafting mode to remove milestone");
    check(prop.milestones > 1, "proposal must have at least one milestone");
    check(prop.total_requested - ms.requested > asset(0, TLOS_SYM), "total requested amount cannot be below zero");

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.total_requested -= ms.requested;
        col.milestones -= 1;
    });

    //erase milestone
    milestones.erase(ms);

}

ACTION works::editms(name proposal_name, uint64_t milestone_id, asset new_requested) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(milestone_id, "milestone not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //initialize
    asset request_delta = new_requested - ms.requested;
    asset new_total_requested = prop.total_requested + request_delta;

    //validate
    check(prop.status == "drafting"_n, "proposal must be in drafting mode to edit milestones");
    check(ms.status == "queued"_n, "milestone must be queued to edit");
    check(new_requested.amount > 0, "must request a postitive amount");
    check(new_total_requested.amount > 0 && new_total_requested <= conf.max_requested, "invalid total requested amount");

    //update milestone
    milestones.modify(ms, same_payer, [&](auto& col) {
        col.requested = new_requested;
    });

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.total_requested = new_total_requested;
    });

}

ACTION works::closems(name proposal_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(prop.current_milestone, "milestone not found");

    //validate
    check(prop.status == "inprogress"_n, "proposal must be in progress to close milestone");
    check(ms.status == "voting"_n, "milestone must be voting to close");
    check(prop.current_ballot == ms.ballot_name, "current ballot and milestone ballot mismatch");
    
    //send inline closeballot
    decide::closevoting_action closevoting_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    closevoting_act.send(ms.ballot_name, true);

}

ACTION works::submitreport(name proposal_name, string report) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(prop.current_milestone, "milestone not found");

    //validate
    check(prop.status == "inprogress"_n, "must submit report when proposal is in progress");
    check(ms.status == "voting"_n || ms.status == "passed"_n || ms.status == "failed"_n, "milestone must be voting, passed, or failed to submit report");
    check(report != "", "report cannot be empty");

    //TODO: further report validation

    //update milestone
    milestones.modify(ms, same_payer, [&](auto& col) {
        col.report = report;
    });

}

ACTION works::claimfunds(name proposal_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(prop.current_milestone, "milestone not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //validate
    check(prop.status == "inprogress"_n, "proposal must be in progress to claim funds");
    check(ms.status == "passed"_n, "milestone must be passed to claim funds");
    check(ms.report != "", "must submit report to claim funds");

    //TODO: further report validation

    //update milestone
    milestones.modify(ms, same_payer, [&](auto& col) {
        col.status = name("paid");
    });

    //initialize
    name new_prop_status = prop.status;
    asset new_remaining = prop.remaining - ms.requested;

    //if final milestone
    if (prop.current_milestone == prop.milestones) {

        //update proposal status to completed
        new_prop_status = "completed"_n;

    }

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.status = new_prop_status;
        col.remaining = new_remaining;
    });

    //update and set conf
    conf.reserved_funds -= ms.requested;
    conf.paid_funds += ms.requested;
    configs.set(conf, get_self());

    //move requresed funds to proposer account
    add_balance(prop.proposer, ms.requested);

}

ACTION works::nextms(name proposal_name, name ballot_name) {

    //open proposals table, get proposal
    proposals_table proposals(get_self(), get_self().value);
    auto& prop = proposals.get(proposal_name.value, "proposal not found");

    //authenticate
    require_auth(prop.proposer);

    //initialize
    uint64_t next_milestone = prop.current_milestone + 1;
    name last_ms_status = get_milestone_status(proposal_name, prop.current_milestone);

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& next_ms = milestones.get(next_milestone, "milestone not found");

    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //validate
    check(prop.status != "failed"_n, "cannot advance a failed proposal");
    check(prop.current_milestone < prop.milestones, "no milestones left");
    check(next_ms.status == "queued"_n, "next milestone must be queued to advance");
    check(last_ms_status == name("failed") || last_ms_status == name("paid"), "last milestone must be failed or paid to advance");

    //update proposal
    proposals.modify(prop, same_payer, [&](auto& col) {
        col.current_ballot = ballot_name;
        col.current_milestone += 1;
    });

    //update new milestone
    milestones.modify(next_ms, same_payer, [&](auto& col) {
        col.status = "voting"_n;
        col.ballot_name = ballot_name;
    });

    //initialize
    vector<name> ballot_options = { name("yes"), name("no"), name("abstain") };
    time_point_sec ballot_end_time = time_point_sec(current_time_point()) + conf.milestone_length;
    asset newballot_fee = asset(300000, TLOS_SYM); //TODO: get from telos.decide config table

    //charge telos decide newballot fee
    sub_balance(prop.proposer, asset(300000, TLOS_SYM)); //30 TLOS

    //send transfer inline to pay for newballot fee
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        name("telos.decide"), //to
        newballot_fee, //quantity
        string("Telos Works Ballot Fee Payment") //memo
    )).send();

    //send inline newballot
    decide::newballot_action newballot_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    newballot_act.send(ballot_name, name("proposal"), get_self(), VOTE_SYM, name("1token1vote"), ballot_options);

    //send inline editdetails
    decide::editdetails_action editdetails_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    editdetails_act.send(ballot_name, prop.title, prop.description, prop.content);

    //send inline togglebal
    decide::togglebal_action togglebal_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    togglebal_act.send(ballot_name, name("votestake"));

    //send inline openvoting
    decide::openvoting_action openvoting_act(DECIDE_N, { get_self(), ACTIVE_PERM_N });
    openvoting_act.send(ballot_name, ballot_end_time);

}

//======================== account actions ========================

ACTION works::withdraw(name account_name, asset quantity) {

    //authenticate
    require_auth(account_name);

    //validate
    check(quantity.symbol == TLOS_SYM, "must withdraw TLOS");
    check(quantity.amount > 0, "must withdraw positive amount");

    //subtract balance from account
    sub_balance(account_name, quantity);

    //inline transfer
    action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
        get_self(), //from
        account_name, //to
        quantity, //quantity
        std::string("Telos Works Withdrawal") //memo
    )).send();

}

ACTION works::deleteacct(name account_name) {

    //authenticate
    require_auth(account_name);
    
    //open accounts table, get account
    accounts_table accounts(get_self(), account_name.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "account not found");

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

        //open config singleton, get config
        config_singleton configs(get_self(), get_self().value);
        auto conf = configs.get();

        //adds to available funds
        if (memo == std::string("fund") || memo == std::string("Transfer worker proposal share to works.decide account")) { 
            
            //update available funds
            conf.available_funds += quantity;

            //set new config
            configs.set(conf, get_self());

        } else {

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
            conf.deposited_funds += quantity;

            configs.set(conf, get_self());
        }

    } else if (rec == "eosio.token"_n && from == get_self() && quantity.symbol == TLOS_SYM) {
        
        //open configs singleton, get config
        config_singleton configs(get_self(), get_self().value);
        auto conf = configs.get();

        //initialize
        asset self_balance = conf.deposited_funds + conf.available_funds;
        asset total_transferable = (self_balance + quantity) - conf.deposited_funds;
        
        //validate
        check(total_transferable >= quantity, "Telos Decide lacks the liquid TLOS to make this transfer");

    }

}

void works::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {

    //get initial receiver contract
    name rec = get_first_receiver();

    if (rec == name("telos.decide")) {
        
        //open proposals table, get by ballot index, find prop
        proposals_table proposals(get_self(), get_self().value);
        auto props_by_ballot = proposals.get_index<"byballot"_n>();
        auto by_ballot_itr = props_by_ballot.lower_bound(ballot_name.value);

        if (by_ballot_itr != props_by_ballot.end()) {

            //open telos decide treasury table, get treasury
            treasuries_table treasuries(name("telos.decide"), name("telos.decide").value);
            auto& trs = treasuries.get(VOTE_SYM.code().raw(), "treasury not found");

            //open milestones table, get milestone
            milestones_table milestones(get_self(), by_ballot_itr->proposal_name.value);
            auto& ms = milestones.get(uint64_t(by_ballot_itr->current_milestone), "milestone not found");

            //open config singleton, get config
            config_singleton configs(get_self(), get_self().value);
            auto conf = configs.get();

            //validate
            check(by_ballot_itr->status == name("inprogress"), "proposal must be in progress");
            check(ms.status == name("voting"), "milestone status must be voting");

            //initialize
            bool approve = false;
            bool refund = false;
            name new_ms_status = name("failed");
            name new_prop_status = name("inprogress");
            asset new_remaining = asset(0, TLOS_SYM);

            asset total_votes = final_results["yes"_n] + final_results["no"_n] + final_results["abstain"_n];
            asset non_abstain_votes = final_results["yes"_n] + final_results["no"_n];

            asset quorum_thresh = trs.supply * conf.quorum_threshold / 100;
            asset approve_thresh = non_abstain_votes * conf.yes_threshold / 100;

            asset quorum_refund_thresh = trs.supply * conf.quorum_refund_threshold / 100;
            asset approve_refund_thresh = non_abstain_votes * conf.yes_threshold / 100;

            //determine approval and refund
            if (total_votes >= quorum_thresh && final_results["yes"_n] >= approve_thresh) {
                
                approve = true;
                refund = true;
                new_ms_status = name("passed");

            } else if (total_votes >= quorum_refund_thresh && final_results["yes"_n] >= approve_refund_thresh && !refund) {
                
                refund = true;

            }
            
            //execute if first milestone
            if (by_ballot_itr->current_milestone == 1) {

                if (approve) {

                    //validate
                    check(conf.available_funds >= by_ballot_itr->total_requested, "insufficient available funds");

                    //update config funds
                    conf.available_funds -= by_ballot_itr->total_requested;
                    conf.reserved_funds += by_ballot_itr->total_requested;
                    configs.set(conf, get_self());

                    //update remaining funds
                    new_remaining = by_ballot_itr->total_requested;

                } else {

                    //update proposal status
                    new_prop_status = name("failed");

                }

                //refund if passed refund thresh, not already refunded
                if (refund && !by_ballot_itr->refunded) {

                    //give refund
                    add_balance(by_ballot_itr->proposer, by_ballot_itr->fee); //TODO: keep newballot fee amounts?

                }

            } else {

                //execute if current and last milestone both failed
                if (!approve && get_milestone_status(by_ballot_itr->proposal_name, by_ballot_itr->current_milestone - 1) == "failed"_n) {

                    //update proposal status
                    new_prop_status = name("failed");

                }

                new_remaining = by_ballot_itr->remaining;

            }

            //update proposal
            proposals.modify(*by_ballot_itr, same_payer, [&](auto& col) {
                col.status = new_prop_status;
                col.refunded = refund;
                col.remaining = new_remaining;
            });

            //update milestone
            milestones.modify(ms, same_payer, [&](auto& col) {
                col.status = new_ms_status;
                col.ballot_results = final_results;
            });

        }

    }

}

//======================== functions ========================

void works::sub_balance(name account_owner, asset quantity) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "sub_balance: account not found");

    //validate
    check(acct.balance >= quantity, "sub_balance: insufficient funds >>> needed: " + asset(quantity.amount - acct.balance.amount, TLOS_SYM).to_string());

    //subtract quantity from balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance -= quantity;
    });

    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    conf.deposited_funds -= quantity;

    configs.set(conf, get_self());
}

void works::add_balance(name account_owner, asset quantity) {

    //open accounts table, get account
    accounts_table accounts(get_self(), account_owner.value);
    auto& acct = accounts.get(TLOS_SYM.code().raw(), "add_balance: account not found");

    //add quantity to balance
    accounts.modify(acct, same_payer, [&](auto& col) {
        col.balance += quantity;
    });

    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    conf.deposited_funds += quantity;

    configs.set(conf, get_self());
}

bool works::valid_category(name category) {

    //check category name
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

name works::get_milestone_status(name proposal_name, uint64_t milestone_id) {

    //open milestones table, get milestone
    milestones_table milestones(get_self(), proposal_name.value);
    auto& ms = milestones.get(milestone_id, "get_milestone_status: milestone not found");

    return ms.status;

}

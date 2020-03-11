// Telos Works is a Worker Proposal System for the Telos Blockchain Network.
//
// @author Craig Branscom
// @contract works
// @version v0.1.0

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <decide.hpp>

using namespace std;
using namespace eosio;

//approved treasuries: VOTE

//categories: marketing, apps, developers, education

//proposal statuses: drafting, inprogress, failed, cancelled, completed

//milestones status: queued, voting, passed, failed, paid

CONTRACT works : public contract {

    public:

    works(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

    ~works() {}

    static constexpr name DECIDE_N = "telos.decide"_n; //TODO: change to telos.decide when purchased
    static constexpr name ACTIVE_PERM_N = "active"_n;
    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol VOTE_SYM = symbol("VOTE", 4);

    //======================== admin actions ========================

    //initialize the contract
    ACTION init(string app_name, string app_version, name initial_admin);

    //set contract version
    ACTION setversion(string new_version);

    //set new admin
    ACTION setadmin(name new_admin);

    //set new config thresholds
    ACTION setthresh(double new_quorum_thresh, double new_yes_thresh, double new_quorum_refund_thresh, double new_yes_refund_thresh);

    //======================== proposal actions ========================

    //draft a new community proposal
    ACTION draftprop(string title, string description, string content, name proposal_name, name proposer, name category, asset total_requested, optional<uint16_t> milestones);
    
    //launch a proposal
    ACTION launchprop(name proposal_name);

    //cancel proposal
    ACTION cancelprop(name proposal_name, string memo);

    //delete proposal
    ACTION deleteprop(name proposal_name);

    //======================== milestone actions ========================

    //add a new milestone to a proposal
    ACTION addmilestone(name proposal_name, asset requested);

    //remove a milestone from a proposal
    ACTION rmvmilestone(name proposal_name);

    //edit a milestone
    ACTION editms(name proposal_name, uint64_t milestone_id, asset new_requested);

    //close milestone voting
    ACTION closems(name proposal_name);

    //submit a milestone report
    ACTION submitreport(name proposal_name, string report);

    //claim milestone funding
    ACTION claimfunds(name proposal_name);

    //start next milestone
    ACTION nextms(name proposal_name, name ballot_name);

    //======================== account actions ========================

    //withdraw from account balance
    ACTION withdraw(name account_name, asset quantity);

    //delete an account
    ACTION deleteacct(name account_name);

    //======================== notification functions ========================

    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    [[eosio::on_notify("telos.decide::broadcast")]]
    void catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

    //======================== functions ========================

    //subtract amount from balance
    void sub_balance(name account_owner, asset quantity);

    //add amount to balance
    void add_balance(name account_owner, asset quantity);

    //validate a category
    bool valid_category(name category);

    //returns status name of milestone
    name get_milestone_status(name proposal_name, uint64_t milestone_id);

    //======================== contract tables ========================

    //config data
    //scope: self
    TABLE config {
        string app_name; //Telos Works
        string app_version; //v0.1.0
        name admin; //self

        asset available_funds; //total available funding for proposals
        asset reserved_funds; //total funding reserved by approved proposals
        asset deposited_funds; //total deposited funds made by accounts
        asset paid_funds; //total lifetime funding paid

        double quorum_threshold; //percent of votes to pass quorum
        double yes_threshold; //percent of yes votes to approve

        double quorum_refund_threshold; //percent of quorum votes to return fee
        double yes_refund_threshold; //percent of yes votes to return fee

        asset min_fee; //30 TLOS per milestone
        double fee_percent; //5%

        uint16_t min_milestones; //1
        uint16_t max_milestones; //12
        uint32_t milestone_length; //2505600 seconds (29 days)
        
        asset min_requested; //1k TLOS
        asset max_requested; //500k TLOS

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)
            (available_funds)(reserved_funds)(deposited_funds)(paid_funds)
            (quorum_threshold)(yes_threshold)(quorum_refund_threshold)(yes_refund_threshold)
            (min_fee)(fee_percent)(min_milestones)(max_milestones)(milestone_length)
            (min_requested)(max_requested))
    };
    typedef singleton<name("config"), config> config_singleton;

    //proposal data
    //scope: self
    TABLE proposal {
        string title; //proposal title
        string description; //short tweet-length description
        string content; //link to full content
        name proposal_name; //name of proposal
        name proposer; //account name making proposal
        name category; //marketing, apps, developers, education
        name status; //drafting, inprogress, failed, cancelled, completed
        name current_ballot; //name of current milestone ballot
        asset fee; //fee paid to launch proposal
        bool refunded; //true if fee refunded
        asset total_requested; //total funds requested
        asset remaining; //total remaining funds
        uint16_t milestones; //total milestones
        uint16_t current_milestone; //current proposal milestone

        uint64_t primary_key() const { return proposal_name.value; }
        uint64_t by_proposer() const { return proposer.value; }
        uint64_t by_category() const { return category.value; }
        uint64_t by_status() const { return status.value; }
        uint64_t by_ballot() const { return current_ballot.value; }

        EOSLIB_SERIALIZE(proposal, (title)(description)(content)
            (proposal_name)(proposer)(category)(status)(current_ballot)
            (fee)(refunded)(total_requested)(remaining)(milestones)(current_milestone))
    };
    typedef multi_index<name("proposals"), proposal,
        indexed_by<name("byproposer"), const_mem_fun<proposal, uint64_t, &proposal::by_proposer>>,
        indexed_by<name("bycategory"), const_mem_fun<proposal, uint64_t, &proposal::by_category>>,
        indexed_by<name("bystatus"), const_mem_fun<proposal, uint64_t, &proposal::by_status>>,
        indexed_by<name("byballot"), const_mem_fun<proposal, uint64_t, &proposal::by_ballot>>
    > proposals_table;

    //milestone data
    //scope: proposal_name.value
    TABLE milestone {
        uint64_t milestone_id; //milestone number
        name status; //queued, voting, passed, failed, paid
        asset requested; //amount requested for milestone
        string report; //previous milestone report and plan for current milestone
        name ballot_name;
        map<name, asset> ballot_results;

        uint64_t primary_key() const { return milestone_id; }

        EOSLIB_SERIALIZE(milestone, (milestone_id)(status)(requested)(report)(ballot_name)(ballot_results))
    };
    typedef multi_index<name("milestones"), milestone> milestones_table;

    //account data
    //scope: account_name.value
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }

        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

    //telos decide treasury
    // TODO: import from decide
    struct treasury {
        asset supply;
        asset max_supply;
        name access;
        name manager;
        string title;
        string description;
        string icon;
        uint32_t voters;
        uint32_t delegates;
        uint32_t committees;
        uint32_t open_ballots;
        bool locked;
        name unlock_acct;
        name unlock_auth;
        map<name, bool> settings;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
        EOSLIB_SERIALIZE(treasury, 
            (supply)(max_supply)(access)(manager)
            (title)(description)(icon)
            (voters)(delegates)(committees)(open_ballots)
            (locked)(unlock_acct)(unlock_auth)(settings))
    };
    typedef multi_index<name("treasuries"), treasury> treasuries_table;

};
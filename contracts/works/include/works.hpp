// Telos Works is a Worker Proposal System for the Telos Blockchain Network.
//
// @author Craig Branscom
// @contract works
// @version v0.1.0

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

using namespace std;
using namespace eosio;

//approved treasuries: VOTE

//proposal statuses: drafting, voting, passed, failed, cancelled, funded

//milestones status: undecided, passed, failed, funded

CONTRACT works : public contract {

    public:

    works(name self, name code, datastream<const char*> ds) : contract(self, code, ds) {}

    ~works() {}

    const symbol TLOS_SYM = symbol("TLOS", 4);
    const symbol VOTE_SYM = symbol("VOTE", 4);

    //======================== admin actions ========================

    //initialize the contract
    ACTION init(string app_name, string app_version, name initial_admin);

    //set contract version
    ACTION setversion(string new_version);

    //set new admin
    ACTION setadmin(name new_admin);

    //TODO: actions to change config settings

    //======================== proposal actions ========================

    //draft a new community proposal
    ACTION draftprop(string title, string description, string content, name proposal_name, 
        name proposer, name category, asset total_requested, optional<uint16_t> milestones);

    //launch a drafted proposal
    ACTION launchprop(name proposal_name);

    //advance a proposal to the next milestone
    ACTION advanceprop(name proposal_name);

    //cancel proposal
    ACTION cancelprop(name proposal_name, string memo);

    //delete proposal
    ACTION deleteprop(name proposal_name);

    //======================== milestone actions ========================

    //add a new milestone to a proposal
    ACTION addmilestone(name proposal_name, asset requested);

    //edit a milestone
    ACTION editms(name proposal_name, name ballot_name, asset requested);

    //submit a milestone report
    ACTION submitreport(name proposal_name, name ballot_name, string report);

    //remove a milestone from a proposal
    ACTION rmvmilestone(name proposal_name, name ballot_name);

    //======================== account actions ========================

    //withdraw from account balance
    ACTION withdraw(account_name, asset quantity);

    //delete an account
    ACTION deleteacct(account_name);

    //======================== notification functions ========================

    [[eosio::on_notify("eosio.token::transfer")]]
    void catch_transfer(name from, name to, asset quantity, string memo);

    [[eosio::on_notify("trailservice::broadcast")]]
    catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

    //======================== functions ========================

    //require a charge to an account
    void require_fee(name account_owner, asset fee);

    //validate a category
    valid_category(name category);

    //======================== contract tables ========================

    //config data
    //scope: self
    TABLE config {
        string app_name;
        string app_version;
        name admin;

        uint32_t milestone_length; //2505600 seconds (29 days)
        uint16_t max_milestones; //12
        asset min_fee; //30 TLOS per milestone
        asset max_requested; //500k TLOS

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(admin)
            (milestone_length)(max_milestones)(min_fee)(max_requested))
    };
    typedef singleton<name("config"), config> config_singleton;

    //proposal data
    //scope: self
    TABLE proposal {
        uint64_t proposal_name;
        name proposer;
        name category;
        name status;
        name current_ballot;
        asset fee;
        asset total_requested;
        asset remaining;
        uint16_t milestones;
        uint16_t current_milestone;

        uint64_t primary_key() const { return proposal_id; }
        uint64_t by_proposer() const { return proposer.value; }
        uint64_t by_category() const { return category.value; }
        uint64_t by_status() const { return status.value; }
        EOSLIB_SERIALIZE(proposal, (proposal_name)(current_ballot)(proposer)(category)(status)
            (fee)(total_requested)(remaining)(milestones)(current_milestone))
    };
    typedef multi_index<name("proposals"), proposal,
        indexed_by<name("byproposer"), const_mem_fun<proposal, uint64_t, &proposal::by_proposer>>,
        indexed_by<name("bycategory"), const_mem_fun<proposal, uint64_t, &proposal::by_category>>,
        indexed_by<name("bystatus"), const_mem_fun<proposal, uint64_t, &proposal::by_status>>
    > proposals_table;

    //milestone data
    //scope: proposal_name.value
    TABLE milestone {
        name ballot_name;
        name status;
        asset requested;
        string report;
        map<name, asset> ballot_results;

        uint64_t primary_key() const { return milestone_id; }
        EOSLIB_SERIALIZE(milestone, (ballot_name)(status)(requested)(report)(ballot_results))
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

};
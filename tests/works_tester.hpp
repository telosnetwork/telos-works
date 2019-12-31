#include <trail_tester.hpp>

#include <contracts.hpp>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;
using namespace trail::testing;

using mvo = fc::mutable_variant_object;

class works_tester : public trail_tester {
public:
    abi_serializer works_abi_ser;

    // NOTE: this name may not be perm yet
    const name works_name = name("telosworks");

    works_tester(setup_mode mode = setup_mode::full): trail_tester(mode) {
        asset ram_amount = asset::from_string("400.0000 TLOS");
        // asset liquid_amount = asset::from_string("10000.0000 TLOS");
        create_account_with_resources(works_name, eosio_name, ram_amount, false);
        setup_works_contract();
    }

    //======================== setup functions ========================

    void setup_works_contract() {
        set_code( works_name, contracts::works_wasm());
        set_abi( works_name, contracts::works_abi().data() );
        {
            const auto& accnt = control->db().get<account_object,by_name>( eosio_name );
            abi_def abi;
            BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
            works_abi_ser.set_abi(abi, abi_serializer_max_time);
        }
        works_init("Telos Works", "v2.0.0", works_name);
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

    //======================== works getters ========================

    //======================== works helpers ========================
};
#include "et.msig_v2.hpp"
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>


namespace eosio {

void multisig_output_limits::addhashfile(std::string hash) {
    hash_table table(_self, _self);
    eosio_assert(!table.exists(), "hash file exist");

    table.set({hash}, _self);
}

void multisig_output_limits::updhashfile(std::string hash) {
    hash_table table(_self, _self);
    eosio_assert(table.exists(), "hash file not exist");

    table.set({hash}, _self);
}

void multisig_output_limits::crtlimits(account_name name, vector<multisig_output_limits::st_limit> vlimits) {
    require_auth(_self);
    eosio_assert(vlimits.size(), "not elements in list");

    auto symbol = vlimits.begin()->amount.symbol;
    limits limits_table(_self, symbol.name());
    auto it_limits = limits_table.find(name);
    eosio_assert(it_limits == limits_table.end(), "This account limit exist");

    limits_table.emplace(_self, [&](auto &item){
        item.account = name;
        item.limit = vlimits;
    });
}

void multisig_output_limits::updlimits(account_name name, vector<multisig_output_limits::st_limit> vlimits) {
    require_auth(_self);
    eosio_assert(vlimits.size(), "not elements in list");

    auto symbol = vlimits.begin()->amount.symbol;
    limits limits_table(_self, symbol.name());
    auto it_limits = limits_table.find(name);
    eosio_assert(it_limits != limits_table.end(), "Not found account in limits table");

    limits_table.modify(it_limits, 0, [&](auto &item){
        item.limit = vlimits;
    });
}

void multisig_output_limits::dellimits(symbol_type symbol) {
    require_auth(_self);

    limits limits_table(_self, symbol.name());
//    for (auto item  = limits_table.begin(); item != limits_table.end(); ++item) {
//        item = limits_table.erase(item);
//    }

    auto item = limits_table.begin();
    while ( item != limits_table.end() ) {
        item = limits_table.erase(item);
    }
}

void multisig_output_limits::propose(account_name proposer, name proposal_name, transaction_object transaction_obj) {
    require_auth( proposer );

    proposals proptable( _self, _self );
    eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

    auto symbol = transaction_obj.packed_transaction.quantity.symbol;
    limits limits_table(_self, symbol.name());
    auto it_limits = limits_table.find(proposer);   
    eosio_assert(it_limits != limits_table.end(), "Not found account");

    auto quantity = transaction_obj.packed_transaction.quantity;
    auto lower_limit = st_limit{asset(0, S(4,EOS))};

    for (auto i(0); i <= it_limits->limit.size(); ++i) {
        st_limit upper_limit;
        if (i == it_limits->limit.size())
            upper_limit = st_limit{asset(quantity.amount + 1, S(4,EOS))};
        else
            upper_limit = it_limits->limit[i];

        if (lower_limit.amount <= quantity && quantity < upper_limit.amount) {
            vector<permission_level> oracles;
            if (i) {
                oracles = lower_limit.requested_approvals;
            }

            proptable.emplace( proposer, [&]( auto& prop ) {
                prop.proposer            = proposer;
                prop.proposal_name       = proposal_name;
                prop.struct_transaction  = transaction_obj;
            });

            approvals apptable(  _self, _self );
            apptable.emplace( proposer, [&]( auto& a ) {
                a.proposal_name       = proposal_name;
                a.requested_approvals = std::move(oracles);
            });

            SEND_INLINE_ACTION( *this, exec, {_self, N(active)}, {proposal_name, _self} );
            break;
        }

        lower_limit = it_limits->limit[i];
    }
}

void multisig_output_limits::approve(name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    auto itr_requested_approvals = std::find( apps.requested_approvals.begin(), apps.requested_approvals.end(), level );
    eosio_assert( itr_requested_approvals != apps.requested_approvals.end(), "approval is not on the list of requested approvals" );
        apptable.modify( apps, 0, [&]( auto& a ) {
            a.provided_approvals.push_back( level );
            a.requested_approvals.erase( itr_requested_approvals );
        });

    SEND_INLINE_ACTION( *this, exec, {_self, N(active)}, {proposal_name, _self} );
}

void multisig_output_limits::unapprove(name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    auto itr_provided_approvals = std::find( apps.provided_approvals.begin(), apps.provided_approvals.end(), level );
    eosio_assert( itr_provided_approvals != apps.provided_approvals.end(), "no approval previously granted" );
    apptable.modify( apps, 0, [&]( auto& a ) {
        a.requested_approvals.push_back( level );
        a.provided_approvals.erase( itr_provided_approvals );
    });
}

void multisig_output_limits::cancel(name proposal_name, account_name canceler ) {
    require_auth( canceler );

    proposals proptable( _self, _self );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    eosio_assert( canceler == prop.proposer, "cannot cancel until expiration" );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    proptable.erase(prop);
    apptable.erase(apps);
}

void multisig_output_limits::exec(name proposal_name, account_name executer ) {
    require_auth( executer );
    eosio_assert( executer == _self, "Not permission exec" );

    proposals proptable( _self, _self );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    if ( !apps.requested_approvals.size() ) {
        transaction trx;
        trx.actions.push_back({permission_level(_self, N(active)), prop.struct_transaction.contract, prop.struct_transaction.action,
                               prop.struct_transaction.packed_transaction });
        trx.send(prop.proposal_name, _self);

        proptable.erase(prop);
        apptable.erase(apps);
    }
}

template<typename test>
void multisig_output_limits::tt(test &&l)
{

}

} /// namespace eosio

EOSIO_ABI( eosio::multisig_output_limits, (addhashfile)(updhashfile)(crtlimits)(updlimits)(dellimits)(propose)(approve)(unapprove)(cancel)(exec) )

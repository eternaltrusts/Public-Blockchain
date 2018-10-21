#include "et.msig.hpp"
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>


namespace eosio {

void multisig::propose(account_name proposer,
                       name proposal_name,
                       vector<permission_level> mandatory_list,
                       vector<permission_level> oracles,
                       uint8_t number_of_confirmations,
                       transaction_object transaction_obj) {
    eosio_assert(oracles.size() >= number_of_confirmations, "There are not enough oracles to confirm the transaction");
    require_auth( proposer );

    proposals proptable( _self, _self );
    eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

    proptable.emplace( proposer, [&]( auto& prop ) {
        prop.proposer            = proposer;
        prop.proposal_name       = proposal_name;
        prop.struct_transaction  = transaction_obj;
        prop.number_of_confirmations = number_of_confirmations;
    });

    approvals apptable(  _self, _self );
    apptable.emplace( proposer, [&]( auto& a ) {
        a.proposal_name       = proposal_name;
        a.requested_approvals = std::move(oracles);
        a.mandatory_requested_approvals = std::move(mandatory_list);
    });
}

void multisig::approve(name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    bool find_flag = false;
    auto itr_requested_approvals = std::find( apps.requested_approvals.begin(), apps.requested_approvals.end(), level );
    if ( itr_requested_approvals != apps.requested_approvals.end() ) {
        find_flag = true;
        apptable.modify( apps, 0, [&]( auto& a ) {
            a.provided_approvals.push_back( level );
            a.requested_approvals.erase( itr_requested_approvals );
        });
    }

    auto itr_mandatory_requested_approvals = std::find( apps.mandatory_requested_approvals.begin(), apps.mandatory_requested_approvals.end(), level );
    if ( itr_mandatory_requested_approvals != apps.mandatory_requested_approvals.end() ) {
        find_flag = true;
        apptable.modify( apps, 0, [&]( auto& a ) {
            a.mandatory_provided_approvals.push_back( level );
            a.mandatory_requested_approvals.erase( itr_mandatory_requested_approvals );
        });
    }

    eosio_assert( find_flag, "approval is not on the list of requested approvals" );
    SEND_INLINE_ACTION( *this, exec, {_self, N(active)}, {proposal_name, _self} );
}

void multisig::unapprove(name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    bool find_flag = false;
    auto itr_provided_approvals = std::find( apps.provided_approvals.begin(), apps.provided_approvals.end(), level );
    if ( itr_provided_approvals != apps.provided_approvals.end() ) {
        find_flag = true;
        apptable.modify( apps, 0, [&]( auto& a ) {
            a.requested_approvals.push_back( level );
            a.provided_approvals.erase( itr_provided_approvals );
        });
    }

    auto itr_mandatory_provided_approvals = std::find( apps.mandatory_provided_approvals.begin(), apps.mandatory_provided_approvals.end(), level );
    if ( itr_mandatory_provided_approvals != apps.mandatory_provided_approvals.end() ) {
        find_flag = true;
        apptable.modify( apps, 0, [&]( auto& a ) {
            a.mandatory_requested_approvals.push_back( level );
            a.mandatory_provided_approvals.erase( itr_mandatory_provided_approvals );
        });
    }

    eosio_assert( find_flag, "no approval previously granted" );
}

void multisig::cancel(name proposal_name, account_name canceler ) {
    require_auth( canceler );

    proposals proptable( _self, _self );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    eosio_assert( canceler == prop.proposer, "cannot cancel until expiration" );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    proptable.erase(prop);
    apptable.erase(apps);
}

void multisig::exec(name proposal_name, account_name executer ) {
    require_auth( executer );
    eosio_assert( executer == _self, "Not permission exec" );

    proposals proptable( _self, _self );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    approvals apptable(  _self, _self );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    if ( !apps.mandatory_requested_approvals.size() && apps.provided_approvals.size() >= prop.number_of_confirmations ) {

        transaction trx;
        trx.actions.push_back({permission_level(_self, N(active)), prop.struct_transaction.contract, prop.struct_transaction.action,
                               prop.struct_transaction.packed_transaction });
        trx.send(prop.proposal_name, _self);

        proptable.erase(prop);
        apptable.erase(apps);
    }
}

} /// namespace eosio

EOSIO_ABI( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec) )

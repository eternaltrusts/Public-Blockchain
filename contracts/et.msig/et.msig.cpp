#include "et.msig.hpp"
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>

#include "eternaltrusts/objects.hpp"

namespace eosio {

const int COUNT_ORACLES = 5;
const int CONSENSUS_ORACLES_APPROVE = 3;

void multisig::propose( account_name proposer,
                        name proposal_name,
                        vector<permission_level> requested,
                        std::string  trx_hl) {
    eosio_assert(requested.size() == COUNT_ORACLES, "There are not enough oracles to confirm the transaction");
    require_auth( proposer );

    proposals proptable( _self, proposer );
    eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

    proptable.emplace( proposer, [&]( auto& prop ) {
        prop.proposal_name       = proposal_name;
        prop.packed_transaction  = trx_hl;
    });

    approvals apptable(  _self, proposer );
    apptable.emplace( proposer, [&]( auto& a ) {
        a.proposal_name       = proposal_name;
        a.requested_approvals = std::move(requested);
    });
}

void multisig::approve( account_name proposer, name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, proposer );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    auto itr = std::find( apps.requested_approvals.begin(), apps.requested_approvals.end(), level );
    eosio_assert( itr != apps.requested_approvals.end(), "approval is not on the list of requested approvals" );

    apptable.modify( apps, proposer, [&]( auto& a ) {
        a.provided_approvals.push_back( level );
        a.requested_approvals.erase( itr );
    });
}

void multisig::unapprove( account_name proposer, name proposal_name, permission_level level ) {
    require_auth( level );

    approvals apptable(  _self, proposer );
    auto& apps = apptable.get( proposal_name, "proposal not found" );
    auto itr = std::find( apps.provided_approvals.begin(), apps.provided_approvals.end(), level );
    eosio_assert( itr != apps.provided_approvals.end(), "no approval previously granted" );

    apptable.modify( apps, proposer, [&]( auto& a ) {
        a.requested_approvals.push_back(level);
        a.provided_approvals.erase(itr);
    });
}

void multisig::cancel( account_name proposer, name proposal_name, account_name canceler ) {
    require_auth( canceler );

    proposals proptable( _self, proposer );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    eosio_assert( canceler == proposer, "cannot cancel until expiration" );

    approvals apptable(  _self, proposer );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    proptable.erase(prop);
    apptable.erase(apps);
}

void multisig::exec( account_name proposer, name proposal_name, account_name executer ) {
    require_auth( executer );
    eosio_assert( proposer == executer, "Not permission exec" );

    proposals proptable( _self, proposer );
    auto& prop = proptable.get( proposal_name, "proposal not found" );

    approvals apptable(  _self, proposer );
    auto& apps = apptable.get( proposal_name, "proposal not found" );

    eosio_assert(apps.provided_approvals.size() >= CONSENSUS_ORACLES_APPROVE, "There is no consensus");


    transaction trx;
    trx.actions.push_back({permission_level(executer, N(active)), N(et.hl), N(addtrx),
                           structures::struct_no_id_transaction{executer, prop.packed_transaction}});
    trx.send(proposal_name, _self, true);

    proptable.erase(prop);
    apptable.erase(apps);
}

} /// namespace eosio

EOSIO_ABI( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec) )

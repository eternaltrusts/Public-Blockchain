#include "eternaltrusts.hpp"

using namespace eosio;

/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        eternaltrusts(receiver).apply(code, action);
    }
} // extern "C"


eternaltrusts::eternaltrusts(account_name creator)
    : eosio::contract(creator)
    , _table_transactions(_self, _self)
{}

/**
 * @brief eternaltrusts::apply
 * @param action
 */
void eternaltrusts::apply(uint64_t /*code*/, uint64_t action) {
    if (N(createtrx) == action)
        hyperLedger_transaction_id(unpack_action_data<structures::struct_no_id_transaction>());
}

/**
 * @brief eternaltrusts::hyperLedger_transaction_id
 * @param m_transaction_id
 */
void eternaltrusts::hyperLedger_transaction_id(const structures::struct_no_id_transaction &m_transaction_id) {
    _table_transactions.emplace(_self, [&](auto &item) {
        item = structures::struct_transaction(m_transaction_id, _table_transactions.available_primary_key());
    });
}

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "objects.hpp"

using namespace eosio;

class eternaltrusts : public eosio::contract {
public:
    explicit eternaltrusts(account_name creator);
    void apply(uint64_t, uint64_t action);

private:
    void hyperLedger_transaction_id(const structures::struct_no_id_transaction &m_transaction_id);

private:
    tables::transactions_table _table_transactions;
};



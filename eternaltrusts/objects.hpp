#include <eosiolib/eosio.hpp>


namespace eosio {

namespace structures {
    using std::string;
    using std::array;

struct struct_no_id_transaction
{
    struct_no_id_transaction() = default;

    account_name account;
    string transaction_id;

    EOSLIB_SERIALIZE( struct_no_id_transaction, (account)(transaction_id) )
};

struct struct_transaction : public struct_no_id_transaction
{
    struct_transaction() = default;
    struct_transaction(const struct_no_id_transaction &m_struct_no_id_transaction, const uint64_t &m_id)
        : struct_no_id_transaction(m_struct_no_id_transaction)
        , id(m_id)
    {}

    uint64_t id;

    auto primary_key()const {
        return id;
    }

    EOSLIB_SERIALIZE( struct_transaction, (account)(transaction_id)(id) )
};

}


namespace tables {
    using transactions_table = eosio::multi_index<N(transactions), structures::struct_transaction>;
}

}


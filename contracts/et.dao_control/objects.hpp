#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace std;
using namespace eosio;

#define ETT_TOKEN symbol_type(S(4,ETT))
#define MIN_LIMIT_BALANCE_DAO_DIRECTOR asset(1000000, ETT_TOKEN)
#define DELEGATE_TOKEN_ORACLE asset(100000, ETT_TOKEN)

namespace structures {
struct director {
    account_name account;

    auto primary_key() const {
        return account;
    }

    EOSLIB_SERIALIZE( director, (account) )
};

struct oracle {
    account_name account;
    string url;

    auto primary_key() const {
        return account;
    }

    EOSLIB_SERIALIZE( oracle, (account)(url) )
};
}

namespace tables {
    using directors = eosio::multi_index<N(directors), structures::director>;
    using oracles = eosio::multi_index<N(oracles), structures::oracle>;
}

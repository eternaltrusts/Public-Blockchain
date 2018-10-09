#pragma once

#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace std;
using namespace eosio;

#define ETT_TOKEN symbol_type(S(4,ETT))
#define CONVERT_RATE 5

namespace structures {

struct account {
   asset    balance;
   time_point_sec frozen_until;

   uint64_t primary_key()const { return balance.symbol.name(); }

   EOSLIB_SERIALIZE( account, (balance)(frozen_until))
};

struct currency_stats {
   asset          supply;
   asset          max_supply;
   account_name   issuer;

   uint64_t primary_key()const { return supply.symbol.name(); }

   EOSLIB_SERIALIZE( currency_stats, (supply)(max_supply)(issuer) )
};

struct transfer_args {
   account_name  from;
   account_name  to;
   asset         quantity;
   string        memo;

   EOSLIB_SERIALIZE( transfer_args, (from)(to)(quantity)(memo) )
};

struct freezing_tokens {
    account_name  account;
    time_point_sec to_which;

    EOSLIB_SERIALIZE( freezing_tokens, (account)(to_which) )
};

struct defrost_tokens {
    account_name  account;

    EOSLIB_SERIALIZE( defrost_tokens, (account) )
};

}

namespace tables {
    using accounts = eosio::multi_index<N(accounts), structures::account>;
    using stats = eosio::multi_index<N(stat), structures::currency_stats>;
}

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#define TOKEN_PTT(amount) asset(int64_t(amount), string_to_symbol(4, "PTT"))
#define MIN_BALANCE_DAO_DIRECTOR TOKEN_PTT(1000000)


namespace structures {

struct account {
    eosio::asset balance;

    auto primary_key() const {
        return balance.symbol.name();
    }

    EOSLIB_SERIALIZE(account, (balance))
};

struct currency_stats {
    eosio::asset   supply;
    eosio::asset   max_supply;
    account_name   issuer;

    auto primary_key() const {
        return supply.symbol.name();
    }

    EOSLIB_SERIALIZE(currency_stats, (supply)(max_supply)(issuer))
};

struct transfer_args {
    account_name  from;
    account_name  to;
    eosio::asset  quantity;
    std::string   memo;

    EOSLIB_SERIALIZE(transfer_args, (from)(to)(quantity)(memo))
};

struct account_args {
    account_name account;
    account_name sender;
    std::string  url;

    auto primary_key() const {
        return account;
    }

    EOSLIB_SERIALIZE(account_args, (account)(sender)(url))
};

}

namespace tables {
using accounts = eosio::multi_index<N(accounts), structures::account>;
using stats = eosio::multi_index<N(stat), structures::currency_stats>;

using dao_oracles = eosio::multi_index<N(oracles), structures::account_args>;
using dao_dirctors = eosio::multi_index<N(directors), structures::account_args>;
}

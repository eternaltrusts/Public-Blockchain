#include "et.dao_control.hpp"
#include <et.token/et.token.hpp>

using namespace eosio;

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        dao_control(receiver).apply(code, action);
    }
}

void eosio::dao_control::apply(uint64_t /*code*/, uint64_t action) {
    if(N(create) == action)
        execute_action(this, &dao_control::create_dao_director);
    if(N(createoracle) == action)
        execute_action(this, &dao_control::create_dao_oracle);
}

void dao_control::create_dao_director(account_name account) {
    require_auth(account);

    tables::directors directors(_self, _self);
    auto it_director = directors.find(account);
    eosio_assert(it_director == directors.end(), "DAO director with the same name exists");

    auto balance_account = et_token(N(et.token)).get_balance(account, ETT_TOKEN.name());
    eosio_assert(balance_account >= MIN_LIMIT_BALANCE_DAO_DIRECTOR, "Not enough money to become DAO director");

    directors.emplace(account, [&]( auto &item ){
        item.account = account;
    });
}

void eosio::dao_control::create_dao_oracle(account_name account, account_name sender, string url) {
    tables::directors directors(_self, _self);
    auto director = directors.find( sender );
    eosio_assert( director != directors.end(), "Not found DAO director" );
    require_auth( sender );

    tables::oracles oracles(_self, _self);
    auto oracle = oracles.find( account );
    eosio_assert( oracle == oracles.end(), "Oracle already exists" );

    auto balance_director = et_token(N(et.token)).get_balance(sender, ETT_TOKEN.name());
    eosio_assert( balance_director > MIN_LIMIT_BALANCE_DAO_DIRECTOR, "DAO director can not create oracles, not enough tokens" );

    oracles.emplace(sender, [&]( auto &item ) {
        item.account = account;
        item.url = url;
    });

    INLINE_ACTION_SENDER(eosio::et_token, transfer)( N(et.token), {sender, N(active)},
                                                     { sender, account, DELEGATE_TOKEN_ORACLE, "Delegating tokens for voting" });
}

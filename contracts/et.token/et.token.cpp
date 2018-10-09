#include "et.token/et.token.hpp"

using namespace eosio;

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        et_token(receiver).apply(code, action);
    }
}

void et_token::apply(uint64_t code, uint64_t action) {
    if (N(transfer) == action && N(eosio.token) == code)
        execute_action(this, &et_token::buy_token);

    if(N(create) == action && _self == code)
        execute_action(this, &et_token::create);
    if(N(issue) == action && _self == code)
        execute_action(this, &et_token::issue);
    if(N(retire) == action && _self == code)
        execute_action(this, &et_token::retire);
    if(N(transfer) == action && _self == code)
        execute_action(this, &et_token::transfer);
    if(N(open) == action && _self == code)
        execute_action(this, &et_token::open);
    if(N(close) == action && _self == code)
        execute_action(this, &et_token::close);
    if(N(frost) == action && _self == code)
        execute_action(this, &et_token::freezing_tokens);
    if(N(defrost) == action && _self == code)
        execute_action(this, &et_token::defrost_tokens);
}

void et_token::create( account_name issuer,
                       asset        maximum_supply ) {
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    tables::stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply    = maximum_supply;
        s.issuer        = issuer;
    });
}

void et_token::issue( account_name to, asset quantity, string memo ) {
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    tables::stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
        s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
        SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void et_token::retire( asset quantity, string memo ) {
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    tables::stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must retire positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, 0, [&]( auto& s ) {
        s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

void et_token::transfer( account_name from,
                         account_name to,
                         asset        quantity,
                         string       memo ) {
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.name();
    tables::stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, quantity );
    add_balance( to, quantity, payer );
}

void et_token::sub_balance( account_name owner, asset value ) {
    tables::accounts from_acnts( _self, owner );

    const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
    eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );
    eosio_assert( from.frozen_until <= time_point_sec(now()), "Tokens are frozen" );

    from_acnts.modify( from, owner, [&]( auto& a ) {
        a.balance -= value;
    });
}

void et_token::add_balance( account_name owner, asset value, account_name ram_payer ) {
    tables::accounts to_acnts( _self, owner );
    auto to = to_acnts.find( value.symbol.name() );
    if( to == to_acnts.end() ) {
        to_acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = value;
            a.frozen_until = time_point_sec(now());
        });
    } else {
        to_acnts.modify( to, 0, [&]( auto& a ) {
            a.balance += value;
        });
    }
}

const asset et_token::convert_to_eternal_trusts_token(const asset &quantity)const {
    tables::stats statstable( _self, ETT_TOKEN.name() );
    auto existing = statstable.find( ETT_TOKEN.name() );

    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    eosio_assert( ETT_TOKEN == existing->supply.symbol, "symbol precision mismatch" );

    asset ett_token(0, ETT_TOKEN);
    ett_token.set_amount(quantity.amount);
    ett_token /= CONVERT_RATE;

    eosio_assert( ett_token <= existing->max_supply - existing->supply, "quantity exceeds available supply" );

    statstable.modify( existing, 0, [&]( auto& s ) {
        s.supply += ett_token;
    });

    return ett_token;
}

void et_token::open( account_name owner, symbol_type symbol, account_name ram_payer ) {
    require_auth( ram_payer );
    tables::accounts acnts( _self, owner );
    auto it = acnts.find( symbol.name() );
    if( it == acnts.end() ) {
        acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance.symbol = symbol;
            a.frozen_until = time_point_sec(now());
        });
    }
}

void et_token::close( account_name owner, symbol_type symbol ) {
    require_auth( owner );
    tables::accounts acnts( _self, owner );
    auto it = acnts.find( symbol.name() );
    eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
    eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
    acnts.erase( it );
}

void et_token::buy_token(account_name from, account_name /*to*/, asset quantity, string memo) {
    require_auth(from);

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.symbol.is_valid(), "invalid symbol name" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto ett_tokens = convert_to_eternal_trusts_token(quantity);
    add_balance( from, ett_tokens, _self );
}

void et_token::freezing_tokens(account_name account, time_point_sec to_which) {
    require_auth(_self);

    tables::accounts from_acnts( _self, account );
    const auto& balance = from_acnts.get( ETT_TOKEN.name(), "no balance object found" );
    from_acnts.modify( balance, 0, [&]( auto& a ) {
        a.frozen_until = to_which;
    });
}

void et_token::defrost_tokens(account_name account) {
    require_auth(_self);

    tables::accounts from_acnts( _self, account );
    const auto& balance = from_acnts.get( ETT_TOKEN.name(), "no balance object found" );
    from_acnts.modify( balance, 0, [&]( auto& a ) {
        a.frozen_until = time_point_sec(now());
    });
}

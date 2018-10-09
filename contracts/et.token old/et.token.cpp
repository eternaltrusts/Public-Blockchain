/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "et.token.hpp"

namespace eosio {

token::token(account_name self)
    : contract(self)
    , _oracles(_self, _self)
    , _directors(_self, _self)
{}

void token::create( account_name issuer,
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

void token::issue( account_name to, asset quantity, string memo ) {
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

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );

    if ( from != _self ) {
        auto from_oracle = _oracles.find( from );
        auto from_director = _directors.find( from );
        bool from_status = ( (from_oracle != _oracles.end()) || (from_director != _directors.end()) );
        eosio_assert(from_status, "Not found from account");
    }

    auto to_oracle = _oracles.find( to );
    auto to_director = _directors.find( to );
    bool to_status = ( (to_oracle != _oracles.end() ) || ( to_director != _directors.end()) );
    eosio_assert(to_status, "Not found from account");

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


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

void token::crtdirector(account_name account, account_name sender, std::string url) {
    require_auth( _self );

    _directors.emplace(_self, [&](auto &item){
        item.account = account;
        item.sender = sender;
        item.url = url;
    });

    SEND_INLINE_ACTION( *this, issue, {_self, N(active)}, {account, TOKEN_PTT(1500000), "Create DAO director"} );
}

void token::crtoracle(account_name account, account_name sender, std::string url) {
    auto director = _directors.find( sender );
    eosio_assert( director != _directors.end(), "Not found DAO director" );
    require_auth( sender );

    auto oracle = _oracles.find( account );
    eosio_assert( oracle == _oracles.end(), "Oracle already exists" );

    auto delegate_token_oracle = TOKEN_PTT(100000);

    tables::accounts account_director(_self, sender);
    auto token_balance = account_director.find( delegate_token_oracle.symbol.name() );
    eosio_assert( token_balance != account_director.end(), "Tokens of this type do not exist in this director" );
    eosio_assert( token_balance->balance > MIN_BALANCE_DAO_DIRECTOR, "DAO director can not create oracles, not enough tokens" );

    _oracles.emplace(sender, [&](auto &item){
        item.account = account;
        item.sender = sender;
        item.url = url;
    });

    SEND_INLINE_ACTION( *this, transfer, {sender, N(active)}, {sender, account, delegate_token_oracle, "Delegating tokens for voting"} );
}

void token::sub_balance( account_name owner, asset value ) {
   tables::accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   tables::accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer)(crtdirector)(crtoracle) )

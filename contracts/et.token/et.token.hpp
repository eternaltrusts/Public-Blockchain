/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include "objects.hpp"

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class et_token : public contract {
      public:
         et_token( account_name self ):contract(self){}
         void apply(uint64_t code, uint64_t action);

         inline asset get_supply( symbol_name sym )const;
         inline asset get_balance( account_name owner, symbol_name sym )const;

      private:
         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );
         void issue_by_token(account_name to, asset quantity, string memo );

         void retire( asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         void open( account_name owner, symbol_type symbol, account_name payer );

         void close( account_name owner, symbol_type symbol );

         void buy_token( account_name from,
                         account_name to,
                         asset        quantity,
                         string       memo );

         void freezing_tokens( account_name account,
                               time_point_sec to_which = time_point_sec(now()) );
         void defrost_tokens( account_name account);

      private:
         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );

         const asset convert_to_eternal_trusts_token( const asset &quantity )const;
   };

   asset et_token::get_supply( symbol_name sym )const {
      tables::stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset et_token::get_balance( account_name owner, symbol_name sym )const {
      tables::accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace eosio

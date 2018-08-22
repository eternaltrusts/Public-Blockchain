/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include "objects.hpp"

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         token( account_name self );

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to,
                     asset quantity,
                     string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );


         void crtdirector(account_name account, account_name sender,
                           std::string  url );

         void crtoracle(account_name account, account_name sender,
                         std::string  url );
      
      
         inline asset get_supply( symbol_name sym ) const;
         inline asset get_balance( account_name owner, symbol_name sym ) const;
         inline bool  is_active_oracle( account_name owner, symbol_name sym ) const;

      private:
         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );

      private:
         tables::dao_oracles _oracles;
         tables::dao_dirctors _directors;
   };

   asset token::get_supply( symbol_name sym ) const
   {
      tables::stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset token::get_balance( account_name owner, symbol_name sym ) const
   {
      tables::accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

   bool token::is_active_oracle( account_name owner, symbol_name sym ) const
   {
       tables::accounts accountstable( _self, owner );
       const auto& account = accountstable.get( sym );
       if ( account.balance >= TOKEN_PTT(10) )
           return true;

       return false;
   }

} /// namespace eosio

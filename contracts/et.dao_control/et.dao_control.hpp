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

   class dao_control : public contract {
      public:
         dao_control( account_name self ) : contract(self){}
         void apply(uint64_t code, uint64_t action);

      private:
        void create_dao_director(account_name account);
        void create_dao_oracle(account_name account, account_name sender, string url);
   };
} /// namespace eosio

#pragma once

#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>

#include "objects.hpp"

namespace eosio {

   class voting : public contract {
   public:
       voting( account_name self ):contract(self){}
       void apply(uint64_t code, uint64_t action);

   private:
       void create_voting(account_name creator, time_point_sec time_end, std::string memo,
                          structures::event_voting event);

       void cancel_voting(account_name account, uint64_t id);

       void send_ahead(account_name account, uint64_t id);
       void user_voice(account_name account, uint8_t type_voice);
   };
}

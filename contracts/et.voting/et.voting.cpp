#include "et.voting.hpp"

using namespace eosio;

extern "C" {
    void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        voting(receiver).apply(code, action);
    }
}


void voting::apply(uint64_t /*code*/, uint64_t action) {
    if (N(create) == action)
        execute_action(this, &voting::create_voting);
    if(N(cancel) == action)
        execute_action(this, &voting::cancel_voting);
    if(N(voice) == action)
        execute_action(this, &voting::user_voice);
    if(N(ahead) == action)
        execute_action(this, &voting::send_ahead);
}

void voting::create_voting(account_name creator, time_point_sec time_end, string memo,
                           structures::event_voting event) {

}

void voting::cancel_voting(account_name account, uint64_t id) {

}

void voting::send_ahead(account_name account, uint64_t id) {

}

void voting::user_voice(account_name account, uint8_t type_voice) {

}

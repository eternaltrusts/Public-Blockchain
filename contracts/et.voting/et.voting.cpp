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
    if (N(cancel) == action)
        execute_action(this, &voting::cancel_voting);
    if (N(voice) == action)
        execute_action(this, &voting::user_voice);
    if (N(ahead) == action)
        execute_action(this, &voting::send_ahead);
}

void voting::create_voting(account_name creator, time_point_sec time_end, string memo,
                           structures::event_voting event) {
    require_auth(creator);

    eosio_assert(memo.size() > 256, "maximum character length 256");
    eosio_assert(time_end.utc_seconds <= now(), "end time is less than or equal to current");

    tables::voting voting_table(_self, _self);
    voting_table.emplace(creator,[&](auto &item){
        item.id = voting_table.available_primary_key();
        item.creator = creator;
        item.date_create = time_point_sec(now());
        item.date_close = time_end;
        item.memo = memo;
        item.status = uint8_t(status_voting::open);
    });
}

void voting::cancel_voting(account_name account, uint64_t id) {
    require_auth(account);

    tables::voting voting_table(_self, _self);
    auto record = voting_table.find(id);

    eosio_assert(record != voting_table.end(), "not found record in table voting");
    eosio_assert(record->creator == account, "creator does not match");
    eosio_assert(record->status == uint8_t(status_voting::open), "you can close only open vote");

    voting_table.erase(record);
}

void voting::send_ahead(account_name account, uint64_t id) {
    require_auth(account);

    tables::voting voting_table(_self, _self);
    auto record = voting_table.find(id);

    eosio_assert(record != voting_table.end(), "not found record in table voting");
    eosio_assert(record->creator == account, "creator does not match");
    eosio_assert(record->status == uint8_t(status_voting::open), "you can close only open vote");

    voting_table.modify(record, 0, [&](auto &item) {
        item.date_close = time_point_sec(now());
    });
}

void voting::user_voice(account_name account, uint8_t type_voice) {

}

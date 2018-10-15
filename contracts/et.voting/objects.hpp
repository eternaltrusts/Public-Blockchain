#pragma once

#include <eosiolib/time.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

using namespace std;
using namespace eosio;


enum class status_voting {
    open,
    close,
    ahead
}

namespace structures {

    struct event_voting {
        account_name contract;
        action_name action;
        bytes data;

        EOSLIB_SERIALIZE( event_voting, (contract)(action)(data) )
    };

    struct voting {
        uint64_t id;
        account_name creator;
        time_point_sec date_create;
        time_point_sec date_close;
        string memo;
        event_voting event;
        uint8_t status;

        auto primary_key() const {
            return id;
        }

        EOSLIB_SERIALIZE( voting, (id)(creator)(date_create)(date_close)(memo)(event)(status) )
    };

    struct vote {
        uint64_t id;
        uint64_t id_voting;
        uint8_t type;
        account_name account;

        auto primary_key() const {
            return id;
        }

        EOSLIB_SERIALIZE( vote, (id)(id_voting)(type)(account) )
    };
}

namespace tables {
    using voting = eosio::multi_index<N(voting), structures::voting>;
    using votes = eosio::multi_index<N(votes), structures::vote>;
}

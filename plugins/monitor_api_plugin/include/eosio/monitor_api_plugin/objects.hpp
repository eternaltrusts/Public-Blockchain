#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {
namespace structures {

    struct empty_responce {
    };

    struct result {
        result(bool m_status = false, const std::string &m_comment = std::string())
            : status(m_status), comment(m_comment) {}

        bool status;
        std::string comment;
    };

    struct eos_trx {
        account_name account;
        std::string transaction_id;

        bool is_valid()const {
            return !transaction_id.empty();
        }
    };

    struct params {
        std::string contract;
        std::string action;
        std::vector<std::string> permissions;
    };
}
}

FC_REFLECT(eosio::structures::empty_responce,);
FC_REFLECT(eosio::structures::result, (status)(comment));
FC_REFLECT(eosio::structures::eos_trx, (account)(transaction_id));
FC_REFLECT(eosio::structures::params, (contract)(action)(permissions));

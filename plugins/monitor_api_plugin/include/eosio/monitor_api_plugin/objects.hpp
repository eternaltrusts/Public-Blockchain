#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/transaction.hpp>

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
        std::string account;
        std::string transaction_id;

        bool is_valid()const {
            return !transaction_id.empty();
        }
    };

    struct params_request {
        std::string contract;
        std::string action;
        std::vector<std::string> permissions;
    };

    struct transaction_hl {
        eos_trx         trx;
        params_request  params;
    };

    struct msig_approve {
        msig_approve() = default;

        std::string proposal_name;
        std::string proposer;
        std::string oracle;
        std::string url;
    };

    struct msig_exec {
        msig_exec() = default;

        std::string proposal_name;
        std::string proposer;
        std::string url;
        fc::variant requested;
        eosio::chain::transaction trx;

        uint8_t counter;
    };

    struct msig_params {
        msig_params() = default;

        std::string proposed_contract;
        std::string proposed_action;
        std::string proposer;

        unsigned int proposal_expiration_hours;
    };

    struct oracle {
        oracle() = default;

        std::string name;
        std::string url;
    };

    struct hl_obj {
        hl_obj() = default;

        std::string $class;
        std::string asset;
        std::string newValue;
        std::string transactionId;
        std::string timestamp;
    };
}
}

FC_REFLECT(eosio::structures::empty_responce,);
FC_REFLECT(eosio::structures::result, (status)(comment));
FC_REFLECT(eosio::structures::eos_trx, (account)(transaction_id));
FC_REFLECT(eosio::structures::params_request, (contract)(action)(permissions));
FC_REFLECT(eosio::structures::transaction_hl, (trx)(params));

FC_REFLECT(eosio::structures::msig_approve, (proposal_name)(proposer)(oracle)(url));
FC_REFLECT(eosio::structures::msig_exec, (proposal_name)(proposer)(url)(trx)(counter));
FC_REFLECT(eosio::structures::msig_params, (proposed_contract)(proposed_action)(proposer)(proposal_expiration_hours));

FC_REFLECT(eosio::structures::oracle, (name)(url));
FC_REFLECT(eosio::structures::hl_obj, ($class)(asset)(newValue)(transactionId)(timestamp));

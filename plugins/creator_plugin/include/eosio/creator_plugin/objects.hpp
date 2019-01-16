#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/transaction.hpp>

namespace structures {
struct empty_responce {
};

struct result {
    result(bool m_status = false, const std::string &m_comment = std::string())
        : status(m_status), comment(m_comment) {}

    bool status;
    std::string comment;
};

struct create_account {
    std::string account;
};

struct add_contract {
    std::string account;
    std::string password;
    std::string type;
};

struct result_create_account {
    result_create_account(std::string m_account)
        : account(m_account), status(false){}

    bool status;
    std::string account;
    std::string password;
    std::string owner_private_key;
    std::string owner_public_key;
    std::string active_private_key;
    std::string active_public_key;
};

};

FC_REFLECT(structures::empty_responce,)
FC_REFLECT(structures::result,(status)(comment))
FC_REFLECT(structures::create_account,(account))
FC_REFLECT(structures::add_contract,(account)(password)(type))
FC_REFLECT(structures::result_create_account,(account)(password)(owner_private_key)(owner_public_key)(active_private_key)(active_public_key))

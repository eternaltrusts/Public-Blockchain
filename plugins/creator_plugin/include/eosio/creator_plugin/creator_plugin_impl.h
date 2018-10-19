#pragma once

#include "eosio/creator_plugin/httpc.hpp"
#include "eosio/creator_plugin/objects.hpp"

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/network/url.hpp>

#include <queue>

namespace eosio {
using namespace eosio::chain;

class creator_plugin_impl
{
public:
    creator_plugin_impl();
    void tester_app();

    structures::result_create_account create_account(const structures::create_account &name_accaunt);


private:
    void create_wallet(structures::result_create_account &obj);
    void generate_key(structures::result_create_account &obj);
    void import_key(const structures::result_create_account &obj);
    void cretae_account_in_eosio(structures::result_create_account &obj);

private:
    template<typename T>
    fc::variant call(const std::string& url, const std::string& path, const T& v);
    fc::variant call(const std::string& url, const std::string& path);

    eosio::chain_apis::read_only::get_info_results get_info(const string &url);
    vector<chain::permission_level> get_account_permissions(const vector<string>& permissions);

    chain::action generate_nonce_action();
    fc::variant determine_required_keys(const string &url, const string &wallet_url, const signed_transaction& trx);

    void sign_transaction(const string &wallet_url, signed_transaction& trx, variant &required_keys, const chain_id_type& chain_id);

    fc::variant push_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    fc::variant push_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    void print_action(const fc::variant& at);
    void print_action_tree(const fc::variant& action);
    void print_result(const fc::variant& result);

    structures::result send_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    void send_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    chain::action create_action(const string &url, const vector<permission_level>& authorization, const account_name& code, const action_name& act, const fc::variant& args);

    fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);

    string is_valid_url(const string &url);
    std::string is_valid_timestamp(const string &timestamp);

    optional<signature_type> try_sign_digest(const digest_type digest, const public_key_type public_key);

    bytes variant_to_bin(const account_name& account, const action_name& action, const fc::variant& action_args_var);

    asset to_asset( account_name code, const string& s );
    inline asset to_asset( const string& s );

    chain::action create_newaccount(const name& creator, const name& newaccount, public_key_type owner, public_key_type active);
    chain::action create_action(const vector<permission_level>& authorization, const account_name& code, const action_name& act, const fc::variant& args);

    chain::action create_buyram(const name& creator, const name& newaccount, const asset& quantity);
    chain::action create_delegate(const name& from, const name& receiver, const asset& net, const asset& cpu, bool transfer);

private:
    eosio::client::http::http_context _context;

    bool _no_verify = false;
    bool   _tx_force_unique;
    bool   _tx_skip_sign;
    bool   _tx_print_json;
    bool   _print_request;
    bool   _print_response;
    bool   _is_creator_app;
};
}



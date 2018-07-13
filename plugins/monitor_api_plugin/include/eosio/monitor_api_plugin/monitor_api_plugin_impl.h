#pragma once

#include "eosio/monitor_api_plugin/httpc.hpp"
#include "eosio/monitor_api_plugin/objects.hpp"

#include <eosio/chain_plugin/chain_plugin.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

namespace eosio {
using namespace eosio::chain;

class monitor_api_plugin_impl
{
public:
    monitor_api_plugin_impl();

    eosio::structures::result push_action(const structures::eos_trx &data, const structures::params &params);

    void monitor_app();

    void set_list_nodes(const vector<string> &nodes);
    void set_list_wallets(const vector<string> &wallets);

    eosio::structures::result clear_list_nodes();
    eosio::structures::result add_nodes(const vector<string> &nodes);
    eosio::structures::result remove_nodes(const vector<string> &nodes);

private:
    template<typename T>
    fc::variant call(const std::string& url, const std::string& path, const T& v);
    fc::variant call(const std::string& url, const std::string& path);

    eosio::chain_apis::read_only::get_info_results get_info(const string &url);
    vector<chain::permission_level> get_account_permissions(const vector<string>& permissions);
    string generate_nonce_string();
    chain::action generate_nonce_action();
    fc::variant determine_required_keys(const string &url, const string &wallet_url, const signed_transaction& trx);

    void sign_transaction(const string &wallet_url, signed_transaction& trx, fc::variant &required_keys, const chain_id_type& chain_id);

    fc::variant push_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    fc::variant push_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    void print_action(const fc::variant& at);
    void print_action_tree(const fc::variant& action);
    void print_result(const fc::variant& result);

    structures::result send_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    void send_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    chain::action create_action(const string &url, const vector<permission_level>& authorization, const account_name& code, const action_name& act, const fc::variant& args);

    fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);

    authority parse_json_authority(const std::string& authorityJsonOrFile);
    authority parse_json_authority_or_key(const std::string& authorityJsonOrFile);

    string is_valid_url(const string &url);


    optional<signature_type> try_sign_digest(const digest_type digest, const public_key_type public_key);

private:
    eosio::client::http::http_context _context;

    bool _no_verify = false;
    bool _is_monitor_app;

    vector<string> _headers;
    vector<string> _nodes;
    vector<string> _wallets;

    string _tx_ref_block_num_or_id;
    bool   _tx_force_unique;
    bool   _tx_dont_broadcast;
    bool   _tx_skip_sign;
    bool   _tx_print_json;
    bool   _print_request;
    bool   _print_response;

    uint8_t  _tx_max_cpu_usage;
    uint32_t _tx_max_net_usage;

};
}



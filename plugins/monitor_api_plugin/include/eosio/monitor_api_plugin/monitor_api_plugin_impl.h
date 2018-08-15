#pragma once

#include "eosio/monitor_api_plugin/httpc.hpp"
#include "eosio/monitor_api_plugin/objects.hpp"

#include <eosio/chain_plugin/chain_plugin.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/network/url.hpp>

#include <queue>

namespace eosio {
using namespace eosio::chain;

class monitor_api_plugin_impl
{
public:
    monitor_api_plugin_impl();

    eosio::structures::result push_action(const structures::transaction_hl &trx_hl);
    bool push_multisig_action(const string &m_node);

    void monitor_app();

    void set_list_nodes(const vector<string> &nodes);
    void set_list_wallets(const vector<string> &wallets);

    eosio::structures::result clear_list_nodes();
    eosio::structures::result add_nodes(const vector<string> &nodes);
    eosio::structures::result remove_nodes(const vector<string> &nodes);

    std::vector<string> get_addr_nodes();

    void update_timeout_monitoring(uint64_t m_timeout);
    void srart_monitoring();
    void stop_monitoring();


    void add_oracle(const structures::oracle &m_oracle);
    void remove_oracle(const std::string &m_oracle_name);

    void msig_params(const structures::msig_params &m_params);
    void approve_msig_contract(const structures::msig_approve &m_obj);

private:
    template<typename T>
    fc::variant call(const std::string& url, const std::string& path, const T& v);
    fc::variant call(const std::string& url, const std::string& path);

    eosio::chain_apis::read_only::get_info_results get_info(const string &url);
    vector<chain::permission_level> get_account_permissions(const vector<string>& permissions);
    string generate_nonce_string();
    chain::action generate_nonce_action();
    fc::variant determine_required_keys(const string &url, const string &wallet_url, const signed_transaction& trx);

    void sign_transaction(const string &wallet_url, signed_transaction& trx, variant &required_keys, const chain_id_type& chain_id);

    fc::variant push_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    fc::variant push_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    void print_action(const fc::variant& at);
    void print_action_tree(const fc::variant& action);
    void print_result(const fc::variant& result);

    structures::result send_actions(const string &url, std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000,
                                    packed_transaction::compression_type compression = packed_transaction::none);
    void send_transaction(const string &url, signed_transaction& trx, int32_t extra_kcpu,
                          packed_transaction::compression_type compression = packed_transaction::none);

    chain::action create_action(const string &url, const vector<permission_level>& authorization, const account_name& code,
                                const action_name& act, const fc::variant& args);

    fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);

    string is_valid_url(const string &url);
    optional<signature_type> try_sign_digest(const digest_type digest, const public_key_type public_key);

    bytes variant_to_bin(const account_name& account, const action_name& action, const fc::variant& action_args_var);


// multisig and trxs HL
    void start_timer();

    string generate_proposal_name();
    void request_list_trxs_hl();
    void timeout_hl();

    void exec_msig_trxs();
    void notify_oracles();

    bool exec_msig(structures::msig_exec &obj);
    void cancel_msig(structures::msig_exec &obj);
    void test_aprove_contr(structures::msig_exec &obj);
    void test_aprove_currency(structures::msig_exec &obj);

private:
    eosio::client::http::http_context _context;

    bool _no_verify = false;
    bool _is_monitor_app;
    bool _is_active_timer;

    uint64_t _timeout;
    boost::asio::high_resolution_timer _timer;

    vector<string> _nodes;
    vector<string> _wallets;
    vector<structures::oracle> _oracles;

    structures::msig_params _msig_params;
    std::queue<eosio::structures::msig_exec> _queue_exec_msig;

    string _tx_ref_block_num_or_id;
    bool   _tx_force_unique;
    bool   _tx_skip_sign;
    bool   _tx_print_json;
    bool   _print_request;
    bool   _print_response;
};
}



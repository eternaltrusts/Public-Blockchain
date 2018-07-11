#pragma once

#include "eosio/monitor_api_plugin/CLI11.hpp"
#include "eosio/monitor_api_plugin/httpc.hpp"

#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <fc/crypto/hex.hpp>
#include <fc/variant.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/log/log_message.hpp>
#include <eosio/utilities/key_conversion.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/contract_types.hpp>

#pragma push_macro("N")
#undef N

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/process/spawn.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/string/classification.hpp>

#pragma pop_macro("N")

namespace eosio {
using namespace eosio::chain;

class monitor_api_plugin_impl
{
public:
    monitor_api_plugin_impl();
    void call_test();

    void push_action();
    void push_transaction();
    void push_transactions();

    void set_list_nodes(const vector<string> &nodes);
    void set_list_wallets(const vector<string> &wallets);

private:
    template<typename T>
    fc::variant call(const std::string& url, const std::string& path, const T& v);
    template<typename T>
    fc::variant call( const std::string& path, const T& v);
    fc::variant call(const std::string& url, const std::string& path);

    eosio::chain_apis::read_only::get_info_results get_info();
    vector<chain::permission_level> get_account_permissions(const vector<string>& permissions);
    string generate_nonce_string();
    chain::action generate_nonce_action();
    fc::variant determine_required_keys(const signed_transaction& trx);
    void sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id);

    fc::variant push_transaction(signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    fc::variant push_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    void print_action(const fc::variant& at);
    void print_action_tree(const fc::variant& action);
    void print_result(const fc::variant& result);

    void send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none);
    void send_transaction(signed_transaction& trx, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none);

    chain::action create_action(const vector<permission_level>& authorization, const account_name& code, const action_name& act, const fc::variant& args);

    fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser);

    authority parse_json_authority(const std::string& authorityJsonOrFile);
    authority parse_json_authority_or_key(const std::string& authorityJsonOrFile);

    string is_valid_url(const string &url);

private:
    eosio::client::http::http_context _context;

    string url = "http://localhost:8888/";
    string wallet_url = "http://localhost:8900/";
    bool no_verify = false;

    vector<string> headers;
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



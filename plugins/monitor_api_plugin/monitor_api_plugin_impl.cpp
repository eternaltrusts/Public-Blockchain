#include "eosio/monitor_api_plugin/monitor_api_plugin_impl.h"

#include <regex>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/wallet_plugin/wallet_manager.hpp>



using namespace std;
using namespace eosio::client::http;

const static std::string default_wallet_url = "http://localhost:8900/";

eosio::monitor_api_plugin_impl::monitor_api_plugin_impl()
    : _is_monitor_app(false)
    , _print_request(false)
    , _print_response(false)
    , _tx_force_unique(false)
    , _tx_skip_sign(false)
    , _tx_print_json(false)
{
    _context = eosio::client::http::create_http_context();
}

eosio::structures::result eosio::monitor_api_plugin_impl::push_action(const eosio::structures::transaction_hl &trx_hl) {
    auto data = trx_hl.trx;
    auto params = trx_hl.params;

    if (!_nodes.size())
        return eosio::structures::result(false, "There is no list of nodes!");

    for (const string addr_node : _nodes) {

        if (!data.is_valid())
            return eosio::structures::result(false, "field is empty transaction_id");

        fc::variant action_args_var;
            try {
                action_args_var = json_from_file_or_string(fc::json::to_string(data), fc::json::relaxed_parser);
            } catch (...) {
                const string &error_msg = "Fail to parse action JSON data='" + fc::json::to_string(data) + "'";
                return eosio::structures::result(false, error_msg);
            }

        auto arg= fc::mutable_variant_object
                ("code", params.contract)  // contract name
                ("action", params.action)  // action
                ("args", action_args_var);
        auto result = call(is_valid_url(addr_node), json_to_bin_func, arg);
        auto accountPermissions = get_account_permissions(params.permissions);

        auto obj = send_actions(is_valid_url(addr_node), {chain::action{accountPermissions, params.contract, params.action, result.get_object()["binargs"].as<bytes>()}});
        if (obj.status)
            return eosio::structures::result(true, "Transactions successful");
    }

    return eosio::structures::result(false, "Requests have not passed");
}


vector<string> tx_permission;
string requested_perm = "[{\"actor\": \"contr\", \"permission\": \"active\"},"
                        " {\"actor\": \"currency\", \"permission\": \"active\"}]";
string transaction_perm = "[{\"actor\": \"currency\", \"permission\": \"active\"}]";
string proposed_transaction = "{\"account\": \"currency\", \"transaction_id\": \"123\"}";

bool eosio::monitor_api_plugin_impl::push_multisig_action(const string &m_node) {
    const string &proposed_contract = "contr";
    const string &proposed_action = "addtrx";
    const string &proposer = "currency";
    unsigned int proposal_expiration_hours = 24;

    fc::variant transaction_perm_var;
    try {
       transaction_perm_var = json_from_file_or_string(transaction_perm);
    } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",transaction_perm))

    fc::variant trx_var;
    try {
       trx_var = json_from_file_or_string(proposed_transaction);
    } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",proposed_transaction))
    bytes proposed_trx_serialized = variant_to_bin( proposed_contract, proposed_action, trx_var );

    vector<permission_level> trxperm;
    try {
       trxperm = transaction_perm_var.as<vector<permission_level>>();
    } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong transaction permissions format: '${data}'", ("data",transaction_perm_var));

    auto accountPermissions = get_account_permissions(tx_permission);
    if (accountPermissions.empty()) {
       if (!proposer.empty()) {
          accountPermissions = vector<permission_level>{{proposer, config::active_name}};
       } else {
          EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <proposer> or -p)");
       }
    }

    fc::variant requested_perm_var;
    if (_oracles.empty()) {
        try {
            requested_perm_var = json_from_file_or_string(requested_perm);
        } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",requested_perm))

        vector<permission_level> reqperm;
        try {
            reqperm = requested_perm_var.as<vector<permission_level>>();
        } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong requested permissions format: '${data}'", ("data",requested_perm_var));
    } else {
        vector<permission_level> requested_perm;
        for (auto oracle : _oracles) {
            requested_perm.push_back(permission_level{oracle, config::active_name});
        }
        requested_perm_var = fc::variant(requested_perm);
    }


    transaction trx;
    trx.expiration = fc::time_point_sec( fc::time_point::now() + fc::hours(proposal_expiration_hours) );
    trx.actions = { chain::action(trxperm, name(proposed_contract), name(proposed_action),
                    proposed_trx_serialized) };

    fc::to_variant(trx, trx_var);

    auto proposal_name = generate_proposal_name();
    auto args = fc::mutable_variant_object()
       ("proposer", proposer )
       ("proposal_name", proposal_name)
       ("requested", requested_perm_var)
       ("trx", trx_var);

    bool status = send_actions(is_valid_url(m_node), {chain::action{accountPermissions, "eosio.msig", "propose",
                                                             variant_to_bin(N(eosio.msig), N(propose), args)}}).status;

    if (status) {
        _list_exec_msig.push_back(eosio::structures::msig_exec{proposal_name, proposer, is_valid_url(m_node), requested_perm, trx});
    }

    return status;
}

void eosio::monitor_api_plugin_impl::monitor_app() {
    _is_monitor_app = true;
}

void eosio::monitor_api_plugin_impl::set_list_nodes(const vector<fc::string> &nodes) {
    for (auto node : nodes)
        _nodes.push_back(is_valid_url(node));
}

void eosio::monitor_api_plugin_impl::set_list_wallets(const vector<string> &wallets) {
    _wallets = wallets;
}

eosio::structures::result eosio::monitor_api_plugin_impl::clear_list_nodes() {
    _nodes.clear();
    return eosio::structures::result(true);
}

eosio::structures::result eosio::monitor_api_plugin_impl::add_nodes(const vector<fc::string> &nodes) {
    for (auto node : nodes) {
        auto it = std::find(_nodes.begin(), _nodes.end(), is_valid_url(node));
        if (it == _nodes.end())
            _nodes.push_back(is_valid_url(node));
    }

    return eosio::structures::result(true);
}

eosio::structures::result eosio::monitor_api_plugin_impl::remove_nodes(const vector<fc::string> &nodes) {
    for (auto node : nodes) {
        auto it = std::find(_nodes.begin(), _nodes.end(), is_valid_url(node));
        if (it != _nodes.end())
            _nodes.erase(it);
    }

    return eosio::structures::result(true);
}

std::vector<string> eosio::monitor_api_plugin_impl::get_addr_nodes() {
    return _nodes;
}

void eosio::monitor_api_plugin_impl::update_timeout_monitoring(uint64_t m_timeout) {

}

void eosio::monitor_api_plugin_impl::srart_monitoring() {

}

void eosio::monitor_api_plugin_impl::stop_monitoring() {

}

template<typename T>
fc::variant eosio::monitor_api_plugin_impl::call(const std::string &url, const std::string &path, const T &v) {
    try {
        vector<string> headers;
        auto cp = new eosio::client::http::connection_param(_context, parse_url(url) + path,
                                                            _no_verify ? false : true, headers);

       return eosio::client::http::do_http_call( *cp, fc::variant(v), _print_request, _print_response );
    }
    catch(boost::system::system_error& e) {
         elog(e.what());
    }
}

fc::variant eosio::monitor_api_plugin_impl::call(const std::string &url, const std::string &path) {
    return call(url, path, fc::variant());
}

eosio::chain_apis::read_only::get_info_results eosio::monitor_api_plugin_impl::get_info(const string &url) {
    return call(url, get_info_func).as<eosio::chain_apis::read_only::get_info_results>();
}

vector<eosio::chain::permission_level> eosio::monitor_api_plugin_impl::get_account_permissions(const vector<string> &permissions) {
    auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
       vector<string> pieces;
       split(pieces, p, boost::algorithm::is_any_of("@"));
       if( pieces.size() == 1 ) pieces.push_back( "active" );
       return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
    });
    vector<chain::permission_level> accountPermissions;
    boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
    return accountPermissions;
}

string eosio::monitor_api_plugin_impl::generate_nonce_string() {
    return fc::to_string(fc::time_point::now().time_since_epoch().count());
}

eosio::chain::action eosio::monitor_api_plugin_impl::generate_nonce_action() {
    return chain::action( {}, eosio::chain::config::null_account_name, "nonce", fc::raw::pack(fc::time_point::now().time_since_epoch().count()));
}

fc::variant eosio::monitor_api_plugin_impl::determine_required_keys(const string &url, const string &wallet_url, const eosio::chain::signed_transaction &trx) {
    fc::variant public_keys;
    if (_is_monitor_app) {
        public_keys = call(wallet_url, wallet_public_keys);
    } else {
        auto& wallet_mgr = app().get_plugin<wallet_plugin>().get_wallet_manager();
        public_keys = fc::variant(wallet_mgr.get_public_keys());
    }

    auto get_arg = fc::mutable_variant_object
            ("transaction", (transaction)trx)
            ("available_keys", public_keys);
    const auto& required_keys = call(url, get_required_keys, get_arg);
    return required_keys["required_keys"];
}

void eosio::monitor_api_plugin_impl::sign_transaction(const string &wallet_url, eosio::chain::signed_transaction &trx,
                                                      fc::variant &required_keys, const eosio::chain::chain_id_type &chain_id) {
    fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id)};
    const auto& signed_trx = call(wallet_url, wallet_sign_trx, sign_args);
    trx = signed_trx.as<signed_transaction>();
}

fc::variant eosio::monitor_api_plugin_impl::push_transaction(const string &url, eosio::chain::signed_transaction &trx,
                                                             int32_t extra_kcpu, eosio::chain::packed_transaction::compression_type compression) {
    auto info = get_info(url);
    trx.expiration = info.head_block_time + fc::seconds(30);

    // Set tapos, default to last irreversible block if it's not specified by the user
    block_id_type ref_block_id = info.last_irreversible_block_id;
    try {
       fc::variant ref_block;
       if (!_tx_ref_block_num_or_id.empty()) {
          ref_block = call(url, get_block_func, fc::mutable_variant_object("block_num_or_id", _tx_ref_block_num_or_id));
          ref_block_id = ref_block["id"].as<block_id_type>();
       }
    } EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id: ${block_num_or_id}", ("block_num_or_id", _tx_ref_block_num_or_id));
    trx.set_reference_block(ref_block_id);

    if (_tx_force_unique) {
       trx.context_free_actions.emplace_back(generate_nonce_action());
    }

    if (!_tx_skip_sign) {
        auto public_keys = determine_required_keys(url, default_wallet_url, trx);
        if(_is_monitor_app) {
            sign_transaction(default_wallet_url, trx, public_keys, info.chain_id);
        } else {
            auto& wallet_mgr = app().get_plugin<wallet_plugin>().get_wallet_manager();
            flat_set<public_key_type> required_keys;
            fc::from_variant(public_keys, required_keys);
            trx = wallet_mgr.sign_transaction(trx, required_keys, info.chain_id);
        }
    }

    return call(url, push_txn_func, packed_transaction(trx, compression));
}

fc::variant eosio::monitor_api_plugin_impl::push_actions(const string &url, std::vector<eosio::chain::action> &&actions,
                                                         int32_t extra_kcpu, eosio::chain::packed_transaction::compression_type compression) {
    signed_transaction trx;
    trx.actions = std::forward<decltype(actions)>(actions);

    return push_transaction(url, trx, extra_kcpu, compression);
}

void eosio::monitor_api_plugin_impl::print_action(const fc::variant &at) {
    const auto& receipt = at["receipt"];
    auto receiver = receipt["receiver"].as_string();
    const auto& act = at["act"].get_object();
    auto code = act["account"].as_string();
    auto func = act["name"].as_string();
    auto args = fc::json::to_string( act["data"] );
    auto console = at["console"].as_string();

    if( args.size() > 100 ) args = args.substr(0,100) + "...";
    cout << "#" << std::setw(14) << right << receiver << " <= " << std::setw(28) << std::left << (code +"::" + func) << " " << args << "\n";
    if( console.size() ) {
       std::stringstream ss(console);
       string line;
       std::getline( ss, line );
       cout << ">> " << line << "\n";
    }
}

void eosio::monitor_api_plugin_impl::print_action_tree(const fc::variant &action) {
    print_action( action );
    const auto& inline_traces = action["inline_traces"].get_array();
    for( const auto& t : inline_traces ) {
       print_action_tree( t );
    }
}

void eosio::monitor_api_plugin_impl::print_result(const fc::variant &result) {
    try {
        if (result.is_object() && result.get_object().contains("processed")) {
           const auto& processed = result["processed"];
           const auto& transaction_id = processed["id"].as_string();
           string status = processed["receipt"].is_object() ? processed["receipt"]["status"].as_string() : "failed";
           int64_t net = -1;
           int64_t cpu = -1;
           if( processed.get_object().contains( "receipt" )) {
              const auto& receipt = processed["receipt"];
              if( receipt.is_object()) {
                 net = receipt["net_usage_words"].as_int64() * 8;
                 cpu = receipt["cpu_usage_us"].as_int64();
              }
           }

           cerr << status << " transaction: " << transaction_id << "  ";
           if( net < 0 ) {
              cerr << "<unknown>";
           } else {
              cerr << net;
           }
           cerr << " bytes  ";
           if( cpu < 0 ) {
              cerr << "<unknown>";
           } else {
              cerr << cpu;
           }

           cerr << " us\n";

           if( status == "failed" ) {
              auto soft_except = processed["except"].as<optional<fc::exception>>();
              if( soft_except ) {
                 edump((soft_except->to_detail_string()));
              }
           } else {
              const auto& actions = processed["action_traces"].get_array();
              for( const auto& a : actions ) {
                 print_action_tree( a );
              }
              wlog( "\rwarning: transaction executed locally, but may not be confirmed by the network yet" );
           }
        } else {
           cerr << fc::json::to_pretty_string( result ) << endl;
        }
    } FC_CAPTURE_AND_RETHROW( (result) ) }

eosio::structures::result eosio::monitor_api_plugin_impl::send_actions(const string &url,
                                                                       std::vector<eosio::chain::action> &&actions, int32_t extra_kcpu,
                                                                       eosio::chain::packed_transaction::compression_type compression) {
        auto result = push_actions(url ,move(actions), extra_kcpu, compression);
        if (result.is_object() && result.get_object().contains("processed"))
            return eosio::structures::result(true, fc::json::to_pretty_string(result));
        else
            return eosio::structures::result(false, fc::json::to_pretty_string(result));
}

void eosio::monitor_api_plugin_impl::send_transaction(const string &url, eosio::chain::signed_transaction &trx,
                                                      int32_t extra_kcpu, eosio::chain::packed_transaction::compression_type compression) {
    auto result = push_transaction(url, trx, extra_kcpu, compression);

    if(_tx_print_json) {
       std::cout << fc::json::to_pretty_string(result) << endl;
    } else {
       print_result(result);
    }
}

eosio::chain::action eosio::monitor_api_plugin_impl::create_action(const string &url, const vector<eosio::chain::permission_level> &authorization,
                                                                   const eosio::chain::account_name &code, const eosio::chain::action_name &act, const fc::variant &args) {
    auto arg = fc::mutable_variant_object()
       ("code", code)
       ("action", act)
       ("args", args);

    auto result = call(url, json_to_bin_func, arg);
    wdump((result)(arg));
    return chain::action{authorization, code, act, result.get_object()["binargs"].as<bytes>()};
 }

fc::variant eosio::monitor_api_plugin_impl::json_from_file_or_string(const fc::string &file_or_str, fc::json::parse_type ptype) {
    regex r("^[ \t]*[\{\[]");
    if (!regex_search(file_or_str, r) && fc::is_regular_file(file_or_str)) {
       return fc::json::from_file(file_or_str, ptype);
    } else {
       return fc::json::from_string(file_or_str, ptype);
    }
 }

fc::string eosio::monitor_api_plugin_impl::is_valid_url(const fc::string &url) {
    regex r("^(ht|f)tp(s?)\:\/\/[0-9a-zA-Z]([-.\w]*[0-9a-zA-Z])*(:(0-9)*)*(\/?)([a-zA-Z0-9\-\.\?\,\'\/\\\+&%\$#_]*)?$");
    if (!regex_search(url, r)) {
        auto valid_url = "http://" + url +"/";
        return valid_url;
    } else {
        if ( url.find_last_of("/") != (url.size() - 1)) {
            auto valid_url = url +"/";
            return valid_url;
        }
    }
    return url;
}

bytes eosio::monitor_api_plugin_impl::variant_to_bin(const account_name &account, const action_name &action, const variant &action_args_var) {
    static unordered_map<account_name, std::vector<char> > abi_cache;
    auto it = abi_cache.find( account );
    if ( it == abi_cache.end() ) {
       const auto result = call(is_valid_url("http://127.0.01:8888"), get_raw_code_and_abi_func, fc::mutable_variant_object("account_name", account));
       std::tie( it, std::ignore ) = abi_cache.emplace( account, result["abi"].as_blob().data );
       //we also received result["wasm"], but we don't use it
    }
    const std::vector<char>& abi_v = it->second;

    abi_def abi;
    if( abi_serializer::to_abi(abi_v, abi) ) {
       abi_serializer abis( abi, fc::seconds(10) );
       auto action_type = abis.get_action_type(action);
       FC_ASSERT(!action_type.empty(), "Unknown action ${action} in contract ${contract}", ("action", action)("contract", account));
       return abis.variant_to_binary(action_type, action_args_var, fc::seconds(10));
    } else {
       FC_ASSERT(false, "No ABI found for ${contract}", ("contract", account));
    }
}

string eosio::monitor_api_plugin_impl::generate_proposal_name() {
    const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

    string name = "";
    auto size_name = rand() % 12 + 1;
    for (auto it = size_name; it >= 0; --it) {
        auto pos_symbol = rand() % 32;
            name += charmap[pos_symbol];
    }

    return name;
}

void eosio::monitor_api_plugin_impl::request_hl() {
    ilog(__FUNCTION__);
}

void eosio::monitor_api_plugin_impl::timeout_hl() {
    exec_msig();

    request_hl();
//    for (auto obj : obj_trx_hl) {
        for (auto node : _nodes) {
            if (push_multisig_action(node))
                break;
        }
//    }

        notify_oracles();
}

void eosio::monitor_api_plugin_impl::exec_msig() {

}

void eosio::monitor_api_plugin_impl::notify_oracles() {

}

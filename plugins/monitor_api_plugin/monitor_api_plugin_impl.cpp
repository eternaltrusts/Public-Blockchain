#include "eosio/monitor_api_plugin/monitor_api_plugin_impl.h"

#include <regex>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

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
    , _tx_print_json(true)
    , _is_active_timer(false)
    , _timeout(1)
    , _timer(app().get_io_service())
    , _last_date_update(boost::posix_time::to_iso_extended_string(boost::posix_time::second_clock::local_time() - boost::posix_time::hours(350)))
{
    _context = eosio::client::http::create_http_context();
    start_timer();
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

void eosio::monitor_api_plugin_impl::monitor_app() {
    _is_monitor_app = true;
}

void eosio::monitor_api_plugin_impl::set_list_nodes(const vector<fc::string> &nodes) {
    for (auto node : nodes)
        _nodes.push_back(is_valid_url(node));
}

void eosio::monitor_api_plugin_impl::set_list_nodes_hl(const vector<string> &nodes) {
    for (auto node : nodes)
        _nodes_hl.push_back(is_valid_url(node));
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

eosio::structures::result eosio::monitor_api_plugin_impl::clear_list_nodes_hl() {
    _nodes_hl.clear();
    return eosio::structures::result(true);
}

eosio::structures::result eosio::monitor_api_plugin_impl::add_nodes_hl(const vector<string> &nodes) {
    for (auto node : nodes) {
        auto it = std::find(_nodes_hl.begin(), _nodes_hl.end(), is_valid_url(node));
        if (it == _nodes_hl.end())
            _nodes_hl.push_back(is_valid_url(node));
    }

    return eosio::structures::result(true);
}

eosio::structures::result eosio::monitor_api_plugin_impl::remove_nodes_hl(const vector<string> &nodes) {
    for (auto node : nodes) {
        auto it = std::find(_nodes_hl.begin(), _nodes_hl.end(), is_valid_url(node));
        if (it != _nodes_hl.end())
            _nodes_hl.erase(it);
    }

    return eosio::structures::result(true);
}

std::vector<string> eosio::monitor_api_plugin_impl::get_addr_nodes() {
    return _nodes;
}

std::vector<string> eosio::monitor_api_plugin_impl::get_addr_nodes_hl() {
    return _nodes_hl;
}

void eosio::monitor_api_plugin_impl::update_timeout_monitoring(uint64_t m_timeout) {
    _timer.cancel();
    _timeout = m_timeout;
    start_timer();
}

void eosio::monitor_api_plugin_impl::srart_monitoring() {
    _is_active_timer = true;
}

void eosio::monitor_api_plugin_impl::stop_monitoring() {
    _is_active_timer = false;
}

void eosio::monitor_api_plugin_impl::add_oracle(const eosio::structures::oracle &m_oracle) {
    _oracles.push_back(m_oracle);
}

void eosio::monitor_api_plugin_impl::remove_oracle(const string &m_oracle_name) {

}

void eosio::monitor_api_plugin_impl::msig_params(const eosio::structures::msig_params &m_params) {
    _msig_params = m_params;
}

bool eosio::monitor_api_plugin_impl::push_propose(const string &m_node, const eosio::structures::hl_obj &obj) {
    try {
        const auto &proposal_name = generate_proposal_name();

        fc::variant requested_perm_var;
        vector<permission_level> requested_perm;
        vector<pair<structures::oracle, structures::msig_approve>> oracles;
        for (auto oracle : generete_random_oracles(m_node)) {
            requested_perm.push_back(permission_level{oracle.name, config::active_name});
            oracles.push_back({oracle, {proposal_name, _msig_params.proposer, oracle.name, m_node}});
        }
        requested_perm_var = fc::variant(requested_perm);

        vector<string> tx_permission;
        auto accountPermissions = get_account_permissions(tx_permission);
        if (accountPermissions.empty()) {
            if (!_msig_params.proposer.empty()) {
                accountPermissions = vector<permission_level>{{_msig_params.proposer, config::owner_name}};
            } else EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <proposer> or -p)");
        }

        auto action_args_var = fc::mutable_variant_object()
                ("proposer", _msig_params.proposer )
                ("proposal_name", proposal_name)
                ("requested", requested_perm_var)
                ("trx", obj.transactionId);

        auto args = fc::mutable_variant_object
                ("code", "et.msig")    // contract name
                ("action", "propose")  // action
                ("args", action_args_var);

        auto result = call(m_node, json_to_bin_func, args);
        auto action_trx = chain::action{accountPermissions, N(et.msig), N(propose), result.get_object()["binargs"].as<bytes>()};

        auto obj_result = send_actions(m_node, {action_trx});
        if (obj_result.status) {
            _queue_msig_params.push(eosio::structures::msig_exec{proposal_name, _msig_params.proposer, m_node, action_trx, 0});
            for (auto obj_pair : oracles)
                notify_oracles(obj_pair.first, obj_pair.second);

            return true;
        }

        return false;
    } catch(fc::exception &e) {
        elog(e.to_detail_string());
        if (e.code() == 3050003) {
            push_propose(m_node, obj);
        }

        return false;
    }
}

void eosio::monitor_api_plugin_impl::push_approve(const eosio::structures::msig_approve &m_obj) {
    auto action_args_var = fc::mutable_variant_object()
            ("proposer", m_obj.proposer)
            ("proposal_name", m_obj.proposal_name)
            ("level", fc::variant(permission_level{name(m_obj.oracle), config::active_name}));

    auto args = fc::mutable_variant_object
            ("code", "et.msig")  // contract name
            ("action", "approve")  // action
            ("args", action_args_var);

    auto accountPermissions = vector<chain::permission_level>{{name(m_obj.oracle), config::active_name}};

    try {
        auto result = call(m_obj.url, json_to_bin_func, args);
        send_actions(m_obj.url, {chain::action{accountPermissions, "et.msig", "approve",
                                               result.get_object()["binargs"].as<bytes>() }});
    } catch(fc::exception &e) {
        elog(e.to_detail_string());
        elog("Сonfirmation error ${e}", ("e", name{m_obj.proposer}));
    }
}

void eosio::monitor_api_plugin_impl::push_cancel(const structures::msig_exec &m_obj) {
    vector<string> tx_permission;
    auto accountPermissions = get_account_permissions(tx_permission);
    if (accountPermissions.empty()) {
        accountPermissions = vector<permission_level>{{m_obj.proposer, config::owner_name}};
    }

    auto action_args_var = fc::mutable_variant_object()
            ("proposer", m_obj.proposer )
            ("proposal_name", m_obj.proposal_name)
            ("canceler", m_obj.proposer);

    auto args = fc::mutable_variant_object
            ("code", "et.msig")  // contract name
            ("action", "cancel")  // action
            ("args", action_args_var);
    try {
        auto result = call(m_obj.url, json_to_bin_func, args);
        send_actions(m_obj.url, {chain::action{accountPermissions, "et.msig", "cancel",
                                               result.get_object()["binargs"].as<bytes>() }});
    } catch(fc::exception &e) {
        elog(e.to_detail_string());
        ilog("Error push cancel. Data: proposer: ${proposer}, proposal name: ${proposal_name}",
             ("proposer", name{m_obj.proposer})("proposal_name", name{m_obj.proposal_name}) );
    }
}

bool eosio::monitor_api_plugin_impl::push_exec(eosio::structures::msig_exec &obj) {
    vector<string> tx_permission;
    auto accountPermissions = get_account_permissions(tx_permission);
    if (accountPermissions.empty()) {
        accountPermissions = vector<permission_level>{{obj.proposer, config::owner_name}};
    }

    auto action_args_var = fc::mutable_variant_object()
            ("proposer", obj.proposer )
            ("proposal_name", obj.proposal_name)
            ("executer", obj.proposer);

    auto args = fc::mutable_variant_object
            ("code", "et.msig")    // contract name
            ("action", "exec")  // action
            ("args", action_args_var);

    try {
        auto result = call(obj.url, json_to_bin_func, args);
        auto obj_result = send_actions(obj.url, {chain::action{accountPermissions, N(et.msig), N(exec), result.get_object()["binargs"].as<bytes>()}});
        if (obj_result.status)
            return true;

        ++obj.counter;
        return false;
    } catch(fc::exception &e) {
        elog(e.to_detail_string());
        ilog("Error push exec. Data: proposer: ${proposer}, proposal name: ${proposal_name}",
             ("proposer", name{obj.proposer})("proposal_name", name{obj.proposal_name}) );
        ++obj.counter;
        return false;
    }
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
    string tx_ref_block_num_or_id;
    try {
       fc::variant ref_block;
       if (!tx_ref_block_num_or_id.empty()) {
          ref_block = call(url, get_block_func, fc::mutable_variant_object("block_num_or_id", tx_ref_block_num_or_id));
          ref_block_id = ref_block["id"].as<block_id_type>();
       }
    } EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id: ${block_num_or_id}", ("block_num_or_id", tx_ref_block_num_or_id));
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

std::string eosio::monitor_api_plugin_impl::is_valid_timestamp(const string &timestamp) {
    std::string new_timestamp(timestamp);
    return new_timestamp.erase(new_timestamp.find('Z'), 1);
}

bytes eosio::monitor_api_plugin_impl::variant_to_bin(const account_name &account, const action_name &action, const variant &action_args_var) {
    static unordered_map<account_name, std::vector<char> > abi_cache;
    auto it = abi_cache.find( account );
    if ( it == abi_cache.end() ) {
        for (auto node : _nodes) {
            try {
                const auto result = call(is_valid_url(node), get_raw_code_and_abi_func, fc::mutable_variant_object("account_name", account));
                std::tie( it, std::ignore ) = abi_cache.emplace( account, result["abi"].as_blob().data );
            } catch(...){}
        }

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

void eosio::monitor_api_plugin_impl::start_timer() {
    _timer.expires_at(boost::asio::high_resolution_timer::clock_type::now() + std::chrono::minutes(_timeout));
    _timer.async_wait([this](const boost::system::error_code &ec) {
        if(ec) return;

        timeout_hl();
        start_timer();
    });
}

string eosio::monitor_api_plugin_impl::generate_proposal_name() {
    const char* charmap = "12345abcdefghijklmnopqrstuvwxyz";

    string name = "";
    auto size_name = rand() % 12;
    for (auto it = size_name; it >= 0; --it) {
        auto pos_symbol = rand() % 31;
            name += charmap[pos_symbol];
    }

    return name;
}

vector<eosio::structures::oracle> eosio::monitor_api_plugin_impl::generete_random_oracles(const std::string &m_url) {
    vector<structures::oracle> oracles;

    if(_msig_params.is_local) {
        if (_oracles.size() < 5)
            EOS_THROW(abort_called, "Not oralces");
        while (oracles.size() < 5) {
            auto pos = rand() % _oracles.size();
            auto it_pos = std::find_if(oracles.begin(), oracles.end(), [&](const auto &obj){
                if (obj.name == _oracles.at(pos).name)
                    return true;
                return false;
            });

            if (it_pos != oracles.end())
                continue;
            oracles.push_back(_oracles.at(pos));
        }
    } else {
        auto result = call(m_url, get_table_func, fc::mutable_variant_object("json", "true")
                           ("code","et.token")
                           ("scope","et.token")
                           ("table","oracles")
                           ("table_key","")
                           ("lower_bound","")
                           ("upper_bound","")
                           ("limit","1000")
                           ("key_type","")
                           ("index_position", "")
                           ("encode_type", "dec")
                           );

        if (result.is_object() && result.get_object().contains("rows")) {
            oracles = result.get_object()["rows"].as<vector<structures::oracle>>();
        }
    }

    if (oracles.size() < 5)
        EOS_THROW(abort_called, "Not list oralces");

    return oracles;
}

fc::variant eosio::monitor_api_plugin_impl::request_list_trxs_hl() {
    for (auto node_hl : _nodes_hl) {
        try {
            vector<string> headers;
            auto cp = new eosio::client::http::connection_param(_context, parse_url(node_hl), _no_verify ? false : true, headers);
            return eosio::client::http::get_request( *cp, {{"startDate",_last_date_update}}, _print_request, _print_response );
        }
        catch(boost::system::system_error& e) {
            elog(e.what());
        }
    }
}

void eosio::monitor_api_plugin_impl::timeout_hl() {
    if (!_is_active_timer)
        return;

    exec_msig_trxs();

    for (auto obj : request_list_trxs_hl().as<std::vector<structures::hl_obj>>()) {
        boost::posix_time::ptime obj_time = boost::posix_time::from_iso_extended_string(is_valid_timestamp(obj.timestamp));
        boost::posix_time::ptime last_update = boost::posix_time::from_iso_extended_string(_last_date_update);

        if (obj_time > last_update) {
            _last_date_update = boost::posix_time::to_iso_extended_string(obj_time + boost::posix_time::milliseconds(1));
        }

        for (auto node : _nodes) {
            if(push_propose(is_valid_url(node), obj))
                break;
        }
    }
}

void eosio::monitor_api_plugin_impl::exec_msig_trxs() {
    while(!_queue_msig_params.empty()) {
        auto obj = _queue_msig_params.front();
        _queue_msig_params.pop();
        if (!push_exec(obj)) {
            if (obj.counter <= 2) {  // TODO repeat request exec
                _queue_msig_params.push(obj);
            } else {
                push_cancel(obj);
            }
        }
    }
}

void eosio::monitor_api_plugin_impl::notify_oracles(const structures::oracle &m_oracle,
                                                    const structures::msig_approve &m_approve) {
    try {
        call(m_oracle.url, approve_msig, m_approve);
    } catch(...) {
        elog("error oracle app");
    }
}

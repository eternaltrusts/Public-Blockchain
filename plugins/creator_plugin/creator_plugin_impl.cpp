#include "eosio/creator_plugin/creator_plugin_impl.h"

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
using namespace eosio;
using namespace eosio::client::http;

const static eosio::account_name creator = N(etrusts12crt);
const static std::string default_url = "http://dev.cryptolions.io:38888"; //"http://127.0.0.1:8888/";

const static std::string default_wallet_url = "http://127.0.0.1:8900/";

eosio::creator_plugin_impl::creator_plugin_impl()
    : _print_request(false)
    , _print_response(false)
    , _tx_force_unique(false)
    , _tx_skip_sign(false)
    , _tx_print_json(true)
    , _is_creator_app(false)
{
    _context = eosio::client::http::create_http_context();
}

void creator_plugin_impl::tester_app() {
    _is_creator_app = true;
}

structures::result_create_account eosio::creator_plugin_impl::create_account(const structures::create_account &name_accaunt) {
    structures::result_create_account result(name_accaunt.account);
    try {
        call(default_url, get_account_func, fc::mutable_variant_object("account_name", name_accaunt.account));
        return result;
    } catch (...) {
        generate_key(result);
        create_wallet(result);
        import_key(result);
        cretae_account_in_eosio(result);
        result.status = true;

        return result;
    }
}

void eosio::creator_plugin_impl::create_wallet(structures::result_create_account &obj) {
    const auto& v = call(default_wallet_url, wallet_create, obj.account);
    obj.password = v.get_string();
}

void eosio::creator_plugin_impl::generate_key(structures::result_create_account &obj) {
    auto owner_pk = private_key_type::generate();
    obj.owner_private_key = string(owner_pk);
    obj.owner_public_key = string(owner_pk.get_public_key());

    auto active_pk = private_key_type::generate();
    obj.active_private_key = string(active_pk);
    obj.active_public_key = string(active_pk.get_public_key());
}

void eosio::creator_plugin_impl::import_key(const structures::result_create_account &obj) {
    private_key_type owner_wallet_key = private_key_type( obj.owner_private_key );
    private_key_type active_wallet_key = private_key_type( obj.active_private_key );

    fc::variants vs_owner = {fc::variant(obj.account), fc::variant(owner_wallet_key)};
    call(default_wallet_url, wallet_import_key, vs_owner); // TODO owner keys

    fc::variants vs_active = {fc::variant(obj.account), fc::variant(active_wallet_key)};
    call(default_wallet_url, wallet_import_key, vs_active); // TODO active keys
}

void eosio::creator_plugin_impl::cretae_account_in_eosio(structures::result_create_account &obj) {
    public_key_type owner_key, active_key;
    try {
       owner_key = public_key_type(obj.owner_public_key);
    } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid owner public key: ${public_key}", ("public_key", obj.owner_public_key));
    try {
       active_key = public_key_type(obj.active_public_key);
    } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid active public key: ${public_key}", ("public_key", obj.active_public_key));

    auto create = create_newaccount(creator, chain::name(obj.account), owner_key, active_key);
    action buyram = create_buyram(creator, chain::name(obj.account), to_asset("1.0000 EOS"));

    auto net = to_asset("0.0010 EOS");
    auto cpu = to_asset("0.0010 EOS");
    if ( net.get_amount() != 0 || cpu.get_amount() != 0 ) {
        action delegate = create_delegate( creator, chain::name(obj.account), net, cpu, true);
        auto test = send_actions( default_url, { create, buyram, delegate } );
        ilog("");
    } else {
        auto test = send_actions( default_url, { create, buyram } );
        ilog("");
    }
}

template<typename T>
fc::variant eosio::creator_plugin_impl::call(const std::string &url, const std::string &path, const T &v) {
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

fc::variant eosio::creator_plugin_impl::call(const std::string &url, const std::string &path) {
    return call(url, path, fc::variant());
}

eosio::chain_apis::read_only::get_info_results eosio::creator_plugin_impl::get_info(const string &url) {
    return call(url, get_info_func).as<eosio::chain_apis::read_only::get_info_results>();
}

vector<eosio::chain::permission_level> eosio::creator_plugin_impl::get_account_permissions(const vector<string> &permissions) {
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

eosio::chain::action eosio::creator_plugin_impl::generate_nonce_action() {
    return chain::action( {}, eosio::chain::config::null_account_name, "nonce", fc::raw::pack(fc::time_point::now().time_since_epoch().count()));
}

fc::variant eosio::creator_plugin_impl::determine_required_keys(const string &url, const string &wallet_url, const eosio::chain::signed_transaction &trx) {
    fc::variant public_keys;
    if (_is_creator_app) {
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

void eosio::creator_plugin_impl::sign_transaction(const string &wallet_url, eosio::chain::signed_transaction &trx,
                                                      fc::variant &required_keys, const eosio::chain::chain_id_type &chain_id) {
    fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id)};
    const auto& signed_trx = call(wallet_url, wallet_sign_trx, sign_args);
    trx = signed_trx.as<signed_transaction>();
}

fc::variant eosio::creator_plugin_impl::push_transaction(const string &url, eosio::chain::signed_transaction &trx,
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
        if(_is_creator_app) {
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

fc::variant eosio::creator_plugin_impl::push_actions(const string &url, std::vector<eosio::chain::action> &&actions,
                                                         int32_t extra_kcpu, eosio::chain::packed_transaction::compression_type compression) {
    signed_transaction trx;
    trx.actions = std::forward<decltype(actions)>(actions);

    return push_transaction(url, trx, extra_kcpu, compression);
}

void eosio::creator_plugin_impl::print_action(const fc::variant &at) {
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

void eosio::creator_plugin_impl::print_action_tree(const fc::variant &action) {
    print_action( action );
    const auto& inline_traces = action["inline_traces"].get_array();
    for( const auto& t : inline_traces ) {
       print_action_tree( t );
    }
}

void eosio::creator_plugin_impl::print_result(const fc::variant &result) {
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

structures::result eosio::creator_plugin_impl::send_actions(const string &url,
                                                            std::vector<eosio::chain::action> &&actions, int32_t extra_kcpu,
                                                            eosio::chain::packed_transaction::compression_type compression) {
    auto result = push_actions(url ,move(actions), extra_kcpu, compression);
    if (result.is_object() && result.get_object().contains("processed"))
        return structures::result(true, fc::json::to_pretty_string(result));
    else
        return structures::result(false, fc::json::to_pretty_string(result));
}

void eosio::creator_plugin_impl::send_transaction(const string &url, eosio::chain::signed_transaction &trx,
                                                      int32_t extra_kcpu, eosio::chain::packed_transaction::compression_type compression) {
    auto result = push_transaction(url, trx, extra_kcpu, compression);

    if(_tx_print_json) {
       std::cout << fc::json::to_pretty_string(result) << endl;
    } else {
       print_result(result);
    }
}

eosio::chain::action eosio::creator_plugin_impl::create_action(const string &url, const vector<eosio::chain::permission_level> &authorization,
                                                                   const eosio::chain::account_name &code, const eosio::chain::action_name &act, const fc::variant &args) {
    auto arg = fc::mutable_variant_object()
       ("code", code)
       ("action", act)
       ("args", args);

    auto result = call(url, json_to_bin_func, arg);
    wdump((result)(arg));
    return chain::action{authorization, code, act, result.get_object()["binargs"].as<bytes>()};
 }

fc::variant eosio::creator_plugin_impl::json_from_file_or_string(const fc::string &file_or_str, fc::json::parse_type ptype) {
    regex r("^[ \t]*[\{\[]");
    if (!regex_search(file_or_str, r) && fc::is_regular_file(file_or_str)) {
       return fc::json::from_file(file_or_str, ptype);
    } else {
       return fc::json::from_string(file_or_str, ptype);
    }
 }

fc::string eosio::creator_plugin_impl::is_valid_url(const fc::string &url) {
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

std::string eosio::creator_plugin_impl::is_valid_timestamp(const string &timestamp) {
    std::string new_timestamp(timestamp);
    return new_timestamp.erase(new_timestamp.find('Z'), 1);
}

bytes eosio::creator_plugin_impl::variant_to_bin(const account_name &account, const action_name &action, const variant &action_args_var) {
    static unordered_map<account_name, std::vector<char> > abi_cache;
    auto it = abi_cache.find( account );
    if ( it == abi_cache.end() ) {
            try {
            // TODO need fix
                const auto result = call(is_valid_url(default_url), get_raw_code_and_abi_func, fc::mutable_variant_object("account_name", account));
                std::tie( it, std::ignore ) = abi_cache.emplace( account, result["abi"].as_blob().data );
            } catch(...){}
        }

    //we also received result["wasm"], but we don't use it
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

asset creator_plugin_impl::to_asset(account_name code, const string &s) {
    static map< pair<account_name, eosio::chain::symbol_code>, eosio::chain::symbol> cache;
    auto a = asset::from_string( s );
    eosio::chain::symbol_code sym = a.get_symbol().to_symbol_code();
    auto it = cache.find( make_pair(code, sym) );
    auto sym_str = a.symbol_name();
    if ( it == cache.end() ) {
        auto json = call(default_url, get_currency_stats_func, fc::mutable_variant_object("json", false)
                         ("code", code)
                         ("symbol", sym_str)
                         );
       auto obj = json.get_object();
       auto obj_it = obj.find( sym_str );
       if (obj_it != obj.end()) {
          auto result = obj_it->value().as<eosio::chain_apis::read_only::get_currency_stats_result>();
          auto p = cache.emplace( make_pair( code, sym ), result.max_supply.get_symbol() );
          it = p.first;
       } else {
          EOS_THROW(symbol_type_exception, "Symbol ${s} is not supported by token contract ${c}", ("s", sym_str)("c", code));
       }
    }
    auto expected_symbol = it->second;
    if ( a.decimals() < expected_symbol.decimals() ) {
       auto factor = expected_symbol.precision() / a.precision();
       auto a_old = a;
       a = asset( a.get_amount() * factor, expected_symbol );
    } else if ( a.decimals() > expected_symbol.decimals() ) {
       EOS_THROW(symbol_type_exception, "Too many decimal digits in ${a}, only ${d} supported", ("a", a)("d", expected_symbol.decimals()));
    } // else precision matches
    return a;
}

asset creator_plugin_impl::to_asset(const string &s) {
    return to_asset( N(eosio.token), s );
}

action creator_plugin_impl::create_newaccount(const name &creator, const name &newaccount, public_key_type owner, public_key_type active) {
    return action {
       vector<eosio::chain::permission_level>{{creator, config::active_name}},
       eosio::chain::newaccount{
          .creator      = creator,
          .name         = newaccount,
          .owner        = chain::authority{1, {{owner, 1}}, {}},
          .active       = chain::authority{1, {{active, 1}}, {}}
       }
    };
}

action creator_plugin_impl::create_action(const vector<permission_level> &authorization, const account_name &code, const action_name &act, const variant &args) {
    return chain::action{authorization, code, act, variant_to_bin(code, act, args)};
}

action creator_plugin_impl::create_buyram(const name &creator, const name &newaccount, const asset &quantity) {
    fc::variant act_payload = fc::mutable_variant_object()
          ("payer", creator.to_string())
          ("receiver", newaccount.to_string())
          ("quant", quantity.to_string());
    return create_action(vector<chain::permission_level>{{creator,config::active_name}},
                         config::system_account_name, N(buyram), act_payload);
}

action creator_plugin_impl::create_delegate(const name &from, const name &receiver, const asset &net, const asset &cpu, bool transfer) {
    fc::variant act_payload = fc::mutable_variant_object()
          ("from", from.to_string())
          ("receiver", receiver.to_string())
          ("stake_net_quantity", net.to_string())
          ("stake_cpu_quantity", cpu.to_string())
          ("transfer", transfer);
    return create_action(vector<chain::permission_level>{{from,config::active_name}},
                         config::system_account_name, N(delegatebw), act_payload);
}

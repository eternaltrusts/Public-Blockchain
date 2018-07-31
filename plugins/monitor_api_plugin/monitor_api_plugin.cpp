/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/monitor_api_plugin/monitor_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}


#define INVOKE_V_V(api_handle, call_name) \
     api_handle->call_name(); \
     eosio::structures::empty_responce result;

#define INVOKE_R_V(api_handle, call_name) \
     auto result = api_handle->call_name();

#define INVOKE_R_LR(api_handle, call_name, in_param0) \
     const auto& list = fc::json::json::from_string(body).as<in_param0>(); \
     auto result = api_handle->call_name(list);

#define INVOKE_R_OR(api_handle, call_name, in_param0) \
     const auto& vs = fc::json::json::from_string(body).as<in_param0>(); \
     auto result = api_handle->call_name(vs);

#define INVOKE_R_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     auto result = api_handle->call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>());


namespace eosio {
   static appbase::abstract_plugin& _monitor_api_plugin = app().register_plugin<monitor_api_plugin>();

monitor_api_plugin::monitor_api_plugin()
    : _monitor_api_plugin_impl(new monitor_api_plugin_impl())
{}

void monitor_api_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
           ("addr_node", bpo::value< vector<string> >()->composing(), "Node(s) to enable, may be specified multiple times")
           ("addr_wallet", bpo::value< vector<string> >()->composing(), "Wallet to enable")
           ;
}

void monitor_api_plugin::plugin_initialize(const variables_map& options) {
   _monitor_api_plugin_impl.reset(new monitor_api_plugin_impl);
   if(options.count("addr_node")) {
       auto nodes = options.at("addr_node").as<std::vector<std::string>>();
       _monitor_api_plugin_impl->set_list_nodes(nodes);
   }
   if(options.count("addr_wallet")) {
       auto wallets = options.at("addr_wallet").as<std::vector<std::string>>();
       _monitor_api_plugin_impl->set_list_wallets(wallets);
   }
}

void monitor_api_plugin::plugin_startup() {
    ilog("starting monitor_api_plugin");

    app().get_plugin<http_plugin>().add_api({
        CALL(monitor, _monitor_api_plugin_impl, list_nodes, INVOKE_R_V(_monitor_api_plugin_impl, get_addr_nodes), 200),
        CALL(monitor, _monitor_api_plugin_impl, clear_list_nodes, INVOKE_R_V(_monitor_api_plugin_impl, clear_list_nodes), 200),
        CALL(monitor, _monitor_api_plugin_impl, add_nodes, INVOKE_R_LR(_monitor_api_plugin_impl, add_nodes, vector<string>), 200),
        CALL(monitor, _monitor_api_plugin_impl, remove_nodes, INVOKE_R_LR(_monitor_api_plugin_impl, remove_nodes, vector<string>), 200),
        CALL(monitor, _monitor_api_plugin_impl, push, INVOKE_R_OR(_monitor_api_plugin_impl, push_action, eosio::structures::transaction_hl), 200)
    });
}

void monitor_api_plugin::plugin_shutdown() {
}

void monitor_api_plugin::monitor_app() {
    _monitor_api_plugin_impl->monitor_app();
}

}

#undef INVOKE_R_R_R
#undef INVOKE_R_OR
#undef INVOKE_R_LR
#undef INVOKE_R_V
#undef INVOKE_V_V
#undef CALL

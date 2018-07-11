/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/monitor_api_plugin/monitor_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include "eosio/monitor_api_plugin/monitor_api_plugin_impl.h"

namespace eosio { namespace detail {
  struct monitor_api_plugin_empty {};
}}

FC_REFLECT(eosio::detail::monitor_api_plugin_empty, );

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
     eosio::detail::monitor_api_plugin_empty result;



namespace eosio {
   static appbase::abstract_plugin& _monitor_api_plugin = app().register_plugin<monitor_api_plugin>();


monitor_api_plugin::monitor_api_plugin()
    : my(new monitor_api_plugin_impl())
{}


void monitor_api_plugin::set_program_options(options_description&, options_description& cfg) {
//   cfg.add_options()
//         ("option-name", bpo::value<string>()->default_value("default value"),
//          "Option Description")
//         ;
}

void monitor_api_plugin::plugin_initialize(const variables_map& options) {
   my.reset(new monitor_api_plugin_impl);
   if(options.count("option-name")) {
      // Handle the option
   }
}

void monitor_api_plugin::plugin_startup() {
    ilog("starting monitor_api_plugin");

    app().get_plugin<http_plugin>().add_api({
         CALL(monitor, my, call, INVOKE_V_V(my, call_test), 200)
    });
}

void monitor_api_plugin::plugin_shutdown() {
}

#undef INVOKE_V_V
#undef CALL

}

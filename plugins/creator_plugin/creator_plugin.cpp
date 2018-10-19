/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/creator_plugin/creator_plugin.hpp>
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

#define INVOKE_V_R(api_handle, call_name, in_param0) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle->call_name(vs.at(0).as<in_param0>()); \
     eosio::structures::empty_responce result;

#define INVOKE_V_OR(api_handle, call_name, in_param0) \
     const auto& vs = fc::json::json::from_string(body).as<in_param0>(); \
     api_handle->call_name(vs); \
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
   static appbase::abstract_plugin& _creator_plugin = app().register_plugin<creator_plugin>();

creator_plugin::creator_plugin()
    : _creator_plugin_impl(new creator_plugin_impl())
{}

void creator_plugin::set_program_options(options_description&, options_description& cfg) {
}

void creator_plugin::plugin_initialize(const variables_map& options) {
   _creator_plugin_impl.reset(new creator_plugin_impl);
}

void creator_plugin::plugin_startup() {
    ilog("starting creator_plugin");

    app().get_plugin<http_plugin>().add_api({
        CALL(creator, _creator_plugin_impl, create_account, INVOKE_R_OR(_creator_plugin_impl, create_account, structures::create_account), 200)
    });
}

void creator_plugin::plugin_shutdown() {}

void creator_plugin::tester_app() {
    _creator_plugin_impl->tester_app();
}

}

#undef INVOKE_R_R_R
#undef INVOKE_R_OR
#undef INVOKE_R_LR
#undef INVOKE_R_V
#undef INVOKE_V_V
#undef CALL

/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;


/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class monitor_api_plugin : public appbase::plugin<monitor_api_plugin> {
public:
    monitor_api_plugin();
    virtual ~monitor_api_plugin() = default;

    APPBASE_PLUGIN_REQUIRES(/*(http_plugin)(wallet_plugin)*/)

    virtual void set_program_options(options_description&, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    void monitor_app();

private:
    bool _is_monitor_app;
    std::unique_ptr<class monitor_api_plugin_impl> _monitor_api_plugin_impl;
};

}

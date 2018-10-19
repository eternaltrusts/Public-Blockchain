/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <appbase/application.hpp>

#include "eosio/creator_plugin/creator_plugin_impl.h"

namespace eosio {

using namespace appbase;


/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class creator_plugin : public appbase::plugin<creator_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES()

    creator_plugin();
    virtual ~creator_plugin() = default;

    virtual void set_program_options(options_description&, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

    void tester_app();

private:
    std::unique_ptr<class creator_plugin_impl> _creator_plugin_impl;
};

}

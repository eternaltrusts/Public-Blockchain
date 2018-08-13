/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include "oracle_api_plugin_impl.hpp"

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class oracle_api_plugin : public appbase::plugin<oracle_api_plugin> {
public:
   oracle_api_plugin();
   virtual ~oracle_api_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<oracle_api_plugin_impl> _oracle_api_plugin_impl;
};

}

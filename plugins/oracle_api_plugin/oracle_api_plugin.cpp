/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosio/oracle_api_plugin/oracle_api_plugin.hpp"
#include <fc/exception/exception.hpp>

namespace eosio {
   static appbase::abstract_plugin& _oracle_api_plugin = app().register_plugin<oracle_api_plugin>();


oracle_api_plugin::oracle_api_plugin()
    : _oracle_api_plugin_impl(new oracle_api_plugin_impl())
{}

oracle_api_plugin::~oracle_api_plugin(){}

void oracle_api_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void oracle_api_plugin::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
   }
   FC_LOG_AND_RETHROW()
}

void oracle_api_plugin::plugin_startup() {
   // Make the magic happen
}

void oracle_api_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}

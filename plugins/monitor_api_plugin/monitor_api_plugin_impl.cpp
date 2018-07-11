#include "eosio/monitor_api_plugin/monitor_api_plugin_impl.h"
#include <eosio/chain/exceptions.hpp>

#include "eosio/monitor_api_plugin/CLI11.hpp"
#include "eosio/monitor_api_plugin/httpc.hpp"

#include <vector>
#include <string>

using namespace std;
using namespace eosio::client::http;

string url = "http://localhost:8888/";
string wallet_url = "http://localhost:8900/";
bool no_verify = false;
vector<string> headers;

auto   tx_expiration = fc::seconds(30);
string tx_ref_block_num_or_id;
bool   tx_force_unique = false;
bool   tx_dont_broadcast = false;
bool   tx_skip_sign = false;
bool   tx_print_json = false;
bool   print_request = false;
bool   print_response = false;

eosio::client::http::http_context context;

template<typename T>
fc::variant call( const std::string& url,
                  const std::string& path,
                  const T& v ) {
   try {
      eosio::client::http::connection_param *cp = new eosio::client::http::connection_param(context, parse_url(url) + path,
              no_verify ? false : true, headers);

      return eosio::client::http::do_http_call( *cp, fc::variant(v), print_request, print_response );
   }
   catch(boost::system::system_error& e) {
        ilog(e.what());
//      if(url == ::url)
//         std::cerr << localized("Failed to connect to nodeos at ${u}; is nodeos running?", ("u", url)) << std::endl;
//      else if(url == ::wallet_url)
//         std::cerr << localized("Failed to connect to keosd at ${u}; is keosd running?", ("u", url)) << std::endl;
//      throw connection_exception(fc::log_messages{FC_LOG_MESSAGE(error, e.what())});
   }
}

void eosio::monitor_api_plugin_impl::call() {
    ilog(__FUNCTION__);


}

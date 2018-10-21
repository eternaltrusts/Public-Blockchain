#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosio {

   class multisig : public contract {
   private:
      struct transaction_object
      {
          account_name contract;
          action_name  action;
          token::transfer_args packed_transaction;
      };

      struct proposal {
         name                       proposal_name;
         account_name               proposer;
         uint8_t                    number_of_confirmations;
         transaction_object         struct_transaction;

         auto primary_key()const { return proposal_name.value; }
      };
      typedef eosio::multi_index<N(proposal), proposal> proposals;

      struct approvals_info {
         name                       proposal_name;
         vector<permission_level>   requested_approvals;
         vector<permission_level>   provided_approvals;

         vector<permission_level>   mandatory_requested_approvals;
         vector<permission_level>   mandatory_provided_approvals;

         auto primary_key()const { return proposal_name.value; }
      };
      typedef eosio::multi_index<N(approvals), approvals_info> approvals;

   public:
      multisig( account_name self ):contract(self){}
      void propose(account_name proposer, name proposal_name, vector<permission_level> mandatory_list, vector<permission_level> oracles,
                   uint8_t number_of_confirmations, transaction_object transaction_obj );
      void approve( name proposal_name, permission_level level );
      void unapprove( name proposal_name, permission_level level );
      void cancel( name proposal_name, account_name canceler );
      void exec( name proposal_name, account_name executer );
   };

} /// namespace eosio

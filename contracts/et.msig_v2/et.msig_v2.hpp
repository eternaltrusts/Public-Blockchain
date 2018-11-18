#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosio {

   class multisig_output_limits : public contract {
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
         transaction_object         struct_transaction;

         auto primary_key()const { return proposal_name.value; }
      };
      typedef eosio::multi_index<N(proposal), proposal> proposals;

      struct approvals_info {
         name                       proposal_name;
         vector<permission_level>   requested_approvals;
         vector<permission_level>   provided_approvals;

         auto primary_key()const { return proposal_name.value; }
      };
      typedef eosio::multi_index<N(approvals), approvals_info> approvals;

      struct st_limit {
            asset amount;
            vector<permission_level> requested_approvals;
      };

      struct limits_info {
          account_name account;
          vector<st_limit> limit;

          auto primary_key()const { return account; }
      };
      typedef eosio::multi_index<N(limits), limits_info> limits;

   public:
      multisig_output_limits( account_name self ):contract(self){}
      void crtlimits(account_name name, vector<st_limit> vlimits);
      void updlimits(account_name name, vector<st_limit> vlimits);
      void dellimits(account_name name);
      void propose(account_name proposer, name proposal_name, transaction_object transaction_obj);
      void approve( name proposal_name, permission_level level );
      void unapprove( name proposal_name, permission_level level );
      void cancel( name proposal_name, account_name canceler );
      void exec( name proposal_name, account_name executer );
   };

} /// namespace eosio

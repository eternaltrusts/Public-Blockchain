import os
import config

def open_account(account, password):
  try:
    command_unlock = "cleos unlock -n " +account+ " --password " +password
    os.system(command_unlock)
    return True;
  except Exception:
    print("this account unlocked")
    return False;

def deploy_contract_msig_v1(account):
  command_deploy = "cleos " +config.url+ " set contract " +account+ " /root/contracts/et.msig -p " +account+"@owner"
  os.system(command_deploy)
    
def deploy_contract_msig_v2(account):
  command_deploy = "cleos " +config.url+ " set contract " +account+ " /root/contracts/et.msig_v2 -p " +account+"@owner"
  os.system(command_deploy)
  

def set_permissions(account):
  command = "cleos " +config.url+ " set account permission " +account+ " active '{\"threshold\":1, \"accounts\":[{\"permission\":{\"actor\":\"" +account+ "\",\"permission\":\"eosio.code\"}, \"weight\":1}]}' owner -p " +account+"@owner"
  os.system(command)
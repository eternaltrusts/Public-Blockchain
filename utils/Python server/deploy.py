import methods

def deploy_v1(account):
  if methods.open_account(account['account'], account['password']):
    methods.set_permissions(account['account'])
    methods.deploy_contract_msig_v1(account['account'])
    return {"status":"successful"}
  return {"status":"error"}
    

def deploy_v2(account):
  if methods.open_account(account['account'], account['password']):
    methods.set_permissions(account['account'])
    methods.deploy_contract_msig_v2(account['account'])
    return {"status":"successful"}
  return {"status":"error"}
    
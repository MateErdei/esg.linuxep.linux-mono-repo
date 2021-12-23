import hvac
import argparse
import os

parser = argparse.ArgumentParser()

parser.add_argument("--vault-address", type=str,
                    default= "https://euw2.vault.sst.sophosapps.com/",
                    help = "The address of the vault server.")

parser.add_argument("--vault-token", type=str,
                    help = "The token used for authenticating with Vault API.")

def get_vault_secret(path, mount_point, client, password_name):
    try:
        secret = client.secrets.kv.v1.read_secret(path=path, mount_point=mount_point)

    except hvac.v1.exceptions.VaultError as err:
        print("Error encountered: {0}".format(err))
        exit(1)
    else:
        data = secret['data']
        return data[password_name]

args = parser.parse_args()
client = hvac.Client(url=args.vault_address)
client.token = args.vault_token
my_path = 'esg/ukdevvlx/fuzzer_secrets'
mount_point = 'kv'
sec_name = ''

token = get_vault_secret(my_path,mount_point, client, 'token')
print(token)
os.environ["TAP_JWT"] = token
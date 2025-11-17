import requests

from stellar_sdk import Asset, Network, utils
from stellar_sdk import xdr as stellar_xdr

LOBSTR_TOKEN_API_URL = "https://lobstr.co/api/v1/sep/assets/curated.json"
SOROSWAP_TOKEN_API_URL = (
    "https://raw.githubusercontent.com/soroswap/token-list/main/tokenList.json"
)
STELLAR_EXPERT_TOKEN_API_URL = (
    "https://api.stellar.expert/explorer/public/asset/?limit=200&order=desc&sort=rating"
)

lobstr_tokens = set()
soroswap_tokens = set()
stellar_expert_tokens = set()

resp = requests.get(LOBSTR_TOKEN_API_URL).json()
for record in resp["assets"]:
    asset = Asset(record["code"], record["issuer"])
    lobstr_tokens.add(asset)

resp = requests.get(SOROSWAP_TOKEN_API_URL).json()
for record in resp["assets"]:
    asset = Asset(record["code"], record["issuer"])
    soroswap_tokens.add(asset)

resp = requests.get(STELLAR_EXPERT_TOKEN_API_URL).json()
for record in resp["_embedded"]["records"]:
    asset = record["asset"]
    if len(asset.split("-")) == 1:
        continue
    else:
        asset_code, asset_issuer = asset.split("-")[:2]
        asset = Asset(asset_code, asset_issuer)
        stellar_expert_tokens.add(asset)

# Find tokens that are common to at least two of the lists.
tokens_in_at_least_two = (
    (lobstr_tokens & soroswap_tokens)
    | (lobstr_tokens & stellar_expert_tokens)
    | (soroswap_tokens & stellar_expert_tokens)
)


def get_asset_contract_id(asset: Asset, network_passphrase: str) -> bytes:
    """Get the contract id of the wrapped token contract."""
    network_id_hash = stellar_xdr.Hash(Network(network_passphrase).network_id())
    data = stellar_xdr.HashIDPreimage(
        stellar_xdr.EnvelopeType.ENVELOPE_TYPE_CONTRACT_ID,
        contract_id=stellar_xdr.HashIDPreimageContractID(
            network_id=network_id_hash,
            contract_id_preimage=stellar_xdr.ContractIDPreimage(
                stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ASSET,
                from_asset=asset.to_xdr_object(),
            ),
        ),
    )
    contract_id = utils.sha256(data.to_xdr_bytes())
    return contract_id


def print_asset(asset):
    contract_id = get_asset_contract_id(asset, Network.PUBLIC_NETWORK_PASSPHRASE)
    print(f"// {asset.code}-{asset.issuer}")
    print("TokenInfo {")
    print("    contract_address: [")
    print("        " + "".join([f"0x{x:02x}, " for x in contract_id]))
    print("    ],")
    print(f'    symbol: "{asset.code}",')
    print("    decimals: 7,")
    print("},")


# Sort the set of assets alphabetically by their code before printing.
for asset in sorted(tokens_in_at_least_two, key=lambda asset: asset.code):
    print_asset(asset)

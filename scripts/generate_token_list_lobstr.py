import requests

from stellar_sdk import Asset, Network, utils
from stellar_sdk import xdr as stellar_xdr


LOBSTR_TOKEN_API_URL = "https://lobstr.co/api/v1/sep/assets/curated.json"

data = requests.get(LOBSTR_TOKEN_API_URL).json()
version = data["version"]
tokens = data["assets"]


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


print(f"// Token list version: {version}, generated from {LOBSTR_TOKEN_API_URL}")
# Sort tokens by asset code
sorted_tokens = sorted(tokens, key=lambda x: x["code"])
for token in sorted_tokens:
    asset = Asset(code=token["code"], issuer=token["issuer"])
    if bytes.fromhex(token["contract"]) != get_asset_contract_id(
        asset, Network.PUBLIC_NETWORK_PASSPHRASE
    ):
        raise ValueError("Contract ID does not match asset")
    print_asset(asset)

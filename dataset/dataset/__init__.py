from stellar_sdk import (
    Account,
    Address,
    Asset,
    Claimant,
    ClaimPredicate,
    FeeBumpTransactionEnvelope,
    HashMemo,
    IdMemo,
    Keypair,
    LiquidityPoolAsset,
    LiquidityPoolId,
    Memo,
    MuxedAccount,
    Network,
    ReturnHashMemo,
    RevokeSponsorship,
    SignedPayloadSigner,
    Signer,
    SignerKey,
    TextMemo,
    TimeBounds,
    TransactionBuilder,
    TransactionEnvelope,
    TrustLineEntryFlag,
    TrustLineFlags,
    scval,
)
from stellar_sdk import xdr as stellar_xdr
from stellar_sdk.operation.revoke_sponsorship import RevokeSponsorshipType
from stellar_sdk.operation.revoke_sponsorship import Signer as RevokeSponsorshipSigner

__all__ = ["SignTxTestCases", "SignSorobanAuthorizationTestCases", "MNEMONIC"]

MNEMONIC = "other base behind follow wet put glad muscle unlock sell income october"
# mnemonic: 'other base behind follow wet put glad muscle unlock sell income october'
# index 0: GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7 / SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK
# index 1: GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX / SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA
# index 2: GCJBZJSKICFGD3FJMN5RBQIIXYUNVWOI7YAHQZQKK4UAWFGW6TRBRVX3 / SCGYXI6ZHWGD5EPFCFVH37EUHA5BIFNJQJGPMXDKHD4DYA3N2MXMA3NI
# index 3: GCPWZDBYNXVQYOKUWAL34ED3GDKD6UITURJP734A4DPB5EPDSXHAM3KX / SB6YADAUJ6ZCG4X7UAKLBJEXGZZJP3S2JUBSD5ZMQNXUGWPNBECNBE6O
# index 4: GATLHLTLWJVCZ52WDZOVTXFE5YXGEQ6SGEAFEL5J52WIYSGPY7PW7BMV / SBWIYHM4LEWSVHIOJXNP66XDUNME373L25EIDEMFWXNZD56PGXEUWSYG
# index 5: GCJ644IGDW7YFNKHTWSCM37FRMFBQ2EDMZLQM4AUCRBFCW562XXC5OW3 / SA3LRNOYCV4NVVYWLX4P3CXQA3ONKBCQRZDSOVENQ2TCNZRFBEO55FXW
# index 6: GCV6BUTD2REAS3MYMXIFPAMPX24FII3HNHLLESPYLOZDNZAJ4ULXP6KU / SDWURSMB36GGY7DBV2D6QMLG5WAZGFQIZNOSCJZDHC7XEMSFNFX2PEWV
# index 7: GALWXOA7RDHCPT7EXBIVCEPQIDNS5RRD6LJORBIA2HU22ORQ6XH267VE / SAKNBXQWB47HK5IN3VI5VM6UVJTCFRIILUQMLTXVA4KNM36RGME6CSJH
# index 8: GBWFAOQTZVL76F27XDJA2YDH2WKYHHMJAOKHF3B2HFHBMNBNBGMCNKQE / SC3N32WGXFSTQQZ6YNCBLUS6QAD6YUBITJROPQ2UWLMC6HOWZ2N3I5F7
# index 9: GD3OAWFV6M5T2DWVWRQONITXSMWJA2DQ3524H7BQYCNVHJZJFWSN65IA / SAJIU6F4S6ML76JWPBE56XWUIFT77VVPMB6IFTKA6L6EIJ2CFWBFYDWO
kp0 = Keypair.from_mnemonic_phrase(MNEMONIC, index=0)
kp1 = Keypair.from_mnemonic_phrase(MNEMONIC, index=1)
kp2 = Keypair.from_mnemonic_phrase(MNEMONIC, index=2)


def common_builder(
    source: Keypair = kp0,
    source_muxed_id: int | None = None,
    sequence: int = 103720918407102567,
    network: str = Network.PUBLIC_NETWORK_PASSPHRASE,
    base_fee: int = 100,
    memo: Memo | None = None,
    time_bounds: TimeBounds | None = TimeBounds(min_time=0, max_time=1670818332),
) -> TransactionBuilder:
    if source_muxed_id is None:
        account = Account(account=source.public_key, sequence=sequence)
    else:
        account = Account(
            MuxedAccount(
                account_id=source.public_key, account_muxed_id=source_muxed_id
            ),
            sequence,
        )

    builder = TransactionBuilder(
        source_account=account,
        network_passphrase=network,
        base_fee=base_fee,
    )

    if memo:
        builder.add_memo(memo)

    if time_bounds:
        builder.add_time_bounds(time_bounds.min_time, time_bounds.max_time)

    return builder


class SignTxTestCases:
    @staticmethod
    def op_create_account() -> TransactionEnvelope:
        return (
            common_builder()
            .append_create_account_op(
                destination=kp1.public_key,
                starting_balance="100",
            )
            .build()
        )

    @staticmethod
    def op_payment_asset_native() -> TransactionEnvelope:
        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="922337203685.4775807",
            )
            .build()
        )

    @staticmethod
    def op_payment_asset_alphanum4() -> TransactionEnvelope:
        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                amount="922337203685.4775807",
            )
            .build()
        )

    @staticmethod
    def op_payment_asset_alphanum12() -> TransactionEnvelope:
        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset(
                    "BANANANANANA",
                    "GCDGPFKW2LUJS2ESKAS42HGOKC6VWOKEJ44TQ3ZXZAMD4ZM5FVHJHPJS",
                ),
                amount="922337203685.4775807",
            )
            .build()
        )

    @staticmethod
    def op_payment_with_muxed_destination() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp1.public_key, account_muxed_id=10000)
        return (
            common_builder()
            .append_payment_op(
                destination=muxed_account,
                asset=Asset.native(),
                amount="922337203685.4775807",
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_with_empty_path() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_swap() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=kp0.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_swap_with_source() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=kp1.public_key,
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_swap_with_muxed_source() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=MuxedAccount(kp1.public_key, 12345),
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=MuxedAccount(kp1.public_key, 12345),
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_swap_with_op_source_not_equals_destination() -> (
        TransactionEnvelope
    ):
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=kp2.public_key,
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_receive_with_muxed_destination() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp1.public_key, account_muxed_id=10000)
        return (
            common_builder()
            .append_path_payment_strict_receive_op(
                destination=muxed_account,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_max="1",
                dest_asset=Asset.native(),
                dest_amount="123456789.334",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_manage_sell_offer_create() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_sell_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="988448423.2134",
                price="0.0001234",
            )
            .build()
        )

    @staticmethod
    def op_manage_sell_offer_update() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_sell_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="988448423.2134",
                price="0.0001234",
                offer_id=7123456,
            )
            .build()
        )

    @staticmethod
    def op_manage_sell_offer_delete() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_sell_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="0",
                price="0.0001234",
                offer_id=7123456,
            )
            .build()
        )

    @staticmethod
    def op_create_passive_sell_offer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_create_passive_sell_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="988448423.2134",
                price="0.0001234",
            )
            .build()
        )

    @staticmethod
    def op_set_options() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                inflation_dest=kp1.public_key,
                clear_flags=TrustLineFlags.AUTHORIZED_FLAG
                | TrustLineFlags.TRUSTLINE_CLAWBACK_ENABLED_FLAG
                | TrustLineFlags.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG,
                set_flags=TrustLineFlags.AUTHORIZED_FLAG,
                master_weight=255,
                low_threshold=10,
                med_threshold=20,
                high_threshold=30,
                home_domain="stellar.org",
                signer=Signer.ed25519_public_key(kp2.public_key, 10),
            )
            .build()
        )

    @staticmethod
    def op_set_options_with_empty_body() -> TransactionEnvelope:
        return common_builder().append_set_options_op().build()

    @staticmethod
    def op_set_options_add_public_key_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.ed25519_public_key(kp1.public_key, 10),
            )
            .build()
        )

    @staticmethod
    def op_set_options_remove_public_key_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.ed25519_public_key(kp1.public_key, 0),
            )
            .build()
        )

    @staticmethod
    def op_set_options_add_hash_x_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.sha256_hash(
                    "XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL", 10
                ),
            )
            .build()
        )

    @staticmethod
    def op_set_options_remove_hash_x_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.sha256_hash(
                    "XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL", 0
                ),
            )
            .build()
        )

    @staticmethod
    def op_set_options_add_pre_auth_tx_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.pre_auth_tx(
                    "TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS", 10
                ),
            )
            .build()
        )

    @staticmethod
    def op_set_options_remove_pre_auth_tx_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer.pre_auth_tx(
                    "TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS", 0
                ),
            )
            .build()
        )

    @staticmethod
    def op_set_options_add_ed25519_signed_payload_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer(
                    signer_key=SignerKey.ed25519_signed_payload(
                        SignedPayloadSigner(
                            kp1.public_key,
                            b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789",
                        )
                    ),
                    weight=10,
                ),
            )
            .build()
        )

    @staticmethod
    def op_set_options_remove_ed25519_signed_payload_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_options_op(
                signer=Signer(
                    signer_key=SignerKey.ed25519_signed_payload(
                        SignedPayloadSigner(
                            kp1.public_key,
                            b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789",
                        )
                    ),
                    weight=0,
                ),
            )
            .build()
        )

    @staticmethod
    def op_change_trust_add_trust_line() -> TransactionEnvelope:
        return (
            common_builder()
            .append_change_trust_op(
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                limit="922337203680.9999999",
            )
            .build()
        )

    @staticmethod
    def op_change_trust_add_trust_line_with_unlimited_limit() -> TransactionEnvelope:
        return (
            common_builder()
            .append_change_trust_op(
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                limit="922337203685.4775807",
            )
            .build()
        )

    @staticmethod
    def op_change_trust_remove_trust_line() -> TransactionEnvelope:
        return (
            common_builder()
            .append_change_trust_op(
                asset=Asset(
                    "USD", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                limit="0",
            )
            .build()
        )

    @staticmethod
    def op_change_trust_with_liquidity_pool_asset_add_trust_line() -> (
        TransactionEnvelope
    ):
        asset1 = Asset(
            "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        )
        asset2 = Asset(
            "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        )
        asset = LiquidityPoolAsset(asset_a=asset1, asset_b=asset2, fee=30)

        return (
            common_builder()
            .append_change_trust_op(
                asset=asset,
                limit="922337203680.9999999",
            )
            .build()
        )

    @staticmethod
    def op_change_trust_with_liquidity_pool_asset_remove_trust_line() -> (
        TransactionEnvelope
    ):
        asset1 = Asset(
            "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        )
        asset2 = Asset(
            "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        )
        asset = LiquidityPoolAsset(asset_a=asset1, asset_b=asset2, fee=30)

        return (
            common_builder()
            .append_change_trust_op(
                asset=asset,
                limit="0",
            )
            .build()
        )

    @staticmethod
    def op_allow_trust_deauthorize() -> TransactionEnvelope:
        return (
            common_builder()
            .append_allow_trust_op(
                trustor=kp1.public_key,
                asset_code="USD",
                authorize=TrustLineEntryFlag.UNAUTHORIZED_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_allow_trust_authorize() -> TransactionEnvelope:
        return (
            common_builder()
            .append_allow_trust_op(
                trustor=kp1.public_key,
                asset_code="USD",
                authorize=TrustLineEntryFlag.AUTHORIZED_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_allow_trust_authorize_to_maintain_liabilities() -> TransactionEnvelope:
        return (
            common_builder()
            .append_allow_trust_op(
                trustor=kp1.public_key,
                asset_code="USD",
                authorize=TrustLineEntryFlag.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_account_merge() -> TransactionEnvelope:
        return (
            common_builder()
            .append_account_merge_op(
                destination=kp1.public_key,
            )
            .build()
        )

    @staticmethod
    def op_account_merge_with_muxed_destination() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp1.public_key, account_muxed_id=10000)
        return (
            common_builder()
            .append_account_merge_op(
                destination=muxed_account,
            )
            .build()
        )

    @staticmethod
    def op_inflation() -> TransactionEnvelope:
        return common_builder().append_inflation_op().build()

    @staticmethod
    def op_manage_data_add() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_data_op(
                data_name="Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
                data_value="Hello Stellar! abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
            )
            .build()
        )

    @staticmethod
    def op_manage_data_add_with_unprintable_data() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_data_op(
                data_name="Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
                data_value="这是一条测试消息 hey".encode("utf-8"),
            )
            .build()
        )

    @staticmethod
    def op_manage_data_remove() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_data_op(
                data_name="Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
                data_value=None,
            )
            .build()
        )

    @staticmethod
    def op_bump_sequence() -> TransactionEnvelope:
        return (
            common_builder()
            .append_bump_sequence_op(
                bump_to=9223372036854775807,
            )
            .build()
        )

    @staticmethod
    def op_manage_buy_offer_create() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_buy_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="988448111.2222",
                price="0.0001011",
            )
            .build()
        )

    @staticmethod
    def op_manage_buy_offer_update() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_buy_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="988448111.2222",
                price="0.0001011",
                offer_id=3523456,
            )
            .build()
        )

    @staticmethod
    def op_manage_buy_offer_delete() -> TransactionEnvelope:
        return (
            common_builder()
            .append_manage_buy_offer_op(
                selling=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                buying=Asset.native(),
                amount="0",
                price="0.0001011",
                offer_id=3523456,
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_with_empty_path() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_swap() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=kp0.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_swap_with_source() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=kp1.public_key,
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_swap_with_muxed_source() -> TransactionEnvelope:
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=MuxedAccount(kp1.public_key, 12345),
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=MuxedAccount(kp1.public_key, 12345),
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_swap_with_op_source_not_equals_destination() -> (
        TransactionEnvelope
    ):
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=kp1.public_key,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
                source=kp2.public_key,
            )
            .build()
        )

    @staticmethod
    def op_path_payment_strict_send_with_muxed_destination() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp1.public_key, account_muxed_id=10000)
        return (
            common_builder()
            .append_path_payment_strict_send_op(
                destination=muxed_account,
                send_asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                send_amount="0.985",
                dest_asset=Asset.native(),
                dest_min="123456789.987",
                path=[
                    Asset(
                        "USDC",
                        "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN",
                    ),
                    Asset(
                        "PANDA",
                        "GDJVFDG5OCW5PYWHB64MGTHGFF57DRRJEDUEFDEL2SLNIOONHYJWHA3Z",
                    ),
                ],
            )
            .build()
        )

    @staticmethod
    def op_create_claimable_balance() -> TransactionEnvelope:
        claimants = [
            Claimant(
                destination=kp1.public_key,
                predicate=ClaimPredicate.predicate_unconditional(),
            ),
            Claimant(
                destination=kp2.public_key,
                predicate=ClaimPredicate.predicate_and(
                    ClaimPredicate.predicate_or(
                        ClaimPredicate.predicate_before_absolute_time(1629344902),
                        ClaimPredicate.predicate_before_absolute_time(1629300000),
                    ),
                    ClaimPredicate.predicate_not(
                        ClaimPredicate.predicate_before_relative_time(180)
                    ),
                ),
            ),
        ]

        return (
            common_builder()
            .append_create_claimable_balance_op(
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                amount="100",
                claimants=claimants,
            )
            .build()
        )

    @staticmethod
    def op_claim_claimable_balance() -> TransactionEnvelope:
        return (
            common_builder()
            .append_claim_claimable_balance_op(
                balance_id="00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
            )
            .build()
        )

    @staticmethod
    def op_begin_sponsoring_future_reserves() -> TransactionEnvelope:
        return (
            common_builder()
            .append_begin_sponsoring_future_reserves_op(
                sponsored_id=kp1.public_key,
            )
            .build()
        )

    @staticmethod
    def op_end_sponsoring_future_reserves() -> TransactionEnvelope:
        return common_builder().append_end_sponsoring_future_reserves_op().build()

    @staticmethod
    def op_revoke_sponsorship_account() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_account_sponsorship_op(
                account_id=kp1.public_key,
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_trust_line_with_asset() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_trustline_sponsorship_op(
                account_id=kp1.public_key,
                asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_trust_line_with_liquidity_pool_id() -> (
        TransactionEnvelope
    ):
        asset1 = Asset(
            "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        )
        asset2 = Asset(
            "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        )

        liquidity_pool_asset = LiquidityPoolAsset(
            asset_a=asset1, asset_b=asset2, fee=30
        )
        pool_id = liquidity_pool_asset.liquidity_pool_id

        return (
            common_builder()
            .append_revoke_trustline_sponsorship_op(
                account_id=kp1.public_key,
                asset=LiquidityPoolId(pool_id),
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_offer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_offer_sponsorship_op(
                seller_id=kp1.public_key,
                offer_id=123456,
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_data() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_data_sponsorship_op(
                account_id=kp1.public_key,
                data_name="Ledger Stellar App abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcda",
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_claimable_balance() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_claimable_balance_sponsorship_op(
                claimable_balance_id="00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_liquidity_pool() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_liquidity_pool_sponsorship_op(
                liquidity_pool_id="dd7b1ab831c273310ddbec6f97870aa83c2fbd78ce22aded37ecbf4f3380fac7",
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_ed25519_public_key_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_ed25519_public_key_signer_sponsorship_op(
                account_id=kp1.public_key,
                signer_key=kp2.public_key,
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_hash_x_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_hashx_signer_sponsorship_op(
                account_id=kp1.public_key,
                signer_key="XDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH235FXL",
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_pre_auth_tx_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_revoke_pre_auth_tx_signer_sponsorship_op(
                account_id=kp1.public_key,
                signer_key="TDNA2V62PVEFBZ74CDJKTUHLY4Y7PL5UAV2MAM4VWF6USFE3SH234BSS",
            )
            .build()
        )

    @staticmethod
    def op_revoke_sponsorship_ed25519_signed_payload_signer() -> TransactionEnvelope:
        return (
            common_builder()
            .append_operation(
                RevokeSponsorship(
                    RevokeSponsorshipType.SIGNER,
                    account_id=None,
                    trustline=None,
                    offer=None,
                    data=None,
                    claimable_balance_id=None,
                    liquidity_pool_id=None,
                    signer=RevokeSponsorshipSigner(
                        kp1.public_key,
                        SignerKey.ed25519_signed_payload(
                            SignedPayloadSigner(
                                kp1.public_key,
                                b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ000123456789",
                            )
                        ),
                    ),
                )
            )
            .build()
        )

    @staticmethod
    def op_clawback() -> TransactionEnvelope:
        return (
            common_builder()
            .append_clawback_op(
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                from_=kp1.public_key,
                amount="1000.85",
            )
            .build()
        )

    @staticmethod
    def op_clawback_with_muxed_from() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp1.public_key, account_muxed_id=10000)
        return (
            common_builder()
            .append_clawback_op(
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                from_=muxed_account,
                amount="1000.85",
            )
            .build()
        )

    @staticmethod
    def op_clawback_claimable_balance() -> TransactionEnvelope:
        return (
            common_builder()
            .append_clawback_claimable_balance_op(
                balance_id="00000000da0d57da7d4850e7fc10d2a9d0ebc731f7afb40574c03395b17d49149b91f5be",
            )
            .build()
        )

    @staticmethod
    def op_set_trust_line_flags_unauthorized() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_trust_line_flags_op(
                trustor=kp1.public_key,
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                clear_flags=TrustLineFlags.AUTHORIZED_FLAG
                | TrustLineFlags.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG
                | TrustLineFlags.TRUSTLINE_CLAWBACK_ENABLED_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_set_trust_line_flags_authorized() -> TransactionEnvelope:
        return (
            common_builder()
            .append_set_trust_line_flags_op(
                trustor=kp1.public_key,
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                clear_flags=TrustLineFlags.TRUSTLINE_CLAWBACK_ENABLED_FLAG,
                set_flags=TrustLineFlags.AUTHORIZED_FLAG
                | TrustLineFlags.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_set_trust_line_flags_authorized_to_maintain_liabilities() -> (
        TransactionEnvelope
    ):
        return (
            common_builder()
            .append_set_trust_line_flags_op(
                trustor=kp1.public_key,
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                clear_flags=TrustLineFlags.AUTHORIZED_FLAG
                | TrustLineFlags.TRUSTLINE_CLAWBACK_ENABLED_FLAG,
                set_flags=TrustLineFlags.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_set_trust_line_flags_authorized_and_clawback_enabled() -> (
        TransactionEnvelope
    ):
        return (
            common_builder()
            .append_set_trust_line_flags_op(
                trustor=kp1.public_key,
                asset=Asset(
                    "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
                ),
                clear_flags=TrustLineFlags.AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG,
                set_flags=TrustLineFlags.AUTHORIZED_FLAG
                | TrustLineFlags.TRUSTLINE_CLAWBACK_ENABLED_FLAG,
            )
            .build()
        )

    @staticmethod
    def op_liquidity_pool_deposit() -> TransactionEnvelope:
        asset1 = Asset(
            "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        )
        asset2 = Asset(
            "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        )

        liquidity_pool_asset = LiquidityPoolAsset(
            asset_a=asset1, asset_b=asset2, fee=30
        )
        pool_id = liquidity_pool_asset.liquidity_pool_id

        return (
            common_builder()
            .append_liquidity_pool_deposit_op(
                liquidity_pool_id=pool_id,
                max_amount_a="1000000",
                max_amount_b="0.2321",
                min_price="14324232.23",
                max_price="10000000.00",
            )
            .build()
        )

    @staticmethod
    def op_liquidity_pool_withdraw() -> TransactionEnvelope:
        asset1 = Asset(
            "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
        )
        asset2 = Asset(
            "USDC", "GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN"
        )

        liquidity_pool_asset = LiquidityPoolAsset(
            asset_a=asset1, asset_b=asset2, fee=30
        )
        pool_id = liquidity_pool_asset.liquidity_pool_id

        return (
            common_builder()
            .append_liquidity_pool_withdraw_op(
                liquidity_pool_id=pool_id,
                amount="5000",
                min_amount_a="10000",
                min_amount_b="20000",
            )
            .build()
        )

    @staticmethod
    def op_invoke_host_function_upload_wasm() -> TransactionEnvelope:
        """
        soroban --very-verbose contract deploy \
         --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
         --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
         --rpc-url https://soroban-testnet.stellar.org \
         --network-passphrase 'Test SDF Network ; September 2015'
        """
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAcdcALZ/tAAAAAQAAAAAAAAAAAAAAAQAAAAEAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAAYAAAAAgAAAkwAYXNtAQAAAAEVBGACfn4BfmADfn5+AX5gAAF+YAAAAhkEAWwBMAAAAWwBMQAAAWwBXwABAWwBOAAAAwUEAgMDAwUDAQAQBhkDfwFBgIDAAAt/AEGAgMAAC38AQYCAwAALBzUFBm1lbW9yeQIACWluY3JlbWVudAAEAV8ABwpfX2RhdGFfZW5kAwELX19oZWFwX2Jhc2UDAgqnAQSSAQIBfwF+QQAhAAJAAkACQEKOutCvhtQ5QgIQgICAgABCAVINAEKOutCvhtQ5QgIQgYCAgAAiAUL/AYNCBFINASABQiCIpyEACyAAQQFqIgBFDQFCjrrQr4bUOSAArUIghkIEhCIBQgIQgoCAgAAaQoSAgICgBkKEgICAwAwQg4CAgAAaIAEPCwAACxCFgICAAAALCQAQhoCAgAAACwQAAAALAgALAHMOY29udHJhY3RzcGVjdjAAAAAAAAAAQEluY3JlbWVudCBpbmNyZW1lbnRzIGFuIGludGVybmFsIGNvdW50ZXIsIGFuZCByZXR1cm5zIHRoZSB2YWx1ZS4AAAAJaW5jcmVtZW50AAAAAAAAAAAAAAEAAAAEAB4RY29udHJhY3RlbnZtZXRhdjAAAAAAAAAAFAAAADkAcw5jb250cmFjdG1ldGF2MAAAAAAAAAAFcnN2ZXIAAAAAAAAGMS43My4wAAAAAAAAAAAACHJzc2RrdmVyAAAAMzIwLjAuMC1yYzIjMDk5MjQxM2Y5YjA1ZTViZmIxZjg3MmJjZTk5ZTg5ZDkxMjliMmU2MQAAAAAAAAAAAQAAAAAAAAABAAAABxPhaFi95KtQoAbb8HFyKI8+wZ2GQNGoUwFsYMFcJREXAAAAAAAYZjYAAAKwAAAAAAAAAAAAAAAMAAAAAU5Gbs0AAABA4Qgx6lFhpvkLoEOoHEV2O/B0+ALtMwuX4Kh3iPmI4CtYXBFNMUmKDnKvsiZE/moqNtxyD8Ce0ZblL6rhjCCaBA=="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_create_contract_wasm_id() -> TransactionEnvelope:
        """
        soroban --very-verbose contract deploy \
         --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
         --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
         --rpc-url https://soroban-testnet.stellar.org \
         --network-passphrase 'Test SDF Network ; September 2015'
        """
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAB7iQALZ/tAAAAAgAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAQAAAAAAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NqDN1ZmOZBULw7RQpLx1hMwklzeYyod2tz7XGLOGlAnsAAAAAE+FoWL3kq1CgBtvwcXIojz7BnYZA0ahTAWxgwVwlERcAAAABAAAAAAAAAAEAAAAAAAAAAAAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzagzdWZjmQVC8O0UKS8dYTMJJc3mMqHdrc+1xizhpQJ7AAAAABPhaFi95KtQoAbb8HFyKI8+wZ2GQNGoUwFsYMFcJREXAAAAAAAAAAEAAAAAAAAAAQAAAAcT4WhYveSrUKAG2/BxciiPPsGdhkDRqFMBbGDBXCURFwAAAAEAAAAGAAAAASDg5o1dgbNGaFKMfUZ6NtiiHIsKeZQjTddBtARypKjgAAAAFAAAAAEAAbYGAAAC4AAAAGgAAAAAAACh8wAAAAFORm7NAAAAQB9/IVcMlt2Uo2f5SSwDUXEimmUOQMqgz2baGQlL6a4aH/Jyqpm5sGsraNzDlbu6W5VIcHkEtuGVY+d7kPNFHwU="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_create_contract_v2_wasm_id() -> TransactionEnvelope:
        raw = "AAAAAgAAAADBPp7TMinJylnn+6dQXJACNc15LF+aJ2Py1BaR4P10JAAD8S0AASA+AAAACgAAAAEAAAAAAAAAAAAAAABoup5qAAAAAAAAAAEAAAAAAAAAGAAAAAMAAAAAAAAAAAAAAADBPp7TMinJylnn+6dQXJACNc15LF+aJ2Py1BaR4P10JAOxFva83LybI/D3Gt4Fc51tE7iov2QvYmUjtOnhBEMEAAAAAJauxXika7rfqGNdERxf2IpO+orjJK7EostLJJygIWnPAAAAAwAAABIAAAAAAAAAAME+ntMyKcnKWef7p1BckAI1zXksX5onY/LUFpHg/XQkAAAAEgAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAAoAAAAAAAAAAAAAAAAAAYagAAAAAQAAAAAAAAACAAAAAAAAAAAAAAAAwT6e0zIpycpZ5/unUFyQAjXNeSxfmidj8tQWkeD9dCQDsRb2vNy8myPw9xreBXOdbRO4qL9kL2JlI7Tp4QRDBAAAAACWrsV4pGu636hjXREcX9iKTvqK4ySuxKLLSyScoCFpzwAAAAMAAAASAAAAAAAAAADBPp7TMinJylnn+6dQXJACNc15LF+aJ2Py1BaR4P10JAAAABIAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAKAAAAAAAAAAAAAAAAAAGGoAAAAAEAAAAAAAAAAToXCSpKl5MnTUbZZRSy4m//Ui0ICra4hpizTUSJtHRcAAAADV9fY29uc3RydWN0b3IAAAAAAAADAAAAEgAAAAAAAAAAwT6e0zIpycpZ5/unUFyQAjXNeSxfmidj8tQWkeD9dCQAAAASAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAACgAAAAAAAAAAAAAAAAABhqAAAAABAAAAAAAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAAAh0cmFuc2ZlcgAAAAMAAAASAAAAAAAAAADBPp7TMinJylnn+6dQXJACNc15LF+aJ2Py1BaR4P10JAAAABIAAAABOhcJKkqXkydNRtllFLLib/9SLQgKtriGmLNNRIm0dFwAAAAKAAAAAAAAAAAAAAAAAAGGoAAAAAAAAAABAAAAAAAAAAIAAAAGAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAAFAAAAAEAAAAHlq7FeKRrut+oY10RHF/Yik76iuMkrsSiy0sknKAhac8AAAADAAAAAAAAAADBPp7TMinJylnn+6dQXJACNc15LF+aJ2Py1BaR4P10JAAAAAYAAAABOhcJKkqXkydNRtllFLLib/9SLQgKtriGmLNNRIm0dFwAAAAUAAAAAQAAAAYAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAQAAAAAQAAAAIAAAAPAAAAB0JhbGFuY2UAAAAAEgAAAAE6FwkqSpeTJ01G2WUUsuJv/1ItCAq2uIaYs01EibR0XAAAAAEADkMVAAAAkAAAAhgAAAAAAAPwyQAAAAA="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_create_contract_new_asset() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import (
        #     Keypair,
        #     Network,
        #     SorobanServer,
        #     StrKey,
        #     TransactionBuilder,
        # )
        # from stellar_sdk import xdr as stellar_xdr
        # from stellar_sdk.exceptions import PrepareTransactionException
        # from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus
        #
        # secret = "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
        # rpc_server_url = "https://soroban-testnet.stellar.org:443"
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # kp = Keypair.from_secret(secret)
        # soroban_server = SorobanServer(rpc_server_url)
        # source = soroban_server.load_account(kp.public_key)
        #
        # tx = (
        #     TransactionBuilder(source, network_passphrase)
        #     .add_time_bounds(0, 0)
        #     .append_create_stellar_asset_contract_from_address_op(address=kp.public_key, salt=b'c' * 32, source=kp.public_key)
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(tx.to_xdr())
        #
        # tx.sign(kp)
        #
        # send_transaction_data = soroban_server.send_transaction(tx)
        # print(f"sent transaction: {send_transaction_data}")
        # if send_transaction_data.status != SendTransactionStatus.PENDING:
        #     raise Exception("send transaction failed")
        #
        # while True:
        #     print("waiting for transaction to be confirmed...")
        #     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
        #     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
        #         break
        #     time.sleep(3)
        #
        # print(f"transaction: {get_transaction_data}")
        #
        # if get_transaction_data.status == GetTransactionStatus.SUCCESS:
        #     assert get_transaction_data.result_meta_xdr is not None
        #     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
        #         get_transaction_data.result_meta_xdr
        #     )
        #     result = transaction_meta.v3.soroban_meta.return_value.address.contract_id.hash  # type: ignore
        #     contract_id = StrKey.encode_contract(result)
        #     print(f"contract id: {contract_id}")
        # else:
        #     print(f"Transaction failed: {get_transaction_data.result_xdr}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABUbYALZ/tAAAACgAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAEAAAAAAAAAAAAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzWNjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjAAAAAQAAAAEAAAAAAAAAAQAAAAAAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2MAAAABAAAAAAAAAAEAAAAAAAAAAAAAAAEAAAAGAAAAAVvz5/0ZDBy7NBxhMS/CR+rdii7rHMTLZO5cDByDoxtUAAAAFAAAAAEAAXf0AAAAMAAAAEgAAAAAAACSxAAAAAA="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_create_contract_wrap_asset() -> TransactionEnvelope:
        """
        soroban contract deploy \
         --wasm ./increment/target/wasm32-unknown-unknown/release/soroban_increment_contract.wasm  \
         --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
         --rpc-url https://soroban-testnet.stellar.org \
         --network-passphrase 'Test SDF Network ; September 2015'
        """
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAGY4YALZ/tAAAAAwAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAQAAAAEAAAACTEVER0VSAAAAAAAAAAAAAOLGgQ+bUJsmS/JcX/hJdFvPzHVliRnd2jaspzLmaehVAAAAAQAAAAAAAAABAAAAAAAAAAAAAAABAAAABgAAAAH1fw7+2c6SK9x+aL47FkMYOgIZwujCGmG2Ve5S7lUfFgAAABQAAAABAAOluwAAADAAAAHsAAAAAAABWgMAAAABTkZuzQAAAEAScsqMRjAFnQsVoZjDSfYjEGAOPXjgZaFvxaN1EuE0q/zuF+uukosgx7UaxNR7R1xvz0Rk27VtC+E0X/SNcWIC"
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_without_args() -> TransactionEnvelope:
        """
        soroban --very-verbose contract invoke \
         --id CAQOBZUNLWA3GRTIKKGH2RT2G3MKEHELBJ4ZII2N25A3IBDSUSUOAUQU  \
         --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
         --rpc-url https://soroban-testnet.stellar.org \
         --network-passphrase 'Test SDF Network ; September 2015' \
         -- \
         increment
        """
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABxvgALZ/tAAAABAAAAAAAAAAAAAAAAQAAAAAAAAAYAAAAAAAAAAEg4OaNXYGzRmhSjH1GejbYohyLCnmUI03XQbQEcqSo4AAAAAlpbmNyZW1lbnQAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAQAAAAcT4WhYveSrUKAG2/BxciiPPsGdhkDRqFMBbGDBXCURFwAAAAEAAAAGAAAAASDg5o1dgbNGaFKMfUZ6NtiiHIsKeZQjTddBtARypKjgAAAAFAAAAAEAGQovAAADSAAAAIQAAAAAAAANSQAAAAFORm7NAAAAQMeYqOX1HnwH9heyEgce5OcjQEakm+vFFqtXBEdaHMqDvMBVCcy4u8WhVAbOWCvNQf+/wjIaj03un47sRyLJtwc="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_with_complex_sub_invocation() -> TransactionEnvelope:
        scvals = [
            scval.to_uint128(1),
            scval.to_int128(2),
            scval.to_uint256(3),
        ]
        tx = (
            common_builder(base_fee=500)
            .append_invoke_contract_function_op(
                contract_id="CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND",
                function_name="rootfunc",
                parameters=scvals,
                auth=[
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_SOURCE_ACCOUNT
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "rootfunc".encode()
                                    ),
                                    args=scvals,
                                ),
                            ),
                            sub_invocations=[
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "func0".encode()
                                            ),
                                            args=[
                                                scval.to_int128(103560 * 10**5),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func1".encode()
                                                    ),
                                                    args=[],
                                                ),
                                            ),
                                            sub_invocations=[],
                                        ),
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func2".encode()
                                                    ),
                                                    args=[],
                                                ),
                                            ),
                                            sub_invocations=[
                                                stellar_xdr.SorobanAuthorizedInvocation(
                                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN,
                                                        create_contract_host_fn=stellar_xdr.CreateContractArgs(
                                                            contract_id_preimage=stellar_xdr.ContractIDPreimage(
                                                                stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ADDRESS,
                                                                from_address=stellar_xdr.ContractIDPreimageFromAddress(
                                                                    address=Address(
                                                                        "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                                    ).to_xdr_sc_address(),
                                                                    salt=stellar_xdr.Uint256(
                                                                        b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                                                                    ),
                                                                ),
                                                            ),
                                                            executable=stellar_xdr.ContractExecutable(
                                                                stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
                                                                stellar_xdr.Hash(
                                                                    b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                                                                ),
                                                            ),
                                                        ),
                                                    ),
                                                    sub_invocations=[
                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_V2_HOST_FN,
                                                                create_contract_v2_host_fn=stellar_xdr.CreateContractArgsV2(
                                                                    contract_id_preimage=stellar_xdr.ContractIDPreimage(
                                                                        stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ADDRESS,
                                                                        from_address=stellar_xdr.ContractIDPreimageFromAddress(
                                                                            address=Address(
                                                                                "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                                            ).to_xdr_sc_address(),
                                                                            salt=stellar_xdr.Uint256(
                                                                                b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                                                                            ),
                                                                        ),
                                                                    ),
                                                                    executable=stellar_xdr.ContractExecutable(
                                                                        stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
                                                                        stellar_xdr.Hash(
                                                                            b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                                                                        ),
                                                                    ),
                                                                    constructor_args=[
                                                                        scval.to_address(
                                                                            "GDAT5HWTGIU4TSSZ4752OUC4SABDLTLZFRPZUJ3D6LKBNEPA7V2CIG54"
                                                                        ),
                                                                        scval.to_int128(
                                                                            12000000
                                                                        ),
                                                                    ],
                                                                ),
                                                            ),
                                                            sub_invocations=[
                                                                stellar_xdr.SorobanAuthorizedInvocation(
                                                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                                                            Address(
                                                                                "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                                            ).to_xdr_sc_address(),
                                                                            stellar_xdr.SCSymbol(
                                                                                "__constructor".encode()
                                                                            ),
                                                                            [
                                                                                scval.to_address(
                                                                                    "GDAT5HWTGIU4TSSZ4752OUC4SABDLTLZFRPZUJ3D6LKBNEPA7V2CIG54"
                                                                                ),
                                                                                scval.to_int128(
                                                                                    12000000
                                                                                ),
                                                                            ],
                                                                        ),
                                                                    ),
                                                                    sub_invocations=[],
                                                                )
                                                            ],
                                                        ),
                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                    contract_address=Address(
                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                    ).to_xdr_sc_address(),
                                                                    function_name=stellar_xdr.SCSymbol(
                                                                        "func4".encode()
                                                                    ),
                                                                    args=[],
                                                                ),
                                                            ),
                                                            sub_invocations=[],
                                                        ),
                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                    contract_address=Address(
                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                    ).to_xdr_sc_address(),
                                                                    function_name=stellar_xdr.SCSymbol(
                                                                        "func5".encode()
                                                                    ),
                                                                    args=[
                                                                        scval.to_void()
                                                                    ],
                                                                ),
                                                            ),
                                                            sub_invocations=[
                                                                stellar_xdr.SorobanAuthorizedInvocation(
                                                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                                                            contract_address=Address(
                                                                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                            ).to_xdr_sc_address(),
                                                                            function_name=stellar_xdr.SCSymbol(
                                                                                "func6".encode()
                                                                            ),
                                                                            args=[
                                                                                scval.to_void()
                                                                            ],
                                                                        ),
                                                                    ),
                                                                    sub_invocations=[
                                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                                    contract_address=Address(
                                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                                    ).to_xdr_sc_address(),
                                                                                    function_name=stellar_xdr.SCSymbol(
                                                                                        "func7".encode()
                                                                                    ),
                                                                                    args=[
                                                                                        scval.to_void()
                                                                                    ],
                                                                                ),
                                                                            ),
                                                                            sub_invocations=[],
                                                                        ),
                                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                                    contract_address=Address(
                                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                                    ).to_xdr_sc_address(),
                                                                                    function_name=stellar_xdr.SCSymbol(
                                                                                        "func8".encode()
                                                                                    ),
                                                                                    args=[
                                                                                        scval.to_void()
                                                                                    ],
                                                                                ),
                                                                            ),
                                                                            sub_invocations=[],
                                                                        ),
                                                                    ],
                                                                )
                                                            ],
                                                        ),
                                                    ],
                                                ),
                                            ],
                                        ),
                                    ],
                                )
                            ],
                        ),
                    ),
                ],
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_asset_transfer() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
        # from stellar_sdk import xdr as stellar_xdr
        # from stellar_sdk.exceptions import PrepareTransactionException
        # from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus
        #
        # rpc_server_url = "https://soroban-testnet.stellar.org:443"
        # soroban_server = SorobanServer(rpc_server_url)
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # alice_kp = Keypair.from_secret(
        #     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
        # )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
        # bob_kp = Keypair.from_secret(
        #     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
        # )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
        # native_token_contract_id = "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
        #
        # alice_source = soroban_server.load_account(alice_kp.public_key)
        #
        # args = [
        #     scval.to_address(alice_kp.public_key),  # from
        #     scval.to_address(bob_kp.public_key),  # to
        #     scval.to_int128(100 * 10 ** 7),  # amount, 100 XLM
        # ]
        #
        # tx = (
        #     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
        #     .add_time_bounds(0, 0)
        #     .append_invoke_contract_function_op(
        #         contract_id=native_token_contract_id,
        #         function_name="transfer",
        #         parameters=args,
        #         source=alice_kp.public_key
        #     )
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(f"Unsigned XDR:\n{tx.to_xdr()}")
        #
        # tx.sign(alice_kp)
        # print(f"Signed XDR:\n{tx.to_xdr()}")
        #
        # send_transaction_data = soroban_server.send_transaction(tx)
        # print(f"sent transaction: {send_transaction_data}")
        # if send_transaction_data.status != SendTransactionStatus.PENDING:
        #     raise Exception("send transaction failed")
        # while True:
        #     print("waiting for transaction to be confirmed...")
        #     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
        #     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
        #         break
        #     time.sleep(3)
        #
        # print(f"transaction: {get_transaction_data}")
        #
        # if get_transaction_data.status == GetTransactionStatus.SUCCESS:
        #     assert get_transaction_data.result_meta_xdr is not None
        #     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
        #         get_transaction_data.result_meta_xdr
        #     )
        #     if transaction_meta.v3.soroban_meta.return_value.type == stellar_xdr.SCValType.SCV_VOID:  # type: ignore[union-attr]
        #         print("send success")
        # else:
        #     print(f"Transaction failed: {get_transaction_data.result_xdr}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQACOxwALZ/tAAAACwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAIdHJhbnNmZXIAAAADAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAA7msoAAAAAAQAAAAAAAAAAAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAACHRyYW5zZmVyAAAAAwAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAEgAAAAAAAAAA4saBD5tQmyZL8lxf+El0W8/MdWWJGd3aNqynMuZp6FUAAAAKAAAAAAAAAAAAAAAAO5rKAAAAAAAAAAABAAAAAAAAAAEAAAAGAAAAAdeSi3LCcDzP6vfrn/TvTVBKVai5efybRQ6iyEK00c5hAAAAFAAAAAEAAAACAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0ABBGDAAACFAAAAOwAAAAAAAAAOwAAAAA="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_asset_approve() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
        # from stellar_sdk import xdr as stellar_xdr
        # from stellar_sdk.exceptions import PrepareTransactionException
        # from stellar_sdk.soroban_rpc import GetTransactionStatus, SendTransactionStatus
        #
        # rpc_server_url = "https://soroban-testnet.stellar.org:443"
        # soroban_server = SorobanServer(rpc_server_url)
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # alice_kp = Keypair.from_secret(
        #     "SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK"
        # )  # GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7
        # bob_kp = Keypair.from_secret(
        #     "SAE52G23WPAS7MIR2OFGILLICLXXR4K6HSXZHMKD6C33JCAVVILIWYAA"
        # )  # GDRMNAIPTNIJWJSL6JOF76CJORN47TDVMWERTXO2G2WKOMXGNHUFL5QX
        # native_token_contract_id = "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
        #
        # alice_source = soroban_server.load_account(alice_kp.public_key)
        #
        # args = [
        #     scval.to_address(alice_kp.public_key),  # from
        #     scval.to_address(bob_kp.public_key),  # spender
        #     scval.to_int128(1000 * 10 ** 7),  # amount, 1000 XLM
        #     scval.to_uint32(2999592)
        # ]
        #
        # tx = (
        #     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
        #     .add_time_bounds(0, 0)
        #     .append_invoke_contract_function_op(
        #         contract_id=native_token_contract_id,
        #         function_name="approve",
        #         parameters=args,
        #         source=alice_kp.public_key
        #     )
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(f"Unsigned XDR:\n{tx.to_xdr()}")
        #
        # tx.sign(alice_kp)
        # print(f"Signed XDR:\n{tx.to_xdr()}")
        #
        # send_transaction_data = soroban_server.send_transaction(tx)
        # print(f"sent transaction: {send_transaction_data}")
        # if send_transaction_data.status != SendTransactionStatus.PENDING:
        #     raise Exception("send transaction failed")
        # while True:
        #     print("waiting for transaction to be confirmed...")
        #     get_transaction_data = soroban_server.get_transaction(send_transaction_data.hash)
        #     if get_transaction_data.status != GetTransactionStatus.NOT_FOUND:
        #         break
        #     time.sleep(3)
        #
        # print(f"transaction: {get_transaction_data}")
        #
        # if get_transaction_data.status == GetTransactionStatus.SUCCESS:
        #     assert get_transaction_data.result_meta_xdr is not None
        #     transaction_meta = stellar_xdr.TransactionMeta.from_xdr(
        #         get_transaction_data.result_meta_xdr
        #     )
        #     if transaction_meta.v3.soroban_meta.return_value.type == stellar_xdr.SCValType.SCV_VOID:  # type: ignore[union-attr]
        #         print("send success")
        # else:
        #     print(f"Transaction failed: {get_transaction_data.result_xdr}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQADJ94ALZ/tAAAADAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAJUC+QAAAAAAwAtxSgAAAABAAAAAAAAAAAAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADixoEPm1CbJkvyXF/4SXRbz8x1ZYkZ3do2rKcy5mnoVQAAAAoAAAAAAAAAAAAAAAJUC+QAAAAAAwAtxSgAAAAAAAAAAQAAAAAAAAABAAAABgAAAAHXkotywnA8z+r365/0701QSlWouXn8m0UOoshCtNHOYQAAABQAAAABAAAAAQAAAAYAAAAB15KLcsJwPM/q9+uf9O9NUEpVqLl5/JtFDqLIQrTRzmEAAAAQAAAAAQAAAAIAAAAPAAAACUFsbG93YW5jZQAAAAAAABEAAAABAAAAAgAAAA8AAAAEZnJvbQAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAADwAAAAdzcGVuZGVyAAAAABIAAAAAAAAAAOLGgQ+bUJsmS/JcX/hJdFvPzHVliRnd2jaspzLmaehVAAAAAAAEyB8AAAFYAAABLAAAAAAAAHGtAAAAAA=="
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_transfer_xlm() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
        # from stellar_sdk.exceptions import PrepareTransactionException
        #
        # rpc_server_url = "https://patient-green-dinghy.stellar-mainnet.quiknode.pro/d92497257a021cd5ea700dae4b20496945962a4b/"
        # soroban_server = SorobanServer(rpc_server_url)
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # alice_kp = Keypair.from_public_key("GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7")
        # bob_kp = Keypair.from_public_key("GDMTVHLWJTHSUDMZVVMXXH6VJHA2ZV3HNG5LYNAZ6RTWB7GISM6PGTUV")
        # native_token_contract_id = "CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA"
        #
        # alice_source = soroban_server.load_account(alice_kp.public_key)
        #
        # args = [
        #     scval.to_address(alice_kp.public_key),  # from
        #     scval.to_address(bob_kp.public_key),  # to
        #     scval.to_int128(5 * 10**7),  # amount, 5 XLM
        # ]
        #
        # tx = (
        #     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
        #     .add_time_bounds(0, 0)
        #     .append_invoke_contract_function_op(
        #         contract_id=native_token_contract_id,
        #         function_name="transfer",
        #         parameters=args,
        #     )
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(f"XDR:\n{tx.to_xdr()}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABLoYDEd5JAAAAAwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAGAAAAAAAAAABJbT82FmuwvpjSEOMSJs8PBDJi20hvk/TyzDLaJU++XcAAAAIdHJhbnNmZXIAAAADAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAoAAAAAAAAAAAAAAAAC+vCAAAAAAQAAAAAAAAAAAAAAASW0/NhZrsL6Y0hDjEibPDwQyYttIb5P08swy2iVPvl3AAAACHRyYW5zZmVyAAAAAwAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAEgAAAAAAAAAA2TqddkzPKg2ZrVl7n9VJwazXZ2m6vDQZ9Gdg/MiTPPMAAAAKAAAAAAAAAAAAAAAAAvrwgAAAAAAAAAABAAAAAAAAAAEAAAAGAAAAASW0/NhZrsL6Y0hDjEibPDwQyYttIb5P08swy2iVPvl3AAAAFAAAAAEAAAACAAAAAAAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AL6XaAAACGAAAASAAAAAAAAEskgAAAAA="
        return TransactionEnvelope.from_xdr(raw, Network.PUBLIC_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_transfer_usdc() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
        # from stellar_sdk.exceptions import PrepareTransactionException
        #
        # rpc_server_url = "https://patient-green-dinghy.stellar-mainnet.quiknode.pro/d92497257a021cd5ea700dae4b20496945962a4b/"
        # soroban_server = SorobanServer(rpc_server_url)
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # alice_kp = Keypair.from_public_key("GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7")
        # bob_kp = Keypair.from_public_key("GDMTVHLWJTHSUDMZVVMXXH6VJHA2ZV3HNG5LYNAZ6RTWB7GISM6PGTUV")
        # native_token_contract_id = "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
        #
        # alice_source = soroban_server.load_account(alice_kp.public_key)
        #
        # args = [
        #     scval.to_address(alice_kp.public_key),  # from
        #     scval.to_address(bob_kp.public_key),  # to
        #     scval.to_int128(123 * 10**4),  # amount, 0.123
        # ]
        #
        # tx = (
        #     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
        #     .add_time_bounds(0, 0)
        #     .append_invoke_contract_function_op(
        #         contract_id=native_token_contract_id,
        #         function_name="transfer",
        #         parameters=args,
        #     )
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(f"XDR:\n{tx.to_xdr()}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABNCsDEd5JAAAAAwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAGAAAAAAAAAABre/OWa7lKWj3YGHUlMJSW3Vln6QpamX0me8p5WR35JYAAAAIdHJhbnNmZXIAAAADAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAoAAAAAAAAAAAAAAAAAEsSwAAAAAQAAAAAAAAAAAAAAAa3vzlmu5Slo92Bh1JTCUlt1ZZ+kKWpl9JnvKeVkd+SWAAAACHRyYW5zZmVyAAAAAwAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAEgAAAAAAAAAA2TqddkzPKg2ZrVl7n9VJwazXZ2m6vDQZ9Gdg/MiTPPMAAAAKAAAAAAAAAAAAAAAAABLEsAAAAAAAAAABAAAAAAAAAAEAAAAGAAAAAa3vzlmu5Slo92Bh1JTCUlt1ZZ+kKWpl9JnvKeVkd+SWAAAAFAAAAAEAAAACAAAAAQAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAFVU0RDAAAAADuZETgO/piLoKiQDrHP5E82b32+lGvtB3JA9/Yk3xXFAAAAAQAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAAAFVU0RDAAAAADuZETgO/piLoKiQDrHP5E82b32+lGvtB3JA9/Yk3xXFAC/zlgAAAsQAAADoAAAAAAABMjcAAAAA"
        return TransactionEnvelope.from_xdr(raw, Network.PUBLIC_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_transfer_from_usdc() -> TransactionEnvelope:
        return (
            common_builder()
            .append_invoke_contract_function_op(
                contract_id="CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75",
                function_name="transfer_from",
                parameters=[
                    scval.to_address(kp0.public_key),  # spender
                    scval.to_address(kp1.public_key),  # from
                    scval.to_address(kp2.public_key),  # to
                    scval.to_int128(100000000 * 10**7),  # amount
                ],
            )
            .build()
        )

    @staticmethod
    def op_invoke_host_function_burn_usdc() -> TransactionEnvelope:
        return (
            common_builder()
            .append_invoke_contract_function_op(
                contract_id="CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75",
                function_name="burn",
                parameters=[
                    scval.to_address(kp0.public_key),  # from
                    scval.to_int128(100000000 * 10**7),  # amount
                ],
            )
            .build()
        )

    @staticmethod
    def op_invoke_host_function_burn_from_usdc() -> TransactionEnvelope:
        return (
            common_builder()
            .append_invoke_contract_function_op(
                contract_id="CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75",
                function_name="burn_from",
                parameters=[
                    scval.to_address(kp0.public_key),  # spender
                    scval.to_address(kp1.public_key),  # from
                    scval.to_int128(100000000 * 10**7),  # amount
                ],
            )
            .build()
        )

    @staticmethod
    def op_invoke_host_function_with_auth() -> TransactionEnvelope:
        scvals = [
            scval.to_uint128(1),
            scval.to_int128(2),
            scval.to_uint256(3),
        ]
        tx = (
            common_builder(base_fee=500)
            .append_invoke_contract_function_op(
                contract_id="CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND",
                function_name="testfunc",
                parameters=scvals,
                source=kp0.public_key,
                auth=[
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_ADDRESS,
                            stellar_xdr.SorobanAddressCredentials(
                                Address(
                                    "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7"
                                ).to_xdr_sc_address(),
                                stellar_xdr.Int64(111324345),
                                stellar_xdr.Uint32(34543543),
                                scval.to_void(),
                            ),
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(399 * 10**5),
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                    ),
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_SOURCE_ACCOUNT
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "testfunc".encode()
                                    ),
                                    args=scvals,
                                ),
                            ),
                            sub_invocations=[
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "transfer".encode()
                                            ),
                                            args=[
                                                scval.to_address(
                                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                                ),  # from
                                                scval.to_address(
                                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                ),  # to
                                                scval.to_int128(456 * 10**5),  # amount
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "approve".encode()
                                            ),
                                            args=[
                                                scval.to_address(
                                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                                ),  # from
                                                scval.to_address(
                                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                ),  # to
                                                scval.to_int128(123 * 10**5),  # amount
                                                scval.to_uint32(234524532),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                            ],
                        ),
                    ),
                ],
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_with_auth_and_no_args_and_no_source() -> (
        TransactionEnvelope
    ):
        tx = (
            common_builder(base_fee=500)
            .append_invoke_contract_function_op(
                contract_id="CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND",
                function_name="testfunc",
                parameters=None,
                auth=[
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_ADDRESS,
                            stellar_xdr.SorobanAddressCredentials(
                                Address(
                                    "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7"
                                ).to_xdr_sc_address(),
                                stellar_xdr.Int64(111324345),
                                stellar_xdr.Uint32(34543543),
                                scval.to_void(),
                            ),
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(399 * 10**5),
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                    ),
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_SOURCE_ACCOUNT
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "testfunc".encode()
                                    ),
                                    args=[],
                                ),
                            ),
                            sub_invocations=[
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "test".encode()
                                            ),
                                            args=[],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "approve".encode()
                                            ),
                                            args=[
                                                scval.to_address(
                                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                                ),  # from
                                                scval.to_address(
                                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                ),  # to
                                                scval.to_int128(123 * 10**5),  # amount
                                                scval.to_uint32(234524532),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                            ],
                        ),
                    ),
                ],
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_with_auth_and_no_args() -> TransactionEnvelope:
        tx = (
            common_builder(base_fee=500)
            .append_invoke_contract_function_op(
                contract_id="CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND",
                function_name="testfunc",
                parameters=None,
                source=kp0.public_key,
                auth=[
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_ADDRESS,
                            stellar_xdr.SorobanAddressCredentials(
                                Address(
                                    "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7"
                                ).to_xdr_sc_address(),
                                stellar_xdr.Int64(111324345),
                                stellar_xdr.Uint32(34543543),
                                scval.to_void(),
                            ),
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(399 * 10**5),
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                    ),
                    stellar_xdr.SorobanAuthorizationEntry(
                        stellar_xdr.SorobanCredentials(
                            stellar_xdr.SorobanCredentialsType.SOROBAN_CREDENTIALS_SOURCE_ACCOUNT
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "testfunc".encode()
                                    ),
                                    args=[],
                                ),
                            ),
                            sub_invocations=[
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "test".encode()
                                            ),
                                            args=[],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "approve".encode()
                                            ),
                                            args=[
                                                scval.to_address(
                                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                                ),  # from
                                                scval.to_address(
                                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                                ),  # to
                                                scval.to_int128(123 * 10**5),  # amount
                                                scval.to_uint32(234524532),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[],
                                ),
                            ],
                        ),
                    ),
                ],
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_without_auth_and_no_source() -> TransactionEnvelope:
        tx = (
            common_builder(base_fee=500)
            .append_invoke_contract_function_op(
                contract_id="CA3B55CUVQCP4C4WXGYG5I2ED7AYE6AFNJB25SFXXVWGEVP3LUVTN7ND",
                function_name="testfunc",
                parameters=None,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_approve_usdc() -> TransactionEnvelope:
        # import time
        #
        # from stellar_sdk import Keypair, Network, SorobanServer, TransactionBuilder, scval
        # from stellar_sdk.exceptions import PrepareTransactionException
        #
        # rpc_server_url = "https://patient-green-dinghy.stellar-mainnet.quiknode.pro/d92497257a021cd5ea700dae4b20496945962a4b/"
        # soroban_server = SorobanServer(rpc_server_url)
        # network_passphrase = Network.TESTNET_NETWORK_PASSPHRASE
        #
        # alice_kp = Keypair.from_public_key("GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7")
        # bob_kp = Keypair.from_public_key("GDMTVHLWJTHSUDMZVVMXXH6VJHA2ZV3HNG5LYNAZ6RTWB7GISM6PGTUV")
        # native_token_contract_id = "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
        #
        # alice_source = soroban_server.load_account(alice_kp.public_key)
        #
        # args = [
        #     scval.to_address(alice_kp.public_key),  # from
        #     scval.to_address(bob_kp.public_key),  # to
        #     scval.to_int128(125 * 10**4),  # amount, 0.125
        #     scval.to_uint32(51503809) # ledger
        # ]
        #
        # tx = (
        #     TransactionBuilder(alice_source, network_passphrase, base_fee=500)
        #     .add_time_bounds(0, 0)
        #     .append_invoke_contract_function_op(
        #         contract_id=native_token_contract_id,
        #         function_name="approve",
        #         parameters=args,
        #         source=alice_kp.public_key
        #     )
        #     .build()
        # )
        #
        # try:
        #     tx = soroban_server.prepare_transaction(tx)
        # except PrepareTransactionException as e:
        #     print(f"Got exception: {e.simulate_transaction_response}")
        #     raise e
        #
        # print(f"XDR:\n{tx.to_xdr()}")
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQABx9kDEd5JAAAAAwAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAABAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAAGAAAAAAAAAABre/OWa7lKWj3YGHUlMJSW3Vln6QpamX0me8p5WR35JYAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAoAAAAAAAAAAAAAAAAAExLQAAAAAwMR4sEAAAABAAAAAAAAAAAAAAABre/OWa7lKWj3YGHUlMJSW3Vln6QpamX0me8p5WR35JYAAAAHYXBwcm92ZQAAAAAEAAAAEgAAAAAAAAAA6TOIu/0vvRGAbdC9Wc6pB558xwznseFU8RTN/k5Gbs0AAAASAAAAAAAAAADZOp12TM8qDZmtWXuf1UnBrNdnabq8NBn0Z2D8yJM88wAAAAoAAAAAAAAAAAAAAAAAExLQAAAAAwMR4sEAAAAAAAAAAQAAAAAAAAABAAAABgAAAAGt785ZruUpaPdgYdSUwlJbdWWfpClqZfSZ7ynlZHfklgAAABQAAAABAAAAAQAAAAYAAAABre/OWa7lKWj3YGHUlMJSW3Vln6QpamX0me8p5WR35JYAAAAQAAAAAQAAAAIAAAAPAAAACUFsbG93YW5jZQAAAAAAABEAAAABAAAAAgAAAA8AAAAEZnJvbQAAABIAAAAAAAAAAOkziLv9L70RgG3QvVnOqQeefMcM57HhVPEUzf5ORm7NAAAADwAAAAdzcGVuZGVyAAAAABIAAAAAAAAAANk6nXZMzyoNma1Ze5/VScGs12dpurw0GfRnYPzIkzzzAAAAAAAwRpwAAAHcAAABLAAAAAAAAcXlAAAAAA=="
        return TransactionEnvelope.from_xdr(raw, Network.PUBLIC_NETWORK_PASSPHRASE)

    @staticmethod
    def op_invoke_host_function_scvals_case0() -> TransactionEnvelope:
        scvals = [
            scval.to_bool(True),
            scval.to_void(),
            scval.to_uint32(1234),
            scval.to_int32(12345),
            scval.to_uint64(23432453),
            scval.to_int64(454546),
            scval.to_timepoint(2356623562),
            scval.to_duration(34543643),
        ]
        tx = (
            common_builder(base_fee=500, memo=None)
            .append_invoke_contract_function_op(
                contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
                function_name="test",
                parameters=scvals,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_scvals_case1() -> TransactionEnvelope:
        scvals = [
            scval.to_duration(34543643),
            scval.to_uint128(43543645645645),
            scval.to_int128(43543645645645),
            scval.to_uint256(2**256 - 1),
            scval.to_int256(-(2**255)),
            scval.to_bytes(b"this is test bytes"),
            scval.to_string("hello this is test string"),
            scval.to_symbol("testfunc"),
            scval.to_vec([scval.to_bool(True), scval.to_bool(False)]),
        ]
        tx = (
            common_builder(base_fee=500, memo=None)
            .append_invoke_contract_function_op(
                contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
                function_name="test",
                parameters=scvals,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_scvals_case2() -> TransactionEnvelope:
        scvals = [
            scval.to_map(
                {
                    scval.to_symbol("true"): scval.to_bool(True),
                    scval.to_symbol("false"): scval.to_bool(False),
                }
            ),
            scval.to_address(kp0.public_key),
            scval.to_address(
                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
            ),
            stellar_xdr.SCVal(stellar_xdr.SCValType.SCV_LEDGER_KEY_CONTRACT_INSTANCE),
            stellar_xdr.SCVal(
                stellar_xdr.SCValType.SCV_LEDGER_KEY_NONCE,
                nonce_key=stellar_xdr.SCNonceKey(stellar_xdr.Int64(100)),
            ),
            stellar_xdr.SCVal(
                stellar_xdr.SCValType.SCV_CONTRACT_INSTANCE,
                instance=stellar_xdr.SCContractInstance(
                    executable=stellar_xdr.ContractExecutable(
                        stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_STELLAR_ASSET
                    ),
                    storage=None,
                ),
            ),
            stellar_xdr.SCVal(
                stellar_xdr.SCValType.SCV_CONTRACT_INSTANCE,
                instance=stellar_xdr.SCContractInstance(
                    executable=stellar_xdr.ContractExecutable(
                        stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
                        wasm_hash=stellar_xdr.Hash(
                            b"\xcf\x88\x84S\xd6`V\xbc\xb6\xaeY*\x91\x90s\xb5\x93\xb5\x96[\xff\xcb\xcf\xc3\x04\xacGT\x9e\xac\xda\xd6"
                        ),
                    ),
                    storage=scval.to_map(
                        {
                            scval.to_symbol("true"): scval.to_bool(True),
                            scval.to_symbol("false"): scval.to_bool(False),
                        }
                    ).map,
                ),
            ),
        ]
        tx = (
            common_builder(base_fee=500, memo=None)
            .append_invoke_contract_function_op(
                contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
                function_name="test",
                parameters=scvals,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_scvals_case3() -> TransactionEnvelope:
        scvals = [
            scval.to_address(
                "MAUPUK7AMSD6JHZCDVIOZQ2O4IHDVCLAJMKXZHOR6UOOT7T3ZFJV2AAAAAAAAAPA6OEF6"
            ),
            scval.to_address(
                "BAAD6DBUX6J22DMZOHIEZTEQ64CVCHEDRKWZONFEUL5Q26QD7R76RGR4TU"
            ),
            scval.to_address(
                "LA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUPJN"
            ),
        ]
        tx = (
            common_builder(base_fee=500, memo=None)
            .append_invoke_contract_function_op(
                contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
                function_name="test",
                parameters=scvals,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_invoke_host_function_scvals_case4() -> TransactionEnvelope:
        scvals = [
            scval.to_map(
                {
                    scval.to_address(
                        Keypair.from_mnemonic_phrase(MNEMONIC, index=3).public_key
                    ): scval.to_vec(
                        [
                            scval.to_int64(1),
                            scval.to_bool(True),
                            scval.to_string("hey!"),
                            scval.to_map(
                                {
                                    scval.to_address(
                                        Keypair.from_mnemonic_phrase(
                                            MNEMONIC, index=4
                                        ).public_key
                                    ): scval.to_vec(
                                        [
                                            scval.to_int32(1),
                                            scval.to_int32(2),
                                            scval.to_int32(3),
                                        ]
                                    )
                                }
                            ),
                        ]
                    ),
                    scval.to_address(
                        Keypair.from_mnemonic_phrase(MNEMONIC, index=5).public_key
                    ): scval.to_vec(
                        [
                            scval.to_int64(9),
                            scval.to_bool(False),
                            scval.to_string("hello!"),
                            scval.to_map(
                                {
                                    scval.to_address(
                                        Keypair.from_mnemonic_phrase(
                                            MNEMONIC, index=6
                                        ).public_key
                                    ): scval.to_vec(
                                        [
                                            scval.to_int32(4),
                                            scval.to_int32(5),
                                            scval.to_int32(6),
                                        ]
                                    )
                                }
                            ),
                        ]
                    ),
                },
            ),
            scval.to_vec(
                [
                    scval.to_map(
                        {
                            scval.to_symbol("a"): scval.to_int32(1),
                            scval.to_symbol("b"): scval.to_int32(2),
                        }
                    ),
                    scval.to_map(
                        {
                            scval.to_symbol("c"): scval.to_int32(3),
                            scval.to_symbol("d"): scval.to_int32(4),
                        }
                    ),
                ]
            ),
        ]
        tx = (
            common_builder(base_fee=500, memo=None)
            .append_invoke_contract_function_op(
                contract_id="CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC",
                function_name="test",
                parameters=scvals,
            )
            .add_time_bounds(0, 0)
            .build()
        )
        return tx

    @staticmethod
    def op_extend_footprint_ttl() -> TransactionEnvelope:
        """
        soroban --very-verbose contract bump --ledgers-to-expire 130816 \
            --durability persistent --id CACEIKVZTU7Z6VKNISE3OO5MXSCKUC7HC2FNCWRO2HJMWSUPUWHDLSJE \
            --source SAIYWGGWU2WMXYDSK33UBQBMBDKU4TTJVY3ZIFF24H2KQDR7RQW5KAEK \
            --rpc-url https://soroban-testnet.stellar.org:443 \
            --network-passphrase 'Test SDF Network ; September 2015'
        """
        raw = "AAAAAgAAAADpM4i7/S+9EYBt0L1ZzqkHnnzHDOex4VTxFM3+TkZuzQAAn9IALZ/tAAAABQAAAAAAAAAAAAAAAQAAAAAAAAAZAAAAAAAB/wAAAAABAAAAAAAAAAEAAAAGAAAAAQREKrmdP59VTUSJtzusvISqC+cWitFaLtHSy0qPpY41AAAAFAAAAAEAAAAAAAAAAAAAAJgAAAAAAAAAAAAAdogAAAABTkZuzQAAAEAQIX09qLt+SIcA7sOc7XGSWjK98FFURHW77g8uWm4lQirDqZU51B0uatCZe90mSt+RK7r7it3I92JSUL1Ba+EA"
        return TransactionEnvelope.from_xdr(raw, Network.TESTNET_NETWORK_PASSPHRASE)

    @staticmethod
    def op_restore_footprint() -> TransactionEnvelope:
        # TODO: add soroban resource?
        return common_builder().append_restore_footprint_op().build()

    @staticmethod
    def op_with_source() -> TransactionEnvelope:
        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="922337203685.4775807",
                source=kp0.public_key,
            )
            .build()
        )

    @staticmethod
    def op_with_muxed_source() -> TransactionEnvelope:
        muxed_account = MuxedAccount(account_id=kp0.public_key, account_muxed_id=10000)

        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="922337203685.4775807",
                source=muxed_account,
            )
            .build()
        )

    @staticmethod
    def tx_memo_none() -> TransactionEnvelope:
        return (
            common_builder(memo=None)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_memo_id() -> TransactionEnvelope:
        return (
            common_builder(memo=IdMemo(18446744073709551615))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_memo_text() -> TransactionEnvelope:
        return (
            common_builder(memo=TextMemo("hello world 123456789 123456"))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_memo_text_unprintable() -> TransactionEnvelope:
        return (
            common_builder(memo=TextMemo("这是一条测试消息 hey"))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_memo_hash() -> TransactionEnvelope:
        return (
            common_builder(
                memo=HashMemo(
                    "573c10b148fc4bc7db97540ce49da22930f4bcd48a060dc7347be84ea9f52d9f"
                )
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_memo_return_hash() -> TransactionEnvelope:
        return (
            common_builder(
                memo=ReturnHashMemo(
                    "573c10b148fc4bc7db97540ce49da22930f4bcd48a060dc7347be84ea9f52d9f"
                )
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_with_all_items() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(1657951297, 1670818332))
            .set_min_sequence_number(103420918407103888)
            .set_min_sequence_age(1649239999)
            .set_min_sequence_ledger_gap(30)
            .set_ledger_bounds(40351800, 40352000)
            .add_extra_signer(
                "GBJCHUKZMTFSLOMNC7P4TS4VJJBTCYL3XKSOLXAUJSD56C4LHND5TWUC"
            )
            .add_extra_signer(
                "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM"
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_is_none() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=None)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_time_bounds() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(1657951297, 1670818332))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_time_bounds_max_is_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(1657951297, 0))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_time_bounds_min_is_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 1670818332))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_time_bounds_are_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_time_bounds_is_none() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=None)
            .set_ledger_bounds(40351800, 40352000)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_ledger_bounds() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_ledger_bounds(40351800, 40352000)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_ledger_bounds_max_is_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_ledger_bounds(40351800, 0)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_ledger_bounds_min_is_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_ledger_bounds(0, 40352000)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_ledger_bounds_are_zero() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_ledger_bounds(0, 0)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_min_account_sequence() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_min_sequence_number(103420918407103888)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_min_account_sequence_age() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_min_sequence_age(1649239999)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_min_account_sequence_ledger_gap() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .set_min_sequence_ledger_gap(30)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_extra_signers_with_one_signer() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .add_extra_signer(
                "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM"
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_cond_extra_signers_with_two_signers() -> TransactionEnvelope:
        return (
            common_builder(time_bounds=TimeBounds(0, 0))
            .add_extra_signer(
                "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBEFAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM"
            )
            .add_extra_signer(
                "GBJCHUKZMTFSLOMNC7P4TS4VJJBTCYL3XKSOLXAUJSD56C4LHND5TWUC"
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_multi_operations() -> TransactionEnvelope:
        return (
            common_builder()
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="922337203685.4775807",
            )
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset(
                    "BTC", "GATEMHCCKCY67ZUCKTROYN24ZYT5GK4EQZ65JJLDHKHRUZI3EUEKMTCH"
                ),
                amount="922337203685.4775807",
            )
            .append_set_options_op(home_domain="stellar.org")
            .build()
        )

    @staticmethod
    def tx_custom_base_fee() -> TransactionEnvelope:
        return (
            common_builder(base_fee=1275)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .append_payment_op(
                destination=kp2.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_with_muxed_source() -> TransactionEnvelope:
        return (
            common_builder(source_muxed_id=10000)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    # Used to test the case where `ENABLE_TRANSACTION_SOURCE` is not enabled.
    @staticmethod
    def tx_with_different_source() -> TransactionEnvelope:
        return (
            common_builder(source=kp1)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )

    @staticmethod
    def tx_network_public() -> TransactionEnvelope:
        return (
            common_builder(network=Network.PUBLIC_NETWORK_PASSPHRASE)
            .append_bump_sequence_op(bump_to=1232134324234)
            .build()
        )

    @staticmethod
    def tx_network_testnet() -> TransactionEnvelope:
        return (
            common_builder(network=Network.TESTNET_NETWORK_PASSPHRASE)
            .append_bump_sequence_op(bump_to=1232134324234)
            .build()
        )

    @staticmethod
    def tx_network_custom() -> TransactionEnvelope:
        return (
            common_builder(network="Custom Network; October 2025")
            .append_bump_sequence_op(bump_to=1232134324234)
            .build()
        )

    @staticmethod
    def fee_bump_tx() -> FeeBumpTransactionEnvelope:
        inner_tx = (
            common_builder(base_fee=50)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .append_payment_op(
                destination=kp2.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .build()
        )
        inner_tx.sign(kp0)
        fee_bump_tx = TransactionBuilder.build_fee_bump_transaction(
            fee_source=kp0.public_key,
            base_fee=750,
            inner_transaction_envelope=inner_tx,
            network_passphrase=Network.PUBLIC_NETWORK_PASSPHRASE,
        )
        return fee_bump_tx

    @staticmethod
    def fee_bump_tx_with_muxed_fee_source() -> FeeBumpTransactionEnvelope:
        inner_tx = (
            common_builder(base_fee=50)
            .append_payment_op(
                destination=kp1.public_key,
                asset=Asset.native(),
                amount="1",
            )
            .append_payment_op(
                destination=kp2.public_key,
                asset=Asset.native(),
                amount="1",
                source=kp1.public_key,
            )
            .build()
        )
        inner_tx.sign(kp0)
        inner_tx.sign(kp1)

        muxed_account = MuxedAccount(account_id=kp0.public_key, account_muxed_id=10000)
        fee_bump_tx = TransactionBuilder.build_fee_bump_transaction(
            fee_source=muxed_account,
            base_fee=750,
            inner_transaction_envelope=inner_tx,
            network_passphrase=Network.PUBLIC_NETWORK_PASSPHRASE,
        )
        return fee_bump_tx


class SignSorobanAuthorizationTestCases:
    @staticmethod
    def soroban_auth_network_testnet():
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.TESTNET_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("transfer".encode()),
                            args=[
                                scval.to_address(
                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                ),  # from
                                scval.to_address(
                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                ),  # to
                                scval.to_int128(103560 * 10**5),  # amount, 100 XLM
                            ],
                        ),
                    ),
                    sub_invocations=[],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_network_public():
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("transfer".encode()),
                            args=[
                                scval.to_address(
                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                ),  # from
                                scval.to_address(
                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                ),  # to
                                scval.to_int128(103560 * 10**5),  # amount, 100 XLM
                            ],
                        ),
                    ),
                    sub_invocations=[],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_network_custom():
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network("Custom Network; October 2025").network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("transfer".encode()),
                            args=[
                                scval.to_address(
                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                ),  # from
                                scval.to_address(
                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                ),  # to
                                scval.to_int128(103560 * 10**5),  # amount, 100 XLM
                            ],
                        ),
                    ),
                    sub_invocations=[],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_create_smart_contract() -> stellar_xdr.HashIDPreimage:
        create_contract = stellar_xdr.CreateContractArgs(
            contract_id_preimage=stellar_xdr.ContractIDPreimage(
                stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ADDRESS,
                from_address=stellar_xdr.ContractIDPreimageFromAddress(
                    address=Address(
                        "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                    ).to_xdr_sc_address(),
                    salt=stellar_xdr.Uint256(
                        b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                    ),
                ),
            ),
            executable=stellar_xdr.ContractExecutable(
                stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
                stellar_xdr.Hash(
                    b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                ),
            ),
        )
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN,
                        create_contract_host_fn=create_contract,
                    ),
                    sub_invocations=[],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_create_smart_contract_v2() -> stellar_xdr.HashIDPreimage:
        create_contract = stellar_xdr.CreateContractArgsV2(
            contract_id_preimage=stellar_xdr.ContractIDPreimage(
                stellar_xdr.ContractIDPreimageType.CONTRACT_ID_PREIMAGE_FROM_ADDRESS,
                from_address=stellar_xdr.ContractIDPreimageFromAddress(
                    address=Address(
                        "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                    ).to_xdr_sc_address(),
                    salt=stellar_xdr.Uint256(
                        b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                    ),
                ),
            ),
            executable=stellar_xdr.ContractExecutable(
                stellar_xdr.ContractExecutableType.CONTRACT_EXECUTABLE_WASM,
                stellar_xdr.Hash(
                    b"\xd9\x9f\x1f\xee4N\xeb\xd80}\xeb\x9f\xf4$W\xd8\xdb\x12\xeeS')\x18\xfe48\x02q\xc1\xd4\x10\n"
                ),
            ),
            constructor_args=[
                scval.to_address(
                    "GDAT5HWTGIU4TSSZ4752OUC4SABDLTLZFRPZUJ3D6LKBNEPA7V2CIG54"
                ),
                scval.to_int128(12000000),
            ],
        )
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_V2_HOST_FN,
                        create_contract_v2_host_fn=create_contract,
                    ),
                    sub_invocations=[
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    Address(
                                        "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                    ).to_xdr_sc_address(),
                                    stellar_xdr.SCSymbol("__constructor".encode()),
                                    [
                                        scval.to_address(
                                            "GDAT5HWTGIU4TSSZ4752OUC4SABDLTLZFRPZUJ3D6LKBNEPA7V2CIG54"
                                        ),
                                        scval.to_int128(12000000),
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        )
                    ],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_invoke_contract() -> stellar_xdr.HashIDPreimage:
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("transfer".encode()),
                            args=[
                                scval.to_address(
                                    "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                ),  # from
                                scval.to_address(
                                    "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                ),  # to
                                scval.to_int128(103560 * 10**5),  # amount, 100 XLM
                            ],
                        ),
                    ),
                    sub_invocations=[
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(
                                            123456 * 10**5
                                        ),  # amount, 100 XLM
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "demofunc".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(
                                            234234 * 10**5
                                        ),  # amount, 100 XLM
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                    ],
                ),
            ),
        )
        return data

    # sorobanAuthInvokeContractWithoutArgs
    @staticmethod
    def soroban_auth_invoke_contract_without_args() -> stellar_xdr.HashIDPreimage:
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("testfunc".encode()),
                            args=[],
                        ),
                    ),
                    sub_invocations=[
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(
                                            103560 * 10**5
                                        ),  # amount, 100 XLM
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "transfer".encode()
                                    ),
                                    args=[
                                        scval.to_address(
                                            "GCWNBLOHPARYAAF5W25NELURTERYS732Q7RRBTXRKBPGYCYLOFKCLKKA"
                                        ),  # from
                                        scval.to_address(
                                            "GB42LIJ3V5KXCY32EFL4NL73OSI5PRCFJ3WNFMFX4QHGOAR7BFX2YC34"
                                        ),  # to
                                        scval.to_int128(
                                            103560 * 10**5
                                        ),  # amount, 100 XLM
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                    ],
                ),
            ),
        )
        return data

    @staticmethod
    def soroban_auth_invoke_contract_with_complex_sub_invocation() -> (
        stellar_xdr.HashIDPreimage
    ):
        data = stellar_xdr.HashIDPreimage(
            stellar_xdr.EnvelopeType.ENVELOPE_TYPE_SOROBAN_AUTHORIZATION,
            soroban_authorization=stellar_xdr.HashIDPreimageSorobanAuthorization(
                network_id=stellar_xdr.Hash(
                    Network(Network.PUBLIC_NETWORK_PASSPHRASE).network_id()
                ),
                nonce=stellar_xdr.Int64(1232432453),
                signature_expiration_ledger=stellar_xdr.Uint32(34654367),
                invocation=stellar_xdr.SorobanAuthorizedInvocation(
                    function=stellar_xdr.SorobanAuthorizedFunction(
                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                        contract_fn=stellar_xdr.InvokeContractArgs(
                            contract_address=Address(
                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                            ).to_xdr_sc_address(),
                            function_name=stellar_xdr.SCSymbol("func0".encode()),
                            args=[
                                scval.to_int128(103560 * 10**5),
                            ],
                        ),
                    ),
                    sub_invocations=[
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "func1".encode()
                                    ),
                                    args=[
                                        scval.to_int128(103560 * 10**5),
                                    ],
                                ),
                            ),
                            sub_invocations=[],
                        ),
                        stellar_xdr.SorobanAuthorizedInvocation(
                            function=stellar_xdr.SorobanAuthorizedFunction(
                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                contract_fn=stellar_xdr.InvokeContractArgs(
                                    contract_address=Address(
                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                    ).to_xdr_sc_address(),
                                    function_name=stellar_xdr.SCSymbol(
                                        "func2".encode()
                                    ),
                                    args=[
                                        scval.to_int128(103560 * 10**5),
                                    ],
                                ),
                            ),
                            sub_invocations=[
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "func3".encode()
                                            ),
                                            args=[
                                                scval.to_int128(103560 * 10**5),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func4".encode()
                                                    ),
                                                    args=[
                                                        scval.to_int128(103560 * 10**5),
                                                    ],
                                                ),
                                            ),
                                            sub_invocations=[],
                                        ),
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func5".encode()
                                                    ),
                                                    args=[
                                                        scval.to_int128(103560 * 10**5),
                                                    ],
                                                ),
                                            ),
                                            sub_invocations=[
                                                stellar_xdr.SorobanAuthorizedInvocation(
                                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                                            contract_address=Address(
                                                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                            ).to_xdr_sc_address(),
                                                            function_name=stellar_xdr.SCSymbol(
                                                                "func6".encode()
                                                            ),
                                                            args=[
                                                                scval.to_int128(
                                                                    103560 * 10**5
                                                                ),
                                                            ],
                                                        ),
                                                    ),
                                                    sub_invocations=[
                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                    contract_address=Address(
                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                    ).to_xdr_sc_address(),
                                                                    function_name=stellar_xdr.SCSymbol(
                                                                        "func7".encode()
                                                                    ),
                                                                    args=[
                                                                        scval.to_int128(
                                                                            103560
                                                                            * 10**5
                                                                        ),
                                                                    ],
                                                                ),
                                                            ),
                                                            sub_invocations=[],
                                                        ),
                                                        stellar_xdr.SorobanAuthorizedInvocation(
                                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                                    contract_address=Address(
                                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                                    ).to_xdr_sc_address(),
                                                                    function_name=stellar_xdr.SCSymbol(
                                                                        "func8".encode()
                                                                    ),
                                                                    args=[
                                                                        scval.to_int128(
                                                                            103560
                                                                            * 10**5
                                                                        ),
                                                                    ],
                                                                ),
                                                            ),
                                                            sub_invocations=[],
                                                        ),
                                                    ],
                                                )
                                            ],
                                        ),
                                    ],
                                ),
                                stellar_xdr.SorobanAuthorizedInvocation(
                                    function=stellar_xdr.SorobanAuthorizedFunction(
                                        stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                        contract_fn=stellar_xdr.InvokeContractArgs(
                                            contract_address=Address(
                                                "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                            ).to_xdr_sc_address(),
                                            function_name=stellar_xdr.SCSymbol(
                                                "func9".encode()
                                            ),
                                            args=[
                                                scval.to_int128(103560 * 10**5),
                                            ],
                                        ),
                                    ),
                                    sub_invocations=[
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func10".encode()
                                                    ),
                                                    args=[
                                                        scval.to_int128(103560 * 10**5),
                                                    ],
                                                ),
                                            ),
                                            sub_invocations=[],
                                        ),
                                        stellar_xdr.SorobanAuthorizedInvocation(
                                            function=stellar_xdr.SorobanAuthorizedFunction(
                                                stellar_xdr.SorobanAuthorizedFunctionType.SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN,
                                                contract_fn=stellar_xdr.InvokeContractArgs(
                                                    contract_address=Address(
                                                        "CDLZFC3SYJYDZT7K67VZ75HPJVIEUVNIXF47ZG2FB2RMQQVU2HHGCYSC"
                                                    ).to_xdr_sc_address(),
                                                    function_name=stellar_xdr.SCSymbol(
                                                        "func11".encode()
                                                    ),
                                                    args=[
                                                        scval.to_int128(103560 * 10**5),
                                                    ],
                                                ),
                                            ),
                                            sub_invocations=[],
                                        ),
                                    ],
                                ),
                            ],
                        ),
                    ],
                ),
            ),
        )
        return data

from contextlib import contextmanager
from enum import IntEnum
from typing import Generator, List, Optional

from ragger.backend.interface import RAPDU, BackendInterface
from ragger.bip import pack_derivation_path

MAX_APDU_LEN: int = 255

CLA: int = 0xE0


class P1(IntEnum):
    # Parameter 1 for first APDU number.
    FIRST_APDU = 0x00
    # Parameter 1 for non-first APDU number.
    MORE_APDU = 0x80


class P2(IntEnum):
    # Parameter 2 for last APDU to receive.
    LAST_APDU = 0x00
    # Parameter 2 for more APDU to receive.
    MORE_APDU = 0x80
    # Parameter 2 for no need to ask user confirmation.
    NON_CONFIRM = 0x00
    # Parameter 2 for ask user confirmation.
    CONFIRM = 0x01


class InsType(IntEnum):
    GET_PUBLIC_KEY = 0x02
    SIGN_TX = 0x04
    GET_CONF = 0x06
    SIGN_HASH = 0x08
    SIGN_SOROBAN_AUTH = 0x0A
    SIGN_MESSAGE = 0x0C


class Errors(IntEnum):
    SW_CLA_NOT_SUPPORTED = 0x6E00
    SW_DENY = 0x6985
    SW_KEY_DERIVE_FAIL = 0xB001
    SW_ADDR_DISPLAY_FAIL = 0xB002
    SW_REQUEST_DATA_TOO_LARGE = 0xB004
    SW_DATA_PARSING_FAIL = 0xB005
    SW_DATA_HASH_FAIL = 0xB006
    SW_DATA_SIGN_FAIL = 0xB008
    SW_DATA_FORMATTING_FAIL = 0x6125
    SW_WRONG_APDU_LENGTH = 0x6A87
    SW_WRONG_P1_P2 = 0x6B00
    SW_INS_NOT_SUPPORTED = 0x6D00
    SW_BLIND_SIGNING_MODE_NOT_ENABLED = 0x6C66


def split_message(message: bytes, max_size: int) -> List[bytes]:
    return [message[x : x + max_size] for x in range(0, len(message), max_size)]


class StellarCommandSender:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend

    def _send_initial_apdu_with_path(self, ins: InsType, path: str) -> None:
        """Send initial APDU with derivation path."""
        self.backend.exchange(
            cla=CLA,
            ins=ins,
            p1=P1.FIRST_APDU,
            p2=P2.MORE_APDU,
            data=pack_derivation_path(path),
        )

    def _send_data_chunks(self, ins: InsType, messages: List[bytes]) -> None:
        """Send all data chunks except the last one."""
        for msg in messages[:-1]:
            self.backend.exchange(
                cla=CLA, ins=ins, p1=P1.MORE_APDU, p2=P2.MORE_APDU, data=msg
            )

    def _send_final_chunk_async(self, ins: InsType, message: bytes):
        """Send final chunk asynchronously and return context manager."""
        return self.backend.exchange_async(
            cla=CLA,
            ins=ins,
            p1=P1.MORE_APDU,
            p2=P2.LAST_APDU,
            data=message,
        )

    def _send_final_chunk_sync(self, ins: InsType, message: bytes) -> RAPDU:
        """Send final chunk synchronously and return RAPDU."""
        return self.backend.exchange(
            cla=CLA,
            ins=ins,
            p1=P1.MORE_APDU,
            p2=P2.LAST_APDU,
            data=message,
        )

    def get_configuration(self) -> RAPDU:
        return self.backend.exchange(
            cla=CLA, ins=InsType.GET_CONF, p1=P1.FIRST_APDU, p2=P2.LAST_APDU
        )

    def get_public_key(self, path: str) -> RAPDU:
        return self.backend.exchange(
            cla=CLA,
            ins=InsType.GET_PUBLIC_KEY,
            p1=P1.FIRST_APDU,
            p2=P2.NON_CONFIRM,
            data=pack_derivation_path(path),
        )

    @contextmanager
    def sign_hash(self, path: str, hash_to_sign: bytes) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.SIGN_HASH,
            p1=P1.FIRST_APDU,
            p2=P2.LAST_APDU,
            data=pack_derivation_path(path) + hash_to_sign,
        ) as response:
            yield response

    @contextmanager
    def get_public_key_with_confirmation(
        self, path: str
    ) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.GET_PUBLIC_KEY,
            p1=P1.FIRST_APDU,
            p2=P2.CONFIRM,
            data=pack_derivation_path(path),
        ) as response:
            yield response

    @contextmanager
    def sign_tx(self, path: str, transaction: bytes) -> Generator[None, None, None]:
        self._send_initial_apdu_with_path(InsType.SIGN_TX, path)
        messages = split_message(transaction, MAX_APDU_LEN)
        self._send_data_chunks(InsType.SIGN_TX, messages)
        with self._send_final_chunk_async(InsType.SIGN_TX, messages[-1]) as response:
            yield response

    @contextmanager
    def sign_soroban_auth(
        self, path: str, soroban_authorization: bytes
    ) -> Generator[None, None, None]:
        self._send_initial_apdu_with_path(InsType.SIGN_SOROBAN_AUTH, path)
        messages = split_message(soroban_authorization, MAX_APDU_LEN)
        self._send_data_chunks(InsType.SIGN_SOROBAN_AUTH, messages)
        with self._send_final_chunk_async(
            InsType.SIGN_SOROBAN_AUTH, messages[-1]
        ) as response:
            yield response

    @contextmanager
    def sign_message(self, path: str, message: bytes) -> Generator[None, None, None]:
        self._send_initial_apdu_with_path(InsType.SIGN_MESSAGE, path)
        messages = split_message(message, MAX_APDU_LEN)
        self._send_data_chunks(InsType.SIGN_MESSAGE, messages)
        with self._send_final_chunk_async(
            InsType.SIGN_MESSAGE, messages[-1]
        ) as response:
            yield response

    def get_async_response(self) -> Optional[RAPDU]:
        return self.backend.last_async_response

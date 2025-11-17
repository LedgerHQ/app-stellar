//! Zero-copy XDR parser for Stellar blockchain data structures

extern crate alloc;

use alloc::boxed::Box;
use alloc::vec::Vec;
use core::{fmt, str};

/// Stellar Testnet network hash
pub const TESTNET_NETWORK_HASH: [u8; 32] = [
    0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32, 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
    0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e, 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72,
];

/// Stellar Public network hash
pub const PUBLIC_NETWORK_HASH: [u8; 32] = [
    0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75, 0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
    0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26, 0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79,
];

/// Parser errors for XDR decoding
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParseError {
    BufferOverflow,
    InvalidType(i32),
    InvalidPadding,
    LengthExceedsMax { actual: usize, max: usize },
    MaxDepthExceeded { depth: usize, max: usize },
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ParseError::BufferOverflow => write!(f, "Buffer overflow: not enough bytes to read"),
            ParseError::InvalidType(val) => write!(f, "Invalid type value: {}", val),
            ParseError::InvalidPadding => write!(f, "Invalid padding: non-zero bytes in padding"),
            ParseError::LengthExceedsMax { actual, max } => {
                write!(f, "Length {} exceeds maximum allowed {}", actual, max)
            }
            ParseError::MaxDepthExceeded { depth, max } => {
                write!(
                    f,
                    "Parse depth {} exceeds maximum allowed depth {}",
                    depth, max
                )
            }
        }
    }
}

/// Maximum allowed parsing depth to prevent stack overflow
const MAX_PARSE_DEPTH: usize = 32;

/// Zero-copy XDR parser with lifetime tracking
pub struct Parser<'a> {
    data: &'a [u8],
    offset: usize,
    depth: usize,
}

impl<'a> Parser<'a> {
    /// Create a new parser from byte slice
    pub fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            offset: 0,
            depth: 0,
        }
    }

    /// Get current offset
    pub fn offset(&self) -> usize {
        self.offset
    }

    /// Enter a recursive parsing context, checking depth limit
    #[inline]
    pub(crate) fn enter_recursion(&mut self) -> Result<(), ParseError> {
        if self.depth >= MAX_PARSE_DEPTH {
            return Err(ParseError::MaxDepthExceeded {
                depth: self.depth + 1,
                max: MAX_PARSE_DEPTH,
            });
        }
        self.depth += 1;
        Ok(())
    }

    /// Exit a recursive parsing context
    #[inline]
    pub(crate) fn exit_recursion(&mut self) {
        self.depth = self.depth.saturating_sub(1);
    }

    /// Parse with depth tracking for recursive types
    #[inline]
    pub fn parse_with_depth<T: XdrParse<'a>>(&mut self) -> Result<T, ParseError> {
        self.enter_recursion()?;
        let result = T::parse(self);
        self.exit_recursion();
        result
    }

    /// Check if we can read n bytes
    #[inline]
    fn can_read(&self, n: usize) -> bool {
        self.offset.saturating_add(n) <= self.data.len()
    }

    /// Read raw bytes without copying (zero-copy)
    fn read_bytes(&mut self, size: usize) -> Result<&'a [u8], ParseError> {
        if !self.can_read(size) {
            return Err(ParseError::BufferOverflow);
        }
        let end = self.offset + size;
        let bytes = &self.data[self.offset..end];
        self.offset = end;
        Ok(bytes)
    }

    /// Read exactly N bytes and convert to array
    #[inline]
    fn read_array<const N: usize>(&mut self) -> Result<[u8; N], ParseError> {
        let bytes = self.read_bytes(N)?;
        bytes.try_into().map_err(|_| ParseError::BufferOverflow)
    }

    /// Read exactly N bytes and return as array reference
    #[inline]
    fn read_array_ref<const N: usize>(&mut self) -> Result<&'a [u8; N], ParseError> {
        let bytes = self.read_bytes(N)?;
        bytes.try_into().map_err(|_| ParseError::BufferOverflow)
    }

    /// Parse uint32 (big-endian)
    #[inline]
    pub fn parse_uint32(&mut self) -> Result<u32, ParseError> {
        Ok(u32::from_be_bytes(self.read_array()?))
    }

    /// Parse int32 (big-endian)
    #[inline]
    pub fn parse_int32(&mut self) -> Result<i32, ParseError> {
        Ok(i32::from_be_bytes(self.read_array()?))
    }

    /// Parse uint64 (big-endian)
    #[inline]
    pub fn parse_uint64(&mut self) -> Result<u64, ParseError> {
        Ok(u64::from_be_bytes(self.read_array()?))
    }

    /// Parse int64 (big-endian)
    #[inline]
    pub fn parse_int64(&mut self) -> Result<i64, ParseError> {
        Ok(i64::from_be_bytes(self.read_array()?))
    }

    /// Parse boolean (encoded as uint32)
    pub fn parse_bool(&mut self) -> Result<bool, ParseError> {
        match self.parse_uint32()? {
            0 => Ok(false),
            1 => Ok(true),
            val => Err(ParseError::InvalidType(val as i32)),
        }
    }

    /// Calculate XDR-aligned size (multiple of 4)
    fn aligned_size(size: usize) -> Result<usize, ParseError> {
        let remainder = size % 4;
        if remainder == 0 {
            Ok(size)
        } else {
            size.checked_add(4 - remainder)
                .ok_or(ParseError::BufferOverflow)
        }
    }

    /// Check padding bytes are zero
    #[inline]
    fn check_padding(data: &[u8], valid_len: usize, total_len: usize) -> Result<(), ParseError> {
        // Use iterator instead of index loop (clippy suggestion)
        if data[valid_len..total_len].iter().any(|&b| b != 0) {
            return Err(ParseError::InvalidPadding);
        }
        Ok(())
    }

    /// Parse variable-length opaque data (with length prefix and padding)
    /// This follows XDR spec for variable-length opaque: length + data + padding
    pub fn parse_var_opaque(&mut self, max_length: Option<usize>) -> Result<&'a [u8], ParseError> {
        let size = self.parse_uint32()? as usize;

        if let Some(max_len) = max_length {
            if size > max_len {
                return Err(ParseError::LengthExceedsMax {
                    actual: size,
                    max: max_len,
                });
            }
        }

        let aligned_size = Self::aligned_size(size)?;
        let data = self.read_bytes(aligned_size)?;

        // Security check: validate padding
        Self::check_padding(data, size, aligned_size)?;

        Ok(&data[..size])
    }

    /// Parse optional value
    pub fn parse_optional<T>(
        &mut self,
        parser: impl FnOnce(&mut Self) -> Result<T, ParseError>,
    ) -> Result<Option<T>, ParseError> {
        if self.parse_bool()? {
            // No need for depth tracking here as the called parser should handle it
            Ok(Some(parser(self)?))
        } else {
            Ok(None)
        }
    }

    /// Parse any type implementing XdrParse
    pub fn parse<T: XdrParse<'a>>(&mut self) -> Result<T, ParseError> {
        T::parse(self)
    }
}

/// Trait for types that can be parsed from XDR
pub trait XdrParse<'a>: Sized {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError>;
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Uint256<'a>(pub &'a [u8; 32]);

impl<'a> Uint256<'a> {
    pub fn as_bytes(&self) -> &[u8; 32] {
        self.0
    }
}

impl<'a> XdrParse<'a> for Uint256<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        Ok(Uint256(parser.read_array_ref()?))
    }
}

pub type Hash<'a> = Uint256<'a>;
pub type TimePoint = u64;
pub type Duration = u64;
pub type SequenceNumber = i64;

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AccountFlags {
    RequiredFlag = 0x1,
    RevocableFlag = 0x2,
    ImmutableFlag = 0x4,
    ClawbackEnabledFlag = 0x8,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MemoType {
    None = 0,
    Text = 1,
    Id = 2,
    Hash = 3,
    Return = 4,
}

impl<'a> XdrParse<'a> for MemoType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(MemoType::None),
            1 => Ok(MemoType::Text),
            2 => Ok(MemoType::Id),
            3 => Ok(MemoType::Hash),
            4 => Ok(MemoType::Return),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

/// String with max length constraint (zero-copy, references original data)
///
/// In embedded environment, we use a reference to the original data instead of `Vec<u8>`
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct StringM<'a, const MAX: usize> {
    data: &'a [u8],
}

impl<'a, const MAX: usize> StringM<'a, MAX> {
    /// Create a new StringM from byte slice (mainly for testing)
    pub fn new(data: &'a [u8]) -> Self {
        Self { data }
    }

    /// Get the string data as bytes
    pub fn as_bytes(&self) -> &'a [u8] {
        self.data
    }

    /// Get the string as str (if valid UTF-8)
    pub fn as_str(&self) -> Result<&'a str, core::str::Utf8Error> {
        core::str::from_utf8(self.data)
    }

    /// Get the length of the string
    pub fn len(&self) -> usize {
        self.data.len()
    }

    /// Check if the string is empty
    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }
}

impl<'a, const MAX: usize> XdrParse<'a> for StringM<'a, MAX> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let data = parser.parse_var_opaque(Some(MAX))?;
        Ok(StringM { data })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BytesM<'a, const MAX: usize> {
    data: &'a [u8],
}

impl<'a, const MAX: usize> BytesM<'a, MAX> {
    /// Create a new BytesM from byte slice (mainly for testing)
    pub fn new(data: &'a [u8]) -> Self {
        Self { data }
    }

    /// Get the byte data
    pub fn as_bytes(&self) -> &'a [u8] {
        self.data
    }

    /// Get the length of the byte array
    pub fn len(&self) -> usize {
        self.data.len()
    }

    /// Check if the byte array is empty
    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }
}

impl<'a, const MAX: usize> XdrParse<'a> for BytesM<'a, MAX> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let data = parser.parse_var_opaque(Some(MAX))?;
        Ok(BytesM { data })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct VecM<T, const MAX: u32 = { u32::MAX }>(Vec<T>);

impl<T, const MAX: u32> VecM<T, MAX> {
    /// Create a new VecM from a Vec (mainly for testing)
    pub fn new(data: Vec<T>) -> Self {
        Self(data)
    }

    pub fn as_slice(&self) -> &[T] {
        &self.0
    }

    pub fn len(&self) -> usize {
        self.0.len()
    }

    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    pub fn iter(&self) -> core::slice::Iter<'_, T> {
        self.0.iter()
    }
}

impl<'a, T: XdrParse<'a>, const MAX: u32> XdrParse<'a> for VecM<T, MAX> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let len = parser.parse_uint32()?;
        if len > MAX {
            return Err(ParseError::LengthExceedsMax {
                actual: len as usize,
                max: MAX as usize,
            });
        }

        // Enter recursion context for VecM parsing
        parser.enter_recursion()?;

        let mut items = Vec::with_capacity(len as usize);
        for _ in 0..len {
            let item = T::parse(parser)?;
            items.push(item);
        }

        parser.exit_recursion();
        Ok(VecM(items))
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Memo<'a> {
    None,
    Text(StringM<'a, 28>),
    Id(u64),
    Hash(Hash<'a>),
    Return(Hash<'a>),
}

impl<'a> XdrParse<'a> for Memo<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let memo_type = MemoType::parse(parser)?;

        match memo_type {
            MemoType::None => Ok(Memo::None),
            MemoType::Text => {
                let text = StringM::<28>::parse(parser)?;
                Ok(Memo::Text(text))
            }
            MemoType::Id => {
                let id = parser.parse_uint64()?;
                Ok(Memo::Id(id))
            }
            MemoType::Hash => {
                let hash = Hash::parse(parser)?;
                Ok(Memo::Hash(hash))
            }
            MemoType::Return => {
                let hash = Hash::parse(parser)?;
                Ok(Memo::Return(hash))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CryptoKeyType {
    KeyTypeEd25519 = 0,
    KeyTypePreAuthTx = 1,
    KeyTypeHashX = 2,
    KeyTypeEd25519SignedPayload = 3,
    KeyTypeMuxedEd25519 = 0x100,
}

impl<'a> XdrParse<'a> for CryptoKeyType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(CryptoKeyType::KeyTypeEd25519),
            1 => Ok(CryptoKeyType::KeyTypePreAuthTx),
            2 => Ok(CryptoKeyType::KeyTypeHashX),
            3 => Ok(CryptoKeyType::KeyTypeEd25519SignedPayload),
            0x100 => Ok(CryptoKeyType::KeyTypeMuxedEd25519),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SignerKeyType {
    SignerKeyTypeEd25519 = 0,
    SignerKeyTypePreAuthTx = 1,
    SignerKeyTypeHashX = 2,
    SignerKeyTypeEd25519SignedPayload = 3,
}

impl<'a> XdrParse<'a> for SignerKeyType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(SignerKeyType::SignerKeyTypeEd25519),
            1 => Ok(SignerKeyType::SignerKeyTypePreAuthTx),
            2 => Ok(SignerKeyType::SignerKeyTypeHashX),
            3 => Ok(SignerKeyType::SignerKeyTypeEd25519SignedPayload),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PublicKeyType {
    PublicKeyTypeEd25519 = 0,
}

impl<'a> XdrParse<'a> for PublicKeyType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;

        match value {
            0 => Ok(PublicKeyType::PublicKeyTypeEd25519),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PublicKey<'a> {
    PublicKeyTypeEd25519(Uint256<'a>),
}

impl<'a> PublicKey<'a> {
    /// Get the underlying uint256 bytes
    pub fn as_bytes(&self) -> &[u8; 32] {
        match self {
            PublicKey::PublicKeyTypeEd25519(key) => key.as_bytes(),
        }
    }

    /// Get the key type
    pub fn key_type(&self) -> PublicKeyType {
        match self {
            PublicKey::PublicKeyTypeEd25519(_) => PublicKeyType::PublicKeyTypeEd25519,
        }
    }
}

pub type AccountId<'a> = PublicKey<'a>;

impl<'a> XdrParse<'a> for PublicKey<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let key_type = PublicKeyType::parse(parser)?;

        #[allow(unreachable_patterns)]
        match key_type {
            PublicKeyType::PublicKeyTypeEd25519 => {
                let uint256 = Uint256::parse(parser)?;
                Ok(PublicKey::PublicKeyTypeEd25519(uint256))
            }
            _ => Err(ParseError::InvalidType(key_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Ed25519SignedPayload<'a> {
    pub ed25519: Uint256<'a>,
    pub payload: &'a [u8],
}

impl<'a> XdrParse<'a> for Ed25519SignedPayload<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let ed25519 = Uint256::parse(parser)?;
        let payload = parser.parse_var_opaque(Some(64))?;
        Ok(Ed25519SignedPayload { ed25519, payload })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SignerKey<'a> {
    SignerKeyTypeEd25519(Uint256<'a>),
    SignerKeyTypePreAuthTx(Uint256<'a>),
    SignerKeyTypeHashX(Uint256<'a>),
    SignerKeyTypeEd25519SignedPayload(Ed25519SignedPayload<'a>),
}

impl<'a> XdrParse<'a> for SignerKey<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let key_type = SignerKeyType::parse(parser)?;

        match key_type {
            SignerKeyType::SignerKeyTypeEd25519 => {
                let uint256 = Uint256::parse(parser)?;
                Ok(SignerKey::SignerKeyTypeEd25519(uint256))
            }
            SignerKeyType::SignerKeyTypePreAuthTx => {
                let uint256 = Uint256::parse(parser)?;
                Ok(SignerKey::SignerKeyTypePreAuthTx(uint256))
            }
            SignerKeyType::SignerKeyTypeHashX => {
                let uint256 = Uint256::parse(parser)?;
                Ok(SignerKey::SignerKeyTypeHashX(uint256))
            }
            SignerKeyType::SignerKeyTypeEd25519SignedPayload => {
                let payload = Ed25519SignedPayload::parse(parser)?;
                Ok(SignerKey::SignerKeyTypeEd25519SignedPayload(payload))
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MuxedEd25519<'a> {
    pub id: u64,
    pub ed25519: Uint256<'a>,
}

impl<'a> XdrParse<'a> for MuxedEd25519<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let id = parser.parse_uint64()?;
        let ed25519 = Uint256::parse(parser)?;
        Ok(MuxedEd25519 { id, ed25519 })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MuxedAccount<'a> {
    KeyTypeEd25519(Uint256<'a>),
    KeyTypeMuxedEd25519(MuxedEd25519<'a>),
}

impl<'a> XdrParse<'a> for MuxedAccount<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let key_type = CryptoKeyType::parse(parser)?;

        match key_type {
            CryptoKeyType::KeyTypeEd25519 => {
                let ed25519 = Uint256::parse(parser)?;
                Ok(MuxedAccount::KeyTypeEd25519(ed25519))
            }
            CryptoKeyType::KeyTypeMuxedEd25519 => {
                let med25519 = MuxedEd25519::parse(parser)?;
                Ok(MuxedAccount::KeyTypeMuxedEd25519(med25519))
            }
            _ => Err(ParseError::InvalidType(key_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AssetCode4<'a>(pub &'a [u8; 4]);

impl<'a> XdrParse<'a> for AssetCode4<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        Ok(AssetCode4(parser.read_array_ref()?))
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AssetCode12<'a>(pub &'a [u8; 12]);

impl<'a> XdrParse<'a> for AssetCode12<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        Ok(AssetCode12(parser.read_array_ref()?))
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AssetType {
    Native = 0,
    CreditAlphanum4 = 1,
    CreditAlphanum12 = 2,
    PoolShare = 3,
}

impl AssetType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(AssetType::Native),
            1 => Ok(AssetType::CreditAlphanum4),
            2 => Ok(AssetType::CreditAlphanum12),
            3 => Ok(AssetType::PoolShare),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for AssetType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        Self::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum AssetCode<'a> {
    AssetCode4(AssetCode4<'a>),
    AssetCode12(AssetCode12<'a>),
}

impl<'a> XdrParse<'a> for AssetCode<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_type = AssetType::parse(parser)?;

        match asset_type {
            AssetType::CreditAlphanum4 => {
                let code = AssetCode4::parse(parser)?;
                Ok(AssetCode::AssetCode4(code))
            }
            AssetType::CreditAlphanum12 => {
                let code = AssetCode12::parse(parser)?;
                Ok(AssetCode::AssetCode12(code))
            }
            _ => Err(ParseError::InvalidType(asset_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AlphaNum4<'a> {
    pub asset_code: AssetCode4<'a>,
    pub issuer: AccountId<'a>,
}

impl<'a> AlphaNum4<'a> {
    /// Get asset code as string (trimming null bytes)
    pub fn code_str(&self) -> &str {
        let bytes = self.asset_code.0;
        let len = bytes.iter().position(|&b| b == 0).unwrap_or(4);
        core::str::from_utf8(&bytes[..len]).unwrap_or("<invalid>")
    }
}

impl<'a> XdrParse<'a> for AlphaNum4<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_code = AssetCode4::parse(parser)?;
        let issuer = AccountId::parse(parser)?;
        Ok(AlphaNum4 { asset_code, issuer })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AlphaNum12<'a> {
    pub asset_code: AssetCode12<'a>,
    pub issuer: AccountId<'a>,
}

impl<'a> AlphaNum12<'a> {
    /// Get asset code as string (trimming null bytes)
    pub fn code_str(&self) -> &str {
        let bytes = self.asset_code.0;
        let len = bytes.iter().position(|&b| b == 0).unwrap_or(12);
        core::str::from_utf8(&bytes[..len]).unwrap_or("<invalid>")
    }
}

impl<'a> XdrParse<'a> for AlphaNum12<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_code = AssetCode12::parse(parser)?;
        let issuer = AccountId::parse(parser)?;
        Ok(AlphaNum12 { asset_code, issuer })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Asset<'a> {
    Native,
    CreditAlphanum4(AlphaNum4<'a>),
    CreditAlphanum12(AlphaNum12<'a>),
}

impl<'a> Asset<'a> {
    /// Check if this is native XLM
    pub fn is_native(&self) -> bool {
        matches!(self, Asset::Native)
    }

    /// Get asset code as string
    pub fn code(&self) -> &str {
        match self {
            Asset::Native => "XLM",
            Asset::CreditAlphanum4(a) => a.code_str(),
            Asset::CreditAlphanum12(a) => a.code_str(),
        }
    }
}

impl<'a> XdrParse<'a> for Asset<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_type = AssetType::parse(parser)?;

        match asset_type {
            AssetType::Native => Ok(Asset::Native),
            AssetType::CreditAlphanum4 => {
                let alpha_num4 = AlphaNum4::parse(parser)?;
                Ok(Asset::CreditAlphanum4(alpha_num4))
            }
            AssetType::CreditAlphanum12 => {
                let alpha_num12 = AlphaNum12::parse(parser)?;
                Ok(Asset::CreditAlphanum12(alpha_num12))
            }
            _ => Err(ParseError::InvalidType(asset_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TimeBounds {
    pub min_time: TimePoint,
    pub max_time: TimePoint,
}

impl<'a> XdrParse<'a> for TimeBounds {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let min_time = parser.parse_uint64()?;
        let max_time = parser.parse_uint64()?;
        Ok(TimeBounds { min_time, max_time })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerBounds {
    pub min_ledger: u32,
    pub max_ledger: u32,
}

impl<'a> XdrParse<'a> for LedgerBounds {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let min_ledger = parser.parse_uint32()?;
        let max_ledger = parser.parse_uint32()?;
        Ok(LedgerBounds {
            min_ledger,
            max_ledger,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PreconditionsV2<'a> {
    pub time_bounds: Option<TimeBounds>,
    pub ledger_bounds: Option<LedgerBounds>,
    pub min_seq_num: Option<SequenceNumber>,
    pub min_seq_age: Duration,
    pub min_seq_ledger_gap: u32,
    pub extra_signers: VecM<SignerKey<'a>, 2>,
}

impl<'a> XdrParse<'a> for PreconditionsV2<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let time_bounds = parser.parse_optional(TimeBounds::parse)?;
        let ledger_bounds = parser.parse_optional(LedgerBounds::parse)?;
        let min_seq_num = parser.parse_optional(|p| p.parse_int64())?;
        let min_seq_age = parser.parse_uint64()?;
        let min_seq_ledger_gap = parser.parse_uint32()?;
        let extra_signers = VecM::parse(parser)?;

        Ok(PreconditionsV2 {
            time_bounds,
            ledger_bounds,
            min_seq_num,
            min_seq_age,
            min_seq_ledger_gap,
            extra_signers,
        })
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PreconditionType {
    None = 0,
    Time = 1,
    V2 = 2,
}

impl<'a> XdrParse<'a> for PreconditionType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(PreconditionType::None),
            1 => Ok(PreconditionType::Time),
            2 => Ok(PreconditionType::V2),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Preconditions<'a> {
    None,
    Time(TimeBounds),
    V2(PreconditionsV2<'a>),
}

impl<'a> XdrParse<'a> for Preconditions<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let precond_type = PreconditionType::parse(parser)?;

        match precond_type {
            PreconditionType::None => Ok(Preconditions::None),
            PreconditionType::Time => {
                let time_bounds = TimeBounds::parse(parser)?;
                Ok(Preconditions::Time(time_bounds))
            }
            PreconditionType::V2 => {
                let v2 = PreconditionsV2::parse(parser)?;
                Ok(Preconditions::V2(v2))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EnvelopeType {
    EnvelopeTypeTxV0 = 0,
    EnvelopeTypeScp = 1,
    EnvelopeTypeTx = 2,
    EnvelopeTypeAuth = 3,
    EnvelopeTypeScpvalue = 4,
    EnvelopeTypeTxFeeBump = 5,
    EnvelopeTypeOpId = 6,
    EnvelopeTypePoolRevokeOpId = 7,
    EnvelopeTypeContractId = 8,
    EnvelopeTypeSorobanAuthorization = 9,
}

impl<'a> XdrParse<'a> for EnvelopeType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(EnvelopeType::EnvelopeTypeTxV0),
            1 => Ok(EnvelopeType::EnvelopeTypeScp),
            2 => Ok(EnvelopeType::EnvelopeTypeTx),
            3 => Ok(EnvelopeType::EnvelopeTypeAuth),
            4 => Ok(EnvelopeType::EnvelopeTypeScpvalue),
            5 => Ok(EnvelopeType::EnvelopeTypeTxFeeBump),
            6 => Ok(EnvelopeType::EnvelopeTypeOpId),
            7 => Ok(EnvelopeType::EnvelopeTypePoolRevokeOpId),
            8 => Ok(EnvelopeType::EnvelopeTypeContractId),
            9 => Ok(EnvelopeType::EnvelopeTypeSorobanAuthorization),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Transaction<'a> {
    pub source_account: MuxedAccount<'a>,
    pub fee: u32,
    pub seq_num: SequenceNumber,
    pub cond: Preconditions<'a>,
    pub memo: Memo<'a>,
    pub op_count: u32,
}

impl<'a> XdrParse<'a> for Transaction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let source_account = MuxedAccount::parse(parser)?;
        let fee = parser.parse_uint32()?;
        let seq_num = parser.parse_int64()?;
        let cond = Preconditions::parse(parser)?;
        let memo = Memo::parse(parser)?;
        let op_count = parser.parse_uint32()?;

        Ok(Transaction {
            source_account,
            fee,
            seq_num,
            cond,
            memo,
            op_count,
        })
    }
}

/// InnerTransaction is the inner transaction for FeeBumpTransaction
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum InnerTransaction<'a> {
    EnvelopeTypeTx(Transaction<'a>),
}

impl<'a> XdrParse<'a> for InnerTransaction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let envelope_type = EnvelopeType::parse(parser)?;

        match envelope_type {
            EnvelopeType::EnvelopeTypeTx => {
                let tx = Transaction::parse(parser)?;
                Ok(InnerTransaction::EnvelopeTypeTx(tx))
            }
            _ => Err(ParseError::InvalidType(envelope_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FeeBumpTransaction<'a> {
    pub fee_source: MuxedAccount<'a>,
    pub fee: i64,
    pub inner_tx: InnerTransaction<'a>,
}

impl<'a> XdrParse<'a> for FeeBumpTransaction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let fee_source = MuxedAccount::parse(parser)?;
        let fee = parser.parse_int64()?;
        let inner_tx = InnerTransaction::parse(parser)?;

        Ok(FeeBumpTransaction {
            fee_source,
            fee,
            inner_tx,
        })
    }
}

/// TaggedTransaction is the union in TransactionSignaturePayload
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TaggedTransaction<'a> {
    EnvelopeTypeTx(Transaction<'a>),
    EnvelopeTypeTxFeeBump(FeeBumpTransaction<'a>),
}

impl<'a> XdrParse<'a> for TaggedTransaction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let envelope_type = EnvelopeType::parse(parser)?;

        match envelope_type {
            EnvelopeType::EnvelopeTypeTx => {
                let tx = Transaction::parse(parser)?;
                Ok(TaggedTransaction::EnvelopeTypeTx(tx))
            }
            EnvelopeType::EnvelopeTypeTxFeeBump => {
                let fee_bump = FeeBumpTransaction::parse(parser)?;
                Ok(TaggedTransaction::EnvelopeTypeTxFeeBump(fee_bump))
            }
            _ => Err(ParseError::InvalidType(envelope_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TransactionSignaturePayload<'a> {
    pub network_id: Hash<'a>,
    pub tagged_transaction: TaggedTransaction<'a>,
}

impl<'a> XdrParse<'a> for TransactionSignaturePayload<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let network_id = Hash::parse(parser)?;
        let tagged_transaction = TaggedTransaction::parse(parser)?;

        Ok(TransactionSignaturePayload {
            network_id,
            tagged_transaction,
        })
    }
}

pub type ContractId<'a> = Hash<'a>;
pub type PoolId<'a> = Hash<'a>;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct UInt128Parts {
    pub hi: u64,
    pub lo: u64,
}

impl<'a> XdrParse<'a> for UInt128Parts {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let hi = parser.parse_uint64()?;
        let lo = parser.parse_uint64()?;
        Ok(UInt128Parts { hi, lo })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Int128Parts {
    pub hi: i64,
    pub lo: u64,
}

impl<'a> XdrParse<'a> for Int128Parts {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let hi = parser.parse_int64()?;
        let lo = parser.parse_uint64()?;
        Ok(Int128Parts { hi, lo })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct UInt256Parts {
    pub hi_hi: u64,
    pub hi_lo: u64,
    pub lo_hi: u64,
    pub lo_lo: u64,
}

impl<'a> XdrParse<'a> for UInt256Parts {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let hi_hi = parser.parse_uint64()?;
        let hi_lo = parser.parse_uint64()?;
        let lo_hi = parser.parse_uint64()?;
        let lo_lo = parser.parse_uint64()?;
        Ok(UInt256Parts {
            hi_hi,
            hi_lo,
            lo_hi,
            lo_lo,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Int256Parts {
    pub hi_hi: i64,
    pub hi_lo: u64,
    pub lo_hi: u64,
    pub lo_lo: u64,
}

impl<'a> XdrParse<'a> for Int256Parts {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let hi_hi = parser.parse_int64()?;
        let hi_lo = parser.parse_uint64()?;
        let lo_hi = parser.parse_uint64()?;
        let lo_lo = parser.parse_uint64()?;
        Ok(Int256Parts {
            hi_hi,
            hi_lo,
            lo_hi,
            lo_lo,
        })
    }
}

/// SCSymbol is an XDR Typedef with max 32 chars
pub type ScSymbol<'a> = StringM<'a, 32>;

/// SCString is an unbounded string (using StringM with max u32::MAX)
pub type ScString<'a> = StringM<'a, { u32::MAX as usize }>;

/// SCBytes is an unbounded byte array
pub type ScBytes<'a> = BytesM<'a, { u32::MAX as usize }>;

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ClaimableBalanceIdType {
    ClaimableBalanceIdTypeV0 = 0,
}

impl<'a> XdrParse<'a> for ClaimableBalanceIdType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(ClaimableBalanceIdType::ClaimableBalanceIdTypeV0),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ClaimableBalanceId<'a> {
    ClaimableBalanceIdTypeV0(Hash<'a>),
}

impl<'a> XdrParse<'a> for ClaimableBalanceId<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let id_type = ClaimableBalanceIdType::parse(parser)?;

        match id_type {
            ClaimableBalanceIdType::ClaimableBalanceIdTypeV0 => {
                let hash = Hash::parse(parser)?;
                Ok(ClaimableBalanceId::ClaimableBalanceIdTypeV0(hash))
            }
        }
    }
}

/// MuxedEd25519Account (different from MuxedEd25519 in Soroban context)
///
/// Note: This reuses the existing MuxedEd25519 type
pub type MuxedEd25519Account<'a> = MuxedEd25519<'a>;

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScAddressType {
    ScAddressTypeAccount = 0,
    ScAddressTypeContract = 1,
    ScAddressTypeMuxedAccount = 2,
    ScAddressTypeClaimableBalance = 3,
    ScAddressTypeLiquidityPool = 4,
}

impl<'a> XdrParse<'a> for ScAddressType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(ScAddressType::ScAddressTypeAccount),
            1 => Ok(ScAddressType::ScAddressTypeContract),
            2 => Ok(ScAddressType::ScAddressTypeMuxedAccount),
            3 => Ok(ScAddressType::ScAddressTypeClaimableBalance),
            4 => Ok(ScAddressType::ScAddressTypeLiquidityPool),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ScAddress<'a> {
    ScAddressTypeAccount(AccountId<'a>),
    ScAddressTypeContract(ContractId<'a>),
    ScAddressTypeMuxedAccount(MuxedEd25519Account<'a>),
    ScAddressTypeClaimableBalance(ClaimableBalanceId<'a>),
    ScAddressTypeLiquidityPool(PoolId<'a>),
}

impl<'a> XdrParse<'a> for ScAddress<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let addr_type = ScAddressType::parse(parser)?;

        match addr_type {
            ScAddressType::ScAddressTypeAccount => {
                let account = AccountId::parse(parser)?;
                Ok(ScAddress::ScAddressTypeAccount(account))
            }
            ScAddressType::ScAddressTypeContract => {
                let contract = ContractId::parse(parser)?;
                Ok(ScAddress::ScAddressTypeContract(contract))
            }
            ScAddressType::ScAddressTypeMuxedAccount => {
                let muxed = MuxedEd25519Account::parse(parser)?;
                Ok(ScAddress::ScAddressTypeMuxedAccount(muxed))
            }
            ScAddressType::ScAddressTypeClaimableBalance => {
                let claimable = ClaimableBalanceId::parse(parser)?;
                Ok(ScAddress::ScAddressTypeClaimableBalance(claimable))
            }
            ScAddressType::ScAddressTypeLiquidityPool => {
                let pool = PoolId::parse(parser)?;
                Ok(ScAddress::ScAddressTypeLiquidityPool(pool))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScValType {
    ScvBool = 0,
    ScvVoid = 1,
    ScvError = 2,
    ScvU32 = 3,
    ScvI32 = 4,
    ScvU64 = 5,
    ScvI64 = 6,
    ScvTimepoint = 7,
    ScvDuration = 8,
    ScvU128 = 9,
    ScvI128 = 10,
    ScvU256 = 11,
    ScvI256 = 12,
    ScvBytes = 13,
    ScvString = 14,
    ScvSymbol = 15,
    ScvVec = 16,
    ScvMap = 17,
    ScvAddress = 18,
    ScvContractInstance = 19,
    ScvLedgerKeyContractInstance = 20,
    ScvLedgerKeyNonce = 21,
}

impl<'a> XdrParse<'a> for ScValType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(ScValType::ScvBool),
            1 => Ok(ScValType::ScvVoid),
            2 => Ok(ScValType::ScvError),
            3 => Ok(ScValType::ScvU32),
            4 => Ok(ScValType::ScvI32),
            5 => Ok(ScValType::ScvU64),
            6 => Ok(ScValType::ScvI64),
            7 => Ok(ScValType::ScvTimepoint),
            8 => Ok(ScValType::ScvDuration),
            9 => Ok(ScValType::ScvU128),
            10 => Ok(ScValType::ScvI128),
            11 => Ok(ScValType::ScvU256),
            12 => Ok(ScValType::ScvI256),
            13 => Ok(ScValType::ScvBytes),
            14 => Ok(ScValType::ScvString),
            15 => Ok(ScValType::ScvSymbol),
            16 => Ok(ScValType::ScvVec),
            17 => Ok(ScValType::ScvMap),
            18 => Ok(ScValType::ScvAddress),
            19 => Ok(ScValType::ScvContractInstance),
            20 => Ok(ScValType::ScvLedgerKeyContractInstance),
            21 => Ok(ScValType::ScvLedgerKeyNonce),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ScMapEntry<'a> {
    pub key: ScVal<'a>,
    pub val: ScVal<'a>,
}

impl<'a> XdrParse<'a> for ScMapEntry<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let key = ScVal::parse(parser)?;
        let val = ScVal::parse(parser)?;
        Ok(ScMapEntry { key, val })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ScNonceKey {
    pub nonce: i64,
}

impl<'a> XdrParse<'a> for ScNonceKey {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let nonce = parser.parse_int64()?;
        Ok(ScNonceKey { nonce })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SCContractInstance<'a> {
    pub executable: ContractExecutable<'a>,
    pub storage: Option<VecM<ScMapEntry<'a>>>,
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SCErrorType {
    SceContract = 0,
    SceWasmVm = 1,
    SceContext = 2,
    SceStorage = 3,
    SceObject = 4,
    SceCrypto = 5,
    SceEvents = 6,
    SceBudget = 7,
    SceValue = 8,
    SceAuth = 9,
}

impl<'a> XdrParse<'a> for SCErrorType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(SCErrorType::SceContract),
            1 => Ok(SCErrorType::SceWasmVm),
            2 => Ok(SCErrorType::SceContext),
            3 => Ok(SCErrorType::SceStorage),
            4 => Ok(SCErrorType::SceObject),
            5 => Ok(SCErrorType::SceCrypto),
            6 => Ok(SCErrorType::SceEvents),
            7 => Ok(SCErrorType::SceBudget),
            8 => Ok(SCErrorType::SceValue),
            9 => Ok(SCErrorType::SceAuth),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SCErrorCode {
    ScecArithDomain = 0,
    ScecIndexBounds = 1,
    ScecInvalidInput = 2,
    ScecMissingValue = 3,
    ScecExistingValue = 4,
    ScecExceededLimit = 5,
    ScecInvalidAction = 6,
    ScecInternalError = 7,
    ScecUnexpectedType = 8,
    ScecUnexpectedSize = 9,
}

impl<'a> XdrParse<'a> for SCErrorCode {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        match value {
            0 => Ok(SCErrorCode::ScecArithDomain),
            1 => Ok(SCErrorCode::ScecIndexBounds),
            2 => Ok(SCErrorCode::ScecInvalidInput),
            3 => Ok(SCErrorCode::ScecMissingValue),
            4 => Ok(SCErrorCode::ScecExistingValue),
            5 => Ok(SCErrorCode::ScecExceededLimit),
            6 => Ok(SCErrorCode::ScecInvalidAction),
            7 => Ok(SCErrorCode::ScecInternalError),
            8 => Ok(SCErrorCode::ScecUnexpectedType),
            9 => Ok(SCErrorCode::ScecUnexpectedSize),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SCError {
    SceContract(u32),
    SceWasmVm(SCErrorCode),
    SceContext(SCErrorCode),
    SceStorage(SCErrorCode),
    SceObject(SCErrorCode),
    SceCrypto(SCErrorCode),
    SceEvents(SCErrorCode),
    SceBudget(SCErrorCode),
    SceValue(SCErrorCode),
    SceAuth(SCErrorCode),
}

impl<'a> XdrParse<'a> for SCError {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let error_type = SCErrorType::parse(parser)?;

        match error_type {
            SCErrorType::SceContract => {
                let contract_code = parser.parse_uint32()?;
                Ok(SCError::SceContract(contract_code))
            }
            SCErrorType::SceWasmVm => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceWasmVm(code))
            }
            SCErrorType::SceContext => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceContext(code))
            }
            SCErrorType::SceStorage => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceStorage(code))
            }
            SCErrorType::SceObject => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceObject(code))
            }
            SCErrorType::SceCrypto => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceCrypto(code))
            }
            SCErrorType::SceEvents => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceEvents(code))
            }
            SCErrorType::SceBudget => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceBudget(code))
            }
            SCErrorType::SceValue => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceValue(code))
            }
            SCErrorType::SceAuth => {
                let code = SCErrorCode::parse(parser)?;
                Ok(SCError::SceAuth(code))
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ScVal<'a> {
    Bool(bool),
    Void,
    Error(SCError),
    U32(u32),
    I32(i32),
    U64(u64),
    I64(i64),
    Timepoint(TimePoint),
    Duration(Duration),
    U128(UInt128Parts),
    I128(Int128Parts),
    U256(UInt256Parts),
    I256(Int256Parts),
    Bytes(ScBytes<'a>),
    String(ScString<'a>),
    Symbol(ScSymbol<'a>),
    Vec(Option<VecM<ScVal<'a>>>),
    Map(Option<VecM<ScMapEntry<'a>>>),
    Address(ScAddress<'a>),
    ContractInstance(SCContractInstance<'a>),
    LedgerKeyContractInstance,
    LedgerKeyNonce(ScNonceKey),
}

impl<'a> XdrParse<'a> for ScVal<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let val_type = ScValType::parse(parser)?;

        match val_type {
            ScValType::ScvBool => {
                let b = parser.parse_bool()?;
                Ok(ScVal::Bool(b))
            }
            ScValType::ScvVoid => Ok(ScVal::Void),
            ScValType::ScvError => {
                let error = SCError::parse(parser)?;
                Ok(ScVal::Error(error))
            }
            ScValType::ScvU32 => {
                let u = parser.parse_uint32()?;
                Ok(ScVal::U32(u))
            }
            ScValType::ScvI32 => {
                let i = parser.parse_int32()?;
                Ok(ScVal::I32(i))
            }
            ScValType::ScvU64 => {
                let u = parser.parse_uint64()?;
                Ok(ScVal::U64(u))
            }
            ScValType::ScvI64 => {
                let i = parser.parse_int64()?;
                Ok(ScVal::I64(i))
            }
            ScValType::ScvTimepoint => {
                let tp = parser.parse_uint64()?;
                Ok(ScVal::Timepoint(tp))
            }
            ScValType::ScvDuration => {
                let dur = parser.parse_uint64()?;
                Ok(ScVal::Duration(dur))
            }
            ScValType::ScvU128 => {
                let u128 = UInt128Parts::parse(parser)?;
                Ok(ScVal::U128(u128))
            }
            ScValType::ScvI128 => {
                let i128 = Int128Parts::parse(parser)?;
                Ok(ScVal::I128(i128))
            }
            ScValType::ScvU256 => {
                let u256 = UInt256Parts::parse(parser)?;
                Ok(ScVal::U256(u256))
            }
            ScValType::ScvI256 => {
                let i256 = Int256Parts::parse(parser)?;
                Ok(ScVal::I256(i256))
            }
            ScValType::ScvBytes => {
                let bytes = ScBytes::parse(parser)?;
                Ok(ScVal::Bytes(bytes))
            }
            ScValType::ScvString => {
                let string = ScString::parse(parser)?;
                Ok(ScVal::String(string))
            }
            ScValType::ScvSymbol => {
                let sym = ScSymbol::parse(parser)?;
                Ok(ScVal::Symbol(sym))
            }
            ScValType::ScvVec => {
                // Parse optional pointer: if present, parse array of SCVal
                let vec_data = parser.parse_optional(VecM::parse)?;
                Ok(ScVal::Vec(vec_data))
            }
            ScValType::ScvMap => {
                // Parse optional pointer: if present, parse array of SCMapEntry
                let map_data = parser.parse_optional(VecM::parse)?;
                Ok(ScVal::Map(map_data))
            }
            ScValType::ScvAddress => {
                let addr = ScAddress::parse(parser)?;
                Ok(ScVal::Address(addr))
            }
            ScValType::ScvContractInstance => {
                let executable = ContractExecutable::parse(parser)?;
                let storage = parser.parse_optional(VecM::parse)?;
                Ok(ScVal::ContractInstance(SCContractInstance {
                    executable,
                    storage,
                }))
            }
            ScValType::ScvLedgerKeyContractInstance => Ok(ScVal::LedgerKeyContractInstance),
            ScValType::ScvLedgerKeyNonce => {
                let nonce_key = ScNonceKey::parse(parser)?;
                Ok(ScVal::LedgerKeyNonce(nonce_key))
            }
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum OperationType {
    CreateAccount = 0,
    Payment = 1,
    PathPaymentStrictReceive = 2,
    ManageSellOffer = 3,
    CreatePassiveSellOffer = 4,
    SetOptions = 5,
    ChangeTrust = 6,
    AllowTrust = 7,
    AccountMerge = 8,
    Inflation = 9,
    ManageData = 10,
    BumpSequence = 11,
    ManageBuyOffer = 12,
    PathPaymentStrictSend = 13,
    CreateClaimableBalance = 14,
    ClaimClaimableBalance = 15,
    BeginSponsoringFutureReserves = 16,
    EndSponsoringFutureReserves = 17,
    RevokeSponsorship = 18,
    Clawback = 19,
    ClawbackClaimableBalance = 20,
    SetTrustLineFlags = 21,
    LiquidityPoolDeposit = 22,
    LiquidityPoolWithdraw = 23,
    InvokeHostFunction = 24,
    ExtendFootprintTtl = 25,
    RestoreFootprint = 26,
}

impl OperationType {
    fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(OperationType::CreateAccount),
            1 => Ok(OperationType::Payment),
            2 => Ok(OperationType::PathPaymentStrictReceive),
            3 => Ok(OperationType::ManageSellOffer),
            4 => Ok(OperationType::CreatePassiveSellOffer),
            5 => Ok(OperationType::SetOptions),
            6 => Ok(OperationType::ChangeTrust),
            7 => Ok(OperationType::AllowTrust),
            8 => Ok(OperationType::AccountMerge),
            9 => Ok(OperationType::Inflation),
            10 => Ok(OperationType::ManageData),
            11 => Ok(OperationType::BumpSequence),
            12 => Ok(OperationType::ManageBuyOffer),
            13 => Ok(OperationType::PathPaymentStrictSend),
            14 => Ok(OperationType::CreateClaimableBalance),
            15 => Ok(OperationType::ClaimClaimableBalance),
            16 => Ok(OperationType::BeginSponsoringFutureReserves),
            17 => Ok(OperationType::EndSponsoringFutureReserves),
            18 => Ok(OperationType::RevokeSponsorship),
            19 => Ok(OperationType::Clawback),
            20 => Ok(OperationType::ClawbackClaimableBalance),
            21 => Ok(OperationType::SetTrustLineFlags),
            22 => Ok(OperationType::LiquidityPoolDeposit),
            23 => Ok(OperationType::LiquidityPoolWithdraw),
            24 => Ok(OperationType::InvokeHostFunction),
            25 => Ok(OperationType::ExtendFootprintTtl),
            26 => Ok(OperationType::RestoreFootprint),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CreateAccountOp<'a> {
    pub destination: AccountId<'a>,
    pub starting_balance: i64,
}

impl<'a> XdrParse<'a> for CreateAccountOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let destination = AccountId::parse(parser)?;
        let starting_balance = parser.parse_int64()?;
        Ok(CreateAccountOp {
            destination,
            starting_balance,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PaymentOp<'a> {
    pub destination: MuxedAccount<'a>,
    pub asset: Asset<'a>,
    pub amount: i64,
}

impl<'a> XdrParse<'a> for PaymentOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let destination = MuxedAccount::parse(parser)?;
        let asset = Asset::parse(parser)?;
        let amount = parser.parse_int64()?;
        Ok(PaymentOp {
            destination,
            asset,
            amount,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Price {
    pub n: i32,
    pub d: i32,
}

impl<'a> XdrParse<'a> for Price {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let n = parser.parse_int32()?;
        let d = parser.parse_int32()?;
        Ok(Price { n, d })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PathPaymentStrictReceiveOp<'a> {
    pub send_asset: Asset<'a>,
    pub send_max: i64,
    pub destination: MuxedAccount<'a>,
    pub dest_asset: Asset<'a>,
    pub dest_amount: i64,
    // skip path, we don't need them for our use case
}

impl<'a> XdrParse<'a> for PathPaymentStrictReceiveOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let send_asset = Asset::parse(parser)?;
        let send_max = parser.parse_int64()?;
        let destination = MuxedAccount::parse(parser)?;
        let dest_asset = Asset::parse(parser)?;
        let dest_amount = parser.parse_int64()?;

        let array_len = parser.parse_uint32()? as usize;
        if array_len > 5 {
            return Err(ParseError::LengthExceedsMax {
                actual: array_len,
                max: 5,
            });
        }
        for _ in 0..array_len {
            Asset::parse(parser)?;
        }

        Ok(PathPaymentStrictReceiveOp {
            send_asset,
            send_max,
            destination,
            dest_asset,
            dest_amount,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PathPaymentStrictSendOp<'a> {
    pub send_asset: Asset<'a>,
    pub send_amount: i64,
    pub destination: MuxedAccount<'a>,
    pub dest_asset: Asset<'a>,
    pub dest_min: i64,
    // skip path, we don't need them for our use case
}

impl<'a> XdrParse<'a> for PathPaymentStrictSendOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let send_asset = Asset::parse(parser)?;
        let send_amount = parser.parse_int64()?;
        let destination = MuxedAccount::parse(parser)?;
        let dest_asset = Asset::parse(parser)?;
        let dest_min = parser.parse_int64()?;

        let array_len = parser.parse_uint32()? as usize;
        if array_len > 5 {
            return Err(ParseError::LengthExceedsMax {
                actual: array_len,
                max: 5,
            });
        }
        for _ in 0..array_len {
            Asset::parse(parser)?;
        }

        Ok(PathPaymentStrictSendOp {
            send_asset,
            send_amount,
            destination,
            dest_asset,
            dest_min,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManageSellOfferOp<'a> {
    pub selling: Asset<'a>,
    pub buying: Asset<'a>,
    pub amount: i64,
    pub price: Price,
    pub offer_id: i64,
}

impl<'a> XdrParse<'a> for ManageSellOfferOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let selling = Asset::parse(parser)?;
        let buying = Asset::parse(parser)?;
        let amount = parser.parse_int64()?;
        let price = Price::parse(parser)?;
        let offer_id = parser.parse_int64()?;
        Ok(ManageSellOfferOp {
            selling,
            buying,
            amount,
            price,
            offer_id,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManageBuyOfferOp<'a> {
    pub selling: Asset<'a>,
    pub buying: Asset<'a>,
    pub buy_amount: i64,
    pub price: Price,
    pub offer_id: i64,
}

impl<'a> XdrParse<'a> for ManageBuyOfferOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let selling = Asset::parse(parser)?;
        let buying = Asset::parse(parser)?;
        let buy_amount = parser.parse_int64()?;
        let price = Price::parse(parser)?;
        let offer_id = parser.parse_int64()?;
        Ok(ManageBuyOfferOp {
            selling,
            buying,
            buy_amount,
            price,
            offer_id,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CreatePassiveSellOfferOp<'a> {
    pub selling: Asset<'a>,
    pub buying: Asset<'a>,
    pub amount: i64,
    pub price: Price,
}

impl<'a> XdrParse<'a> for CreatePassiveSellOfferOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let selling = Asset::parse(parser)?;
        let buying = Asset::parse(parser)?;
        let amount = parser.parse_int64()?;
        let price = Price::parse(parser)?;
        Ok(CreatePassiveSellOfferOp {
            selling,
            buying,
            amount,
            price,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Signer<'a> {
    pub key: SignerKey<'a>,
    pub weight: u32,
}

impl<'a> XdrParse<'a> for Signer<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let key = SignerKey::parse(parser)?;
        let weight = parser.parse_uint32()?;
        Ok(Signer { key, weight })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SetOptionsOp<'a> {
    pub inflation_dest: Option<AccountId<'a>>,
    pub clear_flags: Option<u32>,
    pub set_flags: Option<u32>,
    pub master_weight: Option<u32>,
    pub low_threshold: Option<u32>,
    pub med_threshold: Option<u32>,
    pub high_threshold: Option<u32>,
    pub home_domain: Option<StringM<'a, 32>>,
    pub signer: Option<Signer<'a>>,
}

impl<'a> XdrParse<'a> for SetOptionsOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let inflation_dest = parser.parse_optional(AccountId::parse)?;
        let clear_flags = parser.parse_optional(|p| p.parse_uint32())?;
        let set_flags = parser.parse_optional(|p| p.parse_uint32())?;
        let master_weight = parser.parse_optional(|p| p.parse_uint32())?;
        let low_threshold = parser.parse_optional(|p| p.parse_uint32())?;
        let med_threshold = parser.parse_optional(|p| p.parse_uint32())?;
        let high_threshold = parser.parse_optional(|p| p.parse_uint32())?;
        let home_domain = parser.parse_optional(StringM::parse)?;
        let signer = parser.parse_optional(Signer::parse)?;
        Ok(SetOptionsOp {
            inflation_dest,
            clear_flags,
            set_flags,
            master_weight,
            low_threshold,
            med_threshold,
            high_threshold,
            home_domain,
            signer,
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum LiquidityPoolType {
    ConstantProduct = 0,
}

impl LiquidityPoolType {
    fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(LiquidityPoolType::ConstantProduct),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LiquidityPoolConstantProductParameters<'a> {
    pub asset_a: Asset<'a>,
    pub asset_b: Asset<'a>,
    pub fee: i32,
}

impl<'a> XdrParse<'a> for LiquidityPoolConstantProductParameters<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_a = Asset::parse(parser)?;
        let asset_b = Asset::parse(parser)?;
        let fee = parser.parse_int32()?;
        Ok(LiquidityPoolConstantProductParameters {
            asset_a,
            asset_b,
            fee,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LiquidityPoolParameters<'a> {
    ConstantProduct(LiquidityPoolConstantProductParameters<'a>),
}

impl<'a> XdrParse<'a> for LiquidityPoolParameters<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let pool_type = LiquidityPoolType::from_i32(parser.parse_int32()?)?;
        match pool_type {
            LiquidityPoolType::ConstantProduct => Ok(LiquidityPoolParameters::ConstantProduct(
                LiquidityPoolConstantProductParameters::parse(parser)?,
            )),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ChangeTrustAsset<'a> {
    Native,
    CreditAlphanum4(AlphaNum4<'a>),
    CreditAlphanum12(AlphaNum12<'a>),
    PoolShare(LiquidityPoolParameters<'a>),
}

impl<'a> XdrParse<'a> for ChangeTrustAsset<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_type = AssetType::from_i32(parser.parse_int32()?)?;
        match asset_type {
            AssetType::Native => Ok(ChangeTrustAsset::Native),
            AssetType::CreditAlphanum4 => {
                Ok(ChangeTrustAsset::CreditAlphanum4(AlphaNum4::parse(parser)?))
            }
            AssetType::CreditAlphanum12 => Ok(ChangeTrustAsset::CreditAlphanum12(
                AlphaNum12::parse(parser)?,
            )),
            AssetType::PoolShare => Ok(ChangeTrustAsset::PoolShare(
                LiquidityPoolParameters::parse(parser)?,
            )),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ChangeTrustOp<'a> {
    pub line: ChangeTrustAsset<'a>,
    pub limit: i64,
}

impl<'a> XdrParse<'a> for ChangeTrustOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let line = ChangeTrustAsset::parse(parser)?;
        let limit = parser.parse_int64()?;
        Ok(ChangeTrustOp { line, limit })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AllowTrustOp<'a> {
    pub trustor: AccountId<'a>,
    pub asset: AssetCode<'a>,
    pub authorize: u32,
}

impl<'a> XdrParse<'a> for AllowTrustOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let trustor = AccountId::parse(parser)?;
        let asset = AssetCode::parse(parser)?;
        let authorize = parser.parse_uint32()?;
        Ok(AllowTrustOp {
            trustor,
            asset,
            authorize,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManageDataOp<'a> {
    pub data_name: StringM<'a, 64>,
    pub data_value: Option<BytesM<'a, 64>>,
}

impl<'a> XdrParse<'a> for ManageDataOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let data_name = StringM::parse(parser)?;
        let data_value = parser.parse_optional(BytesM::parse)?;
        Ok(ManageDataOp {
            data_name,
            data_value,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BumpSequenceOp {
    pub bump_to: SequenceNumber,
}

impl<'a> XdrParse<'a> for BumpSequenceOp {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let bump_to = parser.parse_int64()?;
        Ok(BumpSequenceOp { bump_to })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum ClaimPredicateType {
    Unconditional = 0,
    And = 1,
    Or = 2,
    Not = 3,
    BeforeAbsoluteTime = 4,
    BeforeRelativeTime = 5,
}

impl ClaimPredicateType {
    fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ClaimPredicateType::Unconditional),
            1 => Ok(ClaimPredicateType::And),
            2 => Ok(ClaimPredicateType::Or),
            3 => Ok(ClaimPredicateType::Not),
            4 => Ok(ClaimPredicateType::BeforeAbsoluteTime),
            5 => Ok(ClaimPredicateType::BeforeRelativeTime),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ClaimPredicate {
    Unconditional,
    And(VecM<ClaimPredicate, 2>),
    Or(VecM<ClaimPredicate, 2>),
    Not(Box<ClaimPredicate>),
    BeforeAbsoluteTime(i64),
    BeforeRelativeTime(i64),
}

impl<'a> XdrParse<'a> for ClaimPredicate {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        // Enter recursion for ClaimPredicate parsing
        parser.enter_recursion()?;

        let pred_type = ClaimPredicateType::from_i32(parser.parse_int32()?)?;
        let result = match pred_type {
            ClaimPredicateType::Unconditional => Ok(ClaimPredicate::Unconditional),
            ClaimPredicateType::And => {
                let predicates = VecM::parse(parser)?;
                Ok(ClaimPredicate::And(predicates))
            }
            ClaimPredicateType::Or => {
                let predicates = VecM::parse(parser)?;
                Ok(ClaimPredicate::Or(predicates))
            }
            ClaimPredicateType::Not => {
                let predicate = parser.parse_optional(ClaimPredicate::parse)?;
                match predicate {
                    Some(p) => Ok(ClaimPredicate::Not(Box::new(p))),
                    None => Err(ParseError::BufferOverflow),
                }
            }
            ClaimPredicateType::BeforeAbsoluteTime => {
                let abs_before = parser.parse_int64()?;
                Ok(ClaimPredicate::BeforeAbsoluteTime(abs_before))
            }
            ClaimPredicateType::BeforeRelativeTime => {
                let rel_before = parser.parse_int64()?;
                Ok(ClaimPredicate::BeforeRelativeTime(rel_before))
            }
        };

        parser.exit_recursion();
        result
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum ClaimantType {
    V0 = 0,
}

impl ClaimantType {
    fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ClaimantType::V0),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClaimantV0<'a> {
    pub destination: AccountId<'a>,
    pub predicate: ClaimPredicate,
}

impl<'a> XdrParse<'a> for ClaimantV0<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let destination = AccountId::parse(parser)?;
        let predicate = ClaimPredicate::parse(parser)?;
        Ok(ClaimantV0 {
            destination,
            predicate,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Claimant<'a> {
    V0(ClaimantV0<'a>),
}

impl<'a> XdrParse<'a> for Claimant<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let claimant_type = ClaimantType::from_i32(parser.parse_int32()?)?;
        match claimant_type {
            ClaimantType::V0 => Ok(Claimant::V0(ClaimantV0::parse(parser)?)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CreateClaimableBalanceOp<'a> {
    pub asset: Asset<'a>,
    pub amount: i64,
    pub claimants: VecM<Claimant<'a>, 10>,
}

impl<'a> XdrParse<'a> for CreateClaimableBalanceOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset = Asset::parse(parser)?;
        let amount = parser.parse_int64()?;
        let claimants = VecM::parse(parser)?;

        Ok(CreateClaimableBalanceOp {
            asset,
            amount,
            claimants,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClaimClaimableBalanceOp<'a> {
    pub balance_id: ClaimableBalanceId<'a>,
}

impl<'a> XdrParse<'a> for ClaimClaimableBalanceOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let balance_id = ClaimableBalanceId::parse(parser)?;
        Ok(ClaimClaimableBalanceOp { balance_id })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BeginSponsoringFutureReservesOp<'a> {
    pub sponsored_id: AccountId<'a>,
}

impl<'a> XdrParse<'a> for BeginSponsoringFutureReservesOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let sponsored_id = AccountId::parse(parser)?;
        Ok(BeginSponsoringFutureReservesOp { sponsored_id })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClawbackOp<'a> {
    pub asset: Asset<'a>,
    pub from: MuxedAccount<'a>,
    pub amount: i64,
}

impl<'a> XdrParse<'a> for ClawbackOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset = Asset::parse(parser)?;
        let from = MuxedAccount::parse(parser)?;
        let amount = parser.parse_int64()?;
        Ok(ClawbackOp {
            asset,
            from,
            amount,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClawbackClaimableBalanceOp<'a> {
    pub balance_id: ClaimableBalanceId<'a>,
}

impl<'a> XdrParse<'a> for ClawbackClaimableBalanceOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let balance_id = ClaimableBalanceId::parse(parser)?;
        Ok(ClawbackClaimableBalanceOp { balance_id })
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TrustLineFlags {
    AuthorizedFlag = 1,
    AuthorizedToMaintainLiabilitiesFlag = 2,
    TrustlineClawbackEnabledFlag = 4,
}

impl TrustLineFlags {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            1 => Ok(TrustLineFlags::AuthorizedFlag),
            2 => Ok(TrustLineFlags::AuthorizedToMaintainLiabilitiesFlag),
            4 => Ok(TrustLineFlags::TrustlineClawbackEnabledFlag),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for TrustLineFlags {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        TrustLineFlags::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SetTrustLineFlagsOp<'a> {
    pub trustor: AccountId<'a>,
    pub asset: Asset<'a>,
    pub clear_flags: u32,
    pub set_flags: u32,
}

impl<'a> XdrParse<'a> for SetTrustLineFlagsOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let trustor = AccountId::parse(parser)?;
        let asset = Asset::parse(parser)?;
        let clear_flags = parser.parse_uint32()?;
        let set_flags = parser.parse_uint32()?;
        Ok(SetTrustLineFlagsOp {
            trustor,
            asset,
            clear_flags,
            set_flags,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LiquidityPoolDepositOp<'a> {
    pub liquidity_pool_id: PoolId<'a>,
    pub max_amount_a: i64,
    pub max_amount_b: i64,
    pub min_price: Price,
    pub max_price: Price,
}

impl<'a> XdrParse<'a> for LiquidityPoolDepositOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let liquidity_pool_id = PoolId::parse(parser)?;
        let max_amount_a = parser.parse_int64()?;
        let max_amount_b = parser.parse_int64()?;
        let min_price = Price::parse(parser)?;
        let max_price = Price::parse(parser)?;
        Ok(LiquidityPoolDepositOp {
            liquidity_pool_id,
            max_amount_a,
            max_amount_b,
            min_price,
            max_price,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LiquidityPoolWithdrawOp<'a> {
    pub liquidity_pool_id: PoolId<'a>,
    pub amount: i64,
    pub min_amount_a: i64,
    pub min_amount_b: i64,
}

impl<'a> XdrParse<'a> for LiquidityPoolWithdrawOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let liquidity_pool_id = PoolId::parse(parser)?;
        let amount = parser.parse_int64()?;
        let min_amount_a = parser.parse_int64()?;
        let min_amount_b = parser.parse_int64()?;
        Ok(LiquidityPoolWithdrawOp {
            liquidity_pool_id,
            amount,
            min_amount_a,
            min_amount_b,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ExtensionPoint {
    V0,
}

impl<'a> XdrParse<'a> for ExtensionPoint {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let v = parser.parse_int32()?;
        match v {
            0 => Ok(ExtensionPoint::V0),
            _ => Err(ParseError::InvalidType(v)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ExtendFootprintTTLOp {
    pub ext: ExtensionPoint,
    pub extend_to: u32,
}

impl<'a> XdrParse<'a> for ExtendFootprintTTLOp {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let ext = ExtensionPoint::parse(parser)?;
        let extend_to = parser.parse_uint32()?;
        Ok(ExtendFootprintTTLOp { ext, extend_to })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RestoreFootprintOp {
    pub ext: ExtensionPoint,
}

impl<'a> XdrParse<'a> for RestoreFootprintOp {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let ext = ExtensionPoint::parse(parser)?;
        Ok(RestoreFootprintOp { ext })
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HostFunctionType {
    InvokeContract = 0,
    CreateContract = 1,
    UploadContractWasm = 2,
    CreateContractV2 = 3,
}

impl HostFunctionType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(HostFunctionType::InvokeContract),
            1 => Ok(HostFunctionType::CreateContract),
            2 => Ok(HostFunctionType::UploadContractWasm),
            3 => Ok(HostFunctionType::CreateContractV2),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for HostFunctionType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        HostFunctionType::from_i32(value)
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ContractIDPreimageType {
    FromAddress = 0,
    FromAsset = 1,
}

impl ContractIDPreimageType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ContractIDPreimageType::FromAddress),
            1 => Ok(ContractIDPreimageType::FromAsset),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for ContractIDPreimageType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        ContractIDPreimageType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ContractIDPreimageFromAddress<'a> {
    pub address: ScAddress<'a>,
    pub salt: Uint256<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ContractIDPreimage<'a> {
    FromAddress(ContractIDPreimageFromAddress<'a>),
    FromAsset(Asset<'a>),
}

impl<'a> XdrParse<'a> for ContractIDPreimage<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let preimage_type = ContractIDPreimageType::parse(parser)?;

        match preimage_type {
            ContractIDPreimageType::FromAddress => {
                let address = ScAddress::parse(parser)?;
                let salt = Uint256::parse(parser)?;
                Ok(ContractIDPreimage::FromAddress(
                    ContractIDPreimageFromAddress { address, salt },
                ))
            }
            ContractIDPreimageType::FromAsset => {
                let asset = Asset::parse(parser)?;
                Ok(ContractIDPreimage::FromAsset(asset))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ContractExecutableType {
    Wasm = 0,
    StellarAsset = 1,
}

impl ContractExecutableType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ContractExecutableType::Wasm),
            1 => Ok(ContractExecutableType::StellarAsset),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for ContractExecutableType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        ContractExecutableType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ContractExecutable<'a> {
    Wasm(Hash<'a>),
    StellarAsset,
}

impl<'a> XdrParse<'a> for ContractExecutable<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let exec_type = ContractExecutableType::parse(parser)?;

        match exec_type {
            ContractExecutableType::Wasm => {
                let wasm_hash = Hash::parse(parser)?;
                Ok(ContractExecutable::Wasm(wasm_hash))
            }
            ContractExecutableType::StellarAsset => Ok(ContractExecutable::StellarAsset),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CreateContractArgs<'a> {
    pub contract_id_preimage: ContractIDPreimage<'a>,
    pub executable: ContractExecutable<'a>,
}

impl<'a> XdrParse<'a> for CreateContractArgs<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let contract_id_preimage = ContractIDPreimage::parse(parser)?;
        let executable = ContractExecutable::parse(parser)?;
        Ok(CreateContractArgs {
            contract_id_preimage,
            executable,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CreateContractArgsV2<'a> {
    pub contract_id_preimage: ContractIDPreimage<'a>,
    pub executable: ContractExecutable<'a>,
    pub constructor_args: VecM<ScVal<'a>>,
}

impl<'a> XdrParse<'a> for CreateContractArgsV2<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let contract_id_preimage = ContractIDPreimage::parse(parser)?;
        let executable = ContractExecutable::parse(parser)?;
        let constructor_args = VecM::parse(parser)?;
        Ok(CreateContractArgsV2 {
            contract_id_preimage,
            executable,
            constructor_args,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InvokeContractArgs<'a> {
    pub contract_address: ScAddress<'a>,
    pub function_name: ScSymbol<'a>,
    pub args: VecM<ScVal<'a>>,
}

impl<'a> XdrParse<'a> for InvokeContractArgs<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let contract_address = ScAddress::parse(parser)?;
        let function_name = ScSymbol::parse(parser)?;
        let args = VecM::parse(parser)?;
        Ok(InvokeContractArgs {
            contract_address,
            function_name,
            args,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum HostFunction<'a> {
    InvokeContract(InvokeContractArgs<'a>),
    CreateContract(CreateContractArgs<'a>),
    UploadContractWasm(&'a [u8]),
    CreateContractV2(CreateContractArgsV2<'a>),
}

impl<'a> XdrParse<'a> for HostFunction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let func_type = HostFunctionType::parse(parser)?;

        match func_type {
            HostFunctionType::InvokeContract => {
                let invoke_args = InvokeContractArgs::parse(parser)?;
                Ok(HostFunction::InvokeContract(invoke_args))
            }
            HostFunctionType::CreateContract => {
                let create_args = CreateContractArgs::parse(parser)?;
                Ok(HostFunction::CreateContract(create_args))
            }
            HostFunctionType::UploadContractWasm => {
                let wasm = parser.parse_var_opaque(None)?;
                Ok(HostFunction::UploadContractWasm(wasm))
            }
            HostFunctionType::CreateContractV2 => {
                let create_args_v2 = CreateContractArgsV2::parse(parser)?;
                Ok(HostFunction::CreateContractV2(create_args_v2))
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SorobanAddressCredentials<'a> {
    pub address: ScAddress<'a>,
    pub nonce: i64,
    pub signature_expiration_ledger: u32,
    pub signature: ScVal<'a>,
}

impl<'a> XdrParse<'a> for SorobanAddressCredentials<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let address = ScAddress::parse(parser)?;
        let nonce = parser.parse_int64()?;
        let signature_expiration_ledger = parser.parse_uint32()?;
        let signature = ScVal::parse(parser)?;
        Ok(SorobanAddressCredentials {
            address,
            nonce,
            signature_expiration_ledger,
            signature,
        })
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SorobanCredentialsType {
    SourceAccount = 0,
    Address = 1,
}

impl SorobanCredentialsType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(SorobanCredentialsType::SourceAccount),
            1 => Ok(SorobanCredentialsType::Address),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for SorobanCredentialsType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        SorobanCredentialsType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SorobanCredentials<'a> {
    SourceAccount,
    Address(SorobanAddressCredentials<'a>),
}

impl<'a> XdrParse<'a> for SorobanCredentials<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let cred_type = SorobanCredentialsType::parse(parser)?;

        match cred_type {
            SorobanCredentialsType::SourceAccount => Ok(SorobanCredentials::SourceAccount),
            SorobanCredentialsType::Address => {
                let address = SorobanAddressCredentials::parse(parser)?;
                Ok(SorobanCredentials::Address(address))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SorobanAuthorizedFunctionType {
    ContractFn = 0,
    CreateContractHostFn = 1,
    CreateContractV2HostFn = 2,
}

impl SorobanAuthorizedFunctionType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(SorobanAuthorizedFunctionType::ContractFn),
            1 => Ok(SorobanAuthorizedFunctionType::CreateContractHostFn),
            2 => Ok(SorobanAuthorizedFunctionType::CreateContractV2HostFn),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for SorobanAuthorizedFunctionType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        SorobanAuthorizedFunctionType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SorobanAuthorizedFunction<'a> {
    ContractFn(InvokeContractArgs<'a>),
    CreateContractHostFn(CreateContractArgs<'a>),
    CreateContractV2HostFn(CreateContractArgsV2<'a>),
}

impl<'a> XdrParse<'a> for SorobanAuthorizedFunction<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let func_type = SorobanAuthorizedFunctionType::parse(parser)?;

        match func_type {
            SorobanAuthorizedFunctionType::ContractFn => {
                let contract_fn = InvokeContractArgs::parse(parser)?;
                Ok(SorobanAuthorizedFunction::ContractFn(contract_fn))
            }
            SorobanAuthorizedFunctionType::CreateContractHostFn => {
                let create_contract = CreateContractArgs::parse(parser)?;
                Ok(SorobanAuthorizedFunction::CreateContractHostFn(
                    create_contract,
                ))
            }
            SorobanAuthorizedFunctionType::CreateContractV2HostFn => {
                let create_contract_v2 = CreateContractArgsV2::parse(parser)?;
                Ok(SorobanAuthorizedFunction::CreateContractV2HostFn(
                    create_contract_v2,
                ))
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SorobanAuthorizedInvocation<'a> {
    pub function: SorobanAuthorizedFunction<'a>,
    pub sub_invocations: VecM<SorobanAuthorizedInvocation<'a>>,
}

impl<'a> XdrParse<'a> for SorobanAuthorizedInvocation<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let function = SorobanAuthorizedFunction::parse(parser)?;
        let sub_invocations = VecM::parse(parser)?;
        Ok(SorobanAuthorizedInvocation {
            function,
            sub_invocations,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SorobanAuthorizationEntry<'a> {
    pub credentials: SorobanCredentials<'a>,
    pub root_invocation: SorobanAuthorizedInvocation<'a>,
}

impl<'a> XdrParse<'a> for SorobanAuthorizationEntry<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let credentials = SorobanCredentials::parse(parser)?;
        let root_invocation = SorobanAuthorizedInvocation::parse(parser)?;
        Ok(SorobanAuthorizationEntry {
            credentials,
            root_invocation,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InvokeHostFunctionOp<'a> {
    pub host_function: HostFunction<'a>,
    pub auth: VecM<SorobanAuthorizationEntry<'a>>,
}

impl<'a> XdrParse<'a> for InvokeHostFunctionOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let host_function = HostFunction::parse(parser)?;
        let auth = VecM::parse(parser)?;
        Ok(InvokeHostFunctionOp {
            host_function,
            auth,
        })
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ContractDataDurability {
    Temporary = 0,
    Persistent = 1,
}

impl ContractDataDurability {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ContractDataDurability::Temporary),
            1 => Ok(ContractDataDurability::Persistent),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for ContractDataDurability {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        ContractDataDurability::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TrustLineAsset<'a> {
    Native,
    CreditAlphanum4(AlphaNum4<'a>),
    CreditAlphanum12(AlphaNum12<'a>),
    PoolShare(PoolId<'a>),
}

impl<'a> XdrParse<'a> for TrustLineAsset<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let asset_type = AssetType::parse(parser)?;

        match asset_type {
            AssetType::Native => Ok(TrustLineAsset::Native),
            AssetType::CreditAlphanum4 => {
                let alpha_num4 = AlphaNum4::parse(parser)?;
                Ok(TrustLineAsset::CreditAlphanum4(alpha_num4))
            }
            AssetType::CreditAlphanum12 => {
                let alpha_num12 = AlphaNum12::parse(parser)?;
                Ok(TrustLineAsset::CreditAlphanum12(alpha_num12))
            }
            AssetType::PoolShare => {
                let pool_id = PoolId::parse(parser)?;
                Ok(TrustLineAsset::PoolShare(pool_id))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfigSettingID {
    ConfigSettingContractMaxSizeBytes = 0,
    ConfigSettingContractComputeV0 = 1,
    ConfigSettingContractLedgerCostV0 = 2,
    ConfigSettingContractHistoricalDataV0 = 3,
    ConfigSettingContractEventsV0 = 4,
    ConfigSettingContractBandwidthV0 = 5,
    ConfigSettingContractCostParamsCpuInstructions = 6,
    ConfigSettingContractCostParamsMemoryBytes = 7,
    ConfigSettingContractDataKeySizeBytes = 8,
    ConfigSettingContractDataEntrySizeBytes = 9,
    ConfigSettingStateArchival = 10,
    ConfigSettingContractExecutionLanes = 11,
    ConfigSettingLiveSorobanStateSizeWindow = 12,
    ConfigSettingEvictionIterator = 13,
    ConfigSettingContractParallelComputeV0 = 14,
    ConfigSettingContractLedgerCostExtV0 = 15,
    ConfigSettingScpTiming = 16,
}

impl ConfigSettingID {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(ConfigSettingID::ConfigSettingContractMaxSizeBytes),
            1 => Ok(ConfigSettingID::ConfigSettingContractComputeV0),
            2 => Ok(ConfigSettingID::ConfigSettingContractLedgerCostV0),
            3 => Ok(ConfigSettingID::ConfigSettingContractHistoricalDataV0),
            4 => Ok(ConfigSettingID::ConfigSettingContractEventsV0),
            5 => Ok(ConfigSettingID::ConfigSettingContractBandwidthV0),
            6 => Ok(ConfigSettingID::ConfigSettingContractCostParamsCpuInstructions),
            7 => Ok(ConfigSettingID::ConfigSettingContractCostParamsMemoryBytes),
            8 => Ok(ConfigSettingID::ConfigSettingContractDataKeySizeBytes),
            9 => Ok(ConfigSettingID::ConfigSettingContractDataEntrySizeBytes),
            10 => Ok(ConfigSettingID::ConfigSettingStateArchival),
            11 => Ok(ConfigSettingID::ConfigSettingContractExecutionLanes),
            12 => Ok(ConfigSettingID::ConfigSettingLiveSorobanStateSizeWindow),
            13 => Ok(ConfigSettingID::ConfigSettingEvictionIterator),
            14 => Ok(ConfigSettingID::ConfigSettingContractParallelComputeV0),
            15 => Ok(ConfigSettingID::ConfigSettingContractLedgerCostExtV0),
            16 => Ok(ConfigSettingID::ConfigSettingScpTiming),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for ConfigSettingID {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        ConfigSettingID::from_i32(value)
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LedgerEntryType {
    Account = 0,
    Trustline = 1,
    Offer = 2,
    Data = 3,
    ClaimableBalance = 4,
    LiquidityPool = 5,
    ContractData = 6,
    ContractCode = 7,
    ConfigSetting = 8,
    Ttl = 9,
}

impl LedgerEntryType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(LedgerEntryType::Account),
            1 => Ok(LedgerEntryType::Trustline),
            2 => Ok(LedgerEntryType::Offer),
            3 => Ok(LedgerEntryType::Data),
            4 => Ok(LedgerEntryType::ClaimableBalance),
            5 => Ok(LedgerEntryType::LiquidityPool),
            6 => Ok(LedgerEntryType::ContractData),
            7 => Ok(LedgerEntryType::ContractCode),
            8 => Ok(LedgerEntryType::ConfigSetting),
            9 => Ok(LedgerEntryType::Ttl),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for LedgerEntryType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        LedgerEntryType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyAccount<'a> {
    pub account_id: AccountId<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyTrustLine<'a> {
    pub account_id: AccountId<'a>,
    pub asset: TrustLineAsset<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyOffer<'a> {
    pub seller_id: AccountId<'a>,
    pub offer_id: i64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyData<'a> {
    pub account_id: AccountId<'a>,
    pub data_name: StringM<'a, 64>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyClaimableBalance<'a> {
    pub balance_id: ClaimableBalanceId<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyLiquidityPool<'a> {
    pub liquidity_pool_id: PoolId<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyContractData<'a> {
    pub contract: ScAddress<'a>,
    pub key: ScVal<'a>,
    pub durability: ContractDataDurability,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyContractCode<'a> {
    pub hash: Hash<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyConfigSetting {
    pub config_setting_id: ConfigSettingID,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedgerKeyTtl<'a> {
    pub key_hash: Hash<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LedgerKey<'a> {
    Account(LedgerKeyAccount<'a>),
    Trustline(LedgerKeyTrustLine<'a>),
    Offer(LedgerKeyOffer<'a>),
    Data(LedgerKeyData<'a>),
    ClaimableBalance(LedgerKeyClaimableBalance<'a>),
    LiquidityPool(LedgerKeyLiquidityPool<'a>),
    ContractData(LedgerKeyContractData<'a>),
    ContractCode(LedgerKeyContractCode<'a>),
    ConfigSetting(LedgerKeyConfigSetting),
    Ttl(LedgerKeyTtl<'a>),
}

impl<'a> XdrParse<'a> for LedgerKey<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let entry_type = LedgerEntryType::parse(parser)?;

        match entry_type {
            LedgerEntryType::Account => {
                let account_id = AccountId::parse(parser)?;
                Ok(LedgerKey::Account(LedgerKeyAccount { account_id }))
            }
            LedgerEntryType::Trustline => {
                let account_id = AccountId::parse(parser)?;
                let asset = TrustLineAsset::parse(parser)?;
                Ok(LedgerKey::Trustline(LedgerKeyTrustLine {
                    account_id,
                    asset,
                }))
            }
            LedgerEntryType::Offer => {
                let seller_id = AccountId::parse(parser)?;
                let offer_id = parser.parse_int64()?;
                Ok(LedgerKey::Offer(LedgerKeyOffer {
                    seller_id,
                    offer_id,
                }))
            }
            LedgerEntryType::Data => {
                let account_id = AccountId::parse(parser)?;
                let data_name = StringM::parse(parser)?;
                Ok(LedgerKey::Data(LedgerKeyData {
                    account_id,
                    data_name,
                }))
            }
            LedgerEntryType::ClaimableBalance => {
                let balance_id = ClaimableBalanceId::parse(parser)?;
                Ok(LedgerKey::ClaimableBalance(LedgerKeyClaimableBalance {
                    balance_id,
                }))
            }
            LedgerEntryType::LiquidityPool => {
                let liquidity_pool_id = PoolId::parse(parser)?;
                Ok(LedgerKey::LiquidityPool(LedgerKeyLiquidityPool {
                    liquidity_pool_id,
                }))
            }
            LedgerEntryType::ContractData => {
                let contract = ScAddress::parse(parser)?;
                let key = ScVal::parse(parser)?;
                let durability = ContractDataDurability::parse(parser)?;
                Ok(LedgerKey::ContractData(LedgerKeyContractData {
                    contract,
                    key,
                    durability,
                }))
            }
            LedgerEntryType::ContractCode => {
                let hash = Hash::parse(parser)?;
                Ok(LedgerKey::ContractCode(LedgerKeyContractCode { hash }))
            }
            LedgerEntryType::ConfigSetting => {
                let config_setting_id = ConfigSettingID::parse(parser)?;
                Ok(LedgerKey::ConfigSetting(LedgerKeyConfigSetting {
                    config_setting_id,
                }))
            }
            LedgerEntryType::Ttl => {
                let key_hash = Hash::parse(parser)?;
                Ok(LedgerKey::Ttl(LedgerKeyTtl { key_hash }))
            }
        }
    }
}

#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RevokeSponsorshipType {
    LedgerEntry = 0,
    Signer = 1,
}

impl RevokeSponsorshipType {
    pub fn from_i32(value: i32) -> Result<Self, ParseError> {
        match value {
            0 => Ok(RevokeSponsorshipType::LedgerEntry),
            1 => Ok(RevokeSponsorshipType::Signer),
            _ => Err(ParseError::InvalidType(value)),
        }
    }
}

impl<'a> XdrParse<'a> for RevokeSponsorshipType {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let value = parser.parse_int32()?;
        RevokeSponsorshipType::from_i32(value)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RevokeSponsorshipOpSigner<'a> {
    pub account_id: AccountId<'a>,
    pub signer_key: SignerKey<'a>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RevokeSponsorshipOp<'a> {
    LedgerEntry(LedgerKey<'a>),
    Signer(RevokeSponsorshipOpSigner<'a>),
}

impl<'a> XdrParse<'a> for RevokeSponsorshipOp<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let sponsorship_type = RevokeSponsorshipType::parse(parser)?;

        match sponsorship_type {
            RevokeSponsorshipType::LedgerEntry => {
                let ledger_key = LedgerKey::parse(parser)?;
                Ok(RevokeSponsorshipOp::LedgerEntry(ledger_key))
            }
            RevokeSponsorshipType::Signer => {
                let account_id = AccountId::parse(parser)?;
                let signer_key = SignerKey::parse(parser)?;
                Ok(RevokeSponsorshipOp::Signer(RevokeSponsorshipOpSigner {
                    account_id,
                    signer_key,
                }))
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum OperationBody<'a> {
    CreateAccount(CreateAccountOp<'a>),
    Payment(PaymentOp<'a>),
    PathPaymentStrictReceive(PathPaymentStrictReceiveOp<'a>),
    ManageSellOffer(ManageSellOfferOp<'a>),
    CreatePassiveSellOffer(CreatePassiveSellOfferOp<'a>),
    SetOptions(SetOptionsOp<'a>),
    ChangeTrust(ChangeTrustOp<'a>),
    AllowTrust(AllowTrustOp<'a>),
    Inflation, // No fields
    AccountMerge(MuxedAccount<'a>),
    ManageData(ManageDataOp<'a>),
    BumpSequence(BumpSequenceOp),
    ManageBuyOffer(ManageBuyOfferOp<'a>),
    PathPaymentStrictSend(PathPaymentStrictSendOp<'a>),
    CreateClaimableBalance(CreateClaimableBalanceOp<'a>),
    ClaimClaimableBalance(ClaimClaimableBalanceOp<'a>),
    BeginSponsoringFutureReserves(BeginSponsoringFutureReservesOp<'a>),
    EndSponsoringFutureReserves, // No fields
    RevokeSponsorship(RevokeSponsorshipOp<'a>),
    Clawback(ClawbackOp<'a>),
    ClawbackClaimableBalance(ClawbackClaimableBalanceOp<'a>),
    SetTrustLineFlags(SetTrustLineFlagsOp<'a>),
    LiquidityPoolDeposit(LiquidityPoolDepositOp<'a>),
    LiquidityPoolWithdraw(LiquidityPoolWithdrawOp<'a>),
    InvokeHostFunction(InvokeHostFunctionOp<'a>),
    ExtendFootprintTTL(ExtendFootprintTTLOp),
    RestoreFootprint(RestoreFootprintOp),
}

impl<'a> XdrParse<'a> for OperationBody<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let op_type = OperationType::from_i32(parser.parse_int32()?)?;
        match op_type {
            OperationType::CreateAccount => Ok(OperationBody::CreateAccount(
                CreateAccountOp::parse(parser)?,
            )),
            OperationType::Payment => Ok(OperationBody::Payment(PaymentOp::parse(parser)?)),
            OperationType::PathPaymentStrictReceive => Ok(OperationBody::PathPaymentStrictReceive(
                PathPaymentStrictReceiveOp::parse(parser)?,
            )),
            OperationType::ManageSellOffer => Ok(OperationBody::ManageSellOffer(
                ManageSellOfferOp::parse(parser)?,
            )),
            OperationType::CreatePassiveSellOffer => Ok(OperationBody::CreatePassiveSellOffer(
                CreatePassiveSellOfferOp::parse(parser)?,
            )),
            OperationType::SetOptions => {
                Ok(OperationBody::SetOptions(SetOptionsOp::parse(parser)?))
            }
            OperationType::ChangeTrust => {
                Ok(OperationBody::ChangeTrust(ChangeTrustOp::parse(parser)?))
            }
            OperationType::AllowTrust => {
                Ok(OperationBody::AllowTrust(AllowTrustOp::parse(parser)?))
            }
            OperationType::AccountMerge => {
                Ok(OperationBody::AccountMerge(MuxedAccount::parse(parser)?))
            }
            OperationType::Inflation => Ok(OperationBody::Inflation),
            OperationType::ManageData => {
                Ok(OperationBody::ManageData(ManageDataOp::parse(parser)?))
            }
            OperationType::BumpSequence => {
                Ok(OperationBody::BumpSequence(BumpSequenceOp::parse(parser)?))
            }
            OperationType::ManageBuyOffer => Ok(OperationBody::ManageBuyOffer(
                ManageBuyOfferOp::parse(parser)?,
            )),
            OperationType::PathPaymentStrictSend => Ok(OperationBody::PathPaymentStrictSend(
                PathPaymentStrictSendOp::parse(parser)?,
            )),
            OperationType::CreateClaimableBalance => Ok(OperationBody::CreateClaimableBalance(
                CreateClaimableBalanceOp::parse(parser)?,
            )),
            OperationType::ClaimClaimableBalance => Ok(OperationBody::ClaimClaimableBalance(
                ClaimClaimableBalanceOp::parse(parser)?,
            )),
            OperationType::BeginSponsoringFutureReserves => {
                Ok(OperationBody::BeginSponsoringFutureReserves(
                    BeginSponsoringFutureReservesOp::parse(parser)?,
                ))
            }
            OperationType::EndSponsoringFutureReserves => {
                Ok(OperationBody::EndSponsoringFutureReserves)
            }
            OperationType::RevokeSponsorship => Ok(OperationBody::RevokeSponsorship(
                RevokeSponsorshipOp::parse(parser)?,
            )),
            OperationType::Clawback => Ok(OperationBody::Clawback(ClawbackOp::parse(parser)?)),
            OperationType::ClawbackClaimableBalance => Ok(OperationBody::ClawbackClaimableBalance(
                ClawbackClaimableBalanceOp::parse(parser)?,
            )),
            OperationType::SetTrustLineFlags => Ok(OperationBody::SetTrustLineFlags(
                SetTrustLineFlagsOp::parse(parser)?,
            )),
            OperationType::LiquidityPoolDeposit => Ok(OperationBody::LiquidityPoolDeposit(
                LiquidityPoolDepositOp::parse(parser)?,
            )),
            OperationType::LiquidityPoolWithdraw => Ok(OperationBody::LiquidityPoolWithdraw(
                LiquidityPoolWithdrawOp::parse(parser)?,
            )),
            OperationType::InvokeHostFunction => Ok(OperationBody::InvokeHostFunction(
                InvokeHostFunctionOp::parse(parser)?,
            )),
            OperationType::ExtendFootprintTtl => Ok(OperationBody::ExtendFootprintTTL(
                ExtendFootprintTTLOp::parse(parser)?,
            )),
            OperationType::RestoreFootprint => Ok(OperationBody::RestoreFootprint(
                RestoreFootprintOp::parse(parser)?,
            )),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Operation<'a> {
    pub source_account: Option<MuxedAccount<'a>>,
    pub body: OperationBody<'a>,
}

impl<'a> XdrParse<'a> for Operation<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let source_account = parser.parse_optional(MuxedAccount::parse)?;
        let body = OperationBody::parse(parser)?;
        Ok(Operation {
            source_account,
            body,
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum HashIDPreimage<'a> {
    SorobanAuthorization(HashIdPreimageSorobanAuthorization<'a>),
}

impl<'a> XdrParse<'a> for HashIDPreimage<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let preimage_type = EnvelopeType::parse(parser)?;
        match preimage_type {
            EnvelopeType::EnvelopeTypeSorobanAuthorization => {
                let soroban_auth = HashIdPreimageSorobanAuthorization::parse(parser)?;
                Ok(HashIDPreimage::SorobanAuthorization(soroban_auth))
            }
            _ => Err(ParseError::InvalidType(preimage_type as i32)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct HashIdPreimageSorobanAuthorization<'a> {
    pub network_id: Hash<'a>,
    pub nonce: i64,
    pub signature_expiration_ledger: u32,
    pub invocation: SorobanAuthorizedInvocation<'a>,
}

impl<'a> XdrParse<'a> for HashIdPreimageSorobanAuthorization<'a> {
    fn parse(parser: &mut Parser<'a>) -> Result<Self, ParseError> {
        let network_id = Hash::parse(parser)?;
        let nonce = parser.parse_int64()?;
        let signature_expiration_ledger = parser.parse_uint32()?;
        let invocation = SorobanAuthorizedInvocation::parse(parser)?;
        Ok(HashIdPreimageSorobanAuthorization {
            network_id,
            nonce,
            signature_expiration_ledger,
            invocation,
        })
    }
}

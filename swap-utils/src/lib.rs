//! Utility functions for Stellar swap operations
//!
//! This module provides safe, no_std and no-alloc compatible utilities for:
//! - Formatting Stellar addresses from Ed25519 public keys
//! - Converting XLM amounts from stroops to human-readable format

#![no_std]

mod crc;
mod format;

pub use format::{format_printable_amount, format_stellar_address, FormatError};

use base64::{engine::general_purpose::STANDARD as BASE64, Engine};
use openssl::hash::Hasher;
pub use openssl::hash::MessageDigest;
pub use openssl::pkey;
pub use openssl::rsa::{Padding, Rsa};
pub use openssl::symm::Cipher;
use openssl::symm::{Crypter, Mode};
use std::error::Error;

type Result<T> = std::result::Result<T, Box<dyn Error>>;

pub fn encrypt(cipher: Cipher, key: &[u8], iv: &[u8], data: &[u8]) -> Result<Vec<u8>> {
    let mut encrypter = Crypter::new(cipher, Mode::Encrypt, key, Some(iv))?;
    let block_size = cipher.block_size();
    let mut output = vec![0; data.len() + block_size];

    let mut count = encrypter.update(data, &mut output)?;
    count += encrypter.finalize(&mut output[count..])?;
    output.truncate(count);

    Ok(output)
}

pub fn decrypt(cipher: Cipher, key: &[u8], iv: &[u8], data: &[u8]) -> Result<Vec<u8>> {
    let mut decrypter = Crypter::new(cipher, Mode::Decrypt, key, Some(iv))?;
    let block_size = cipher.block_size();
    let mut output = vec![0; data.len() + block_size];

    let mut count = decrypter.update(data, &mut output)?;
    count += decrypter.finalize(&mut output[count..])?;
    output.truncate(count);

    Ok(output)
}

pub fn encode_block(data: &[u8]) -> String {
    BASE64.encode(data)
}

// with 64 '\n'
pub fn encode(data: &[u8]) -> Result<Vec<u8>> {
    let block = BASE64.encode(data);
    let block_bytes = block.as_bytes();
    let len = block.len();
    let block_len = len / 64;
    let last_line_len = len % 64;
    let fin = {
        let end = (last_line_len > 0) as usize;
        len + block_len + end
    };
    let mut out: Vec<u8> = Vec::new();
    out.resize(fin, 0);

    for i in 0..block_len {
        let out_prefix = i * 65;
        let in_prefix = i * 64;
        for j in 0..64 {
            out[out_prefix + j] = block_bytes[in_prefix + j];
        }
        out[out_prefix + 64] = '\n' as u8;
    }
    if last_line_len > 0 {
        let out_prefix = block_len * 65;
        let in_prefix = block_len * 64;
        for i in 0..last_line_len {
            out[out_prefix + i] = block_bytes[in_prefix + i];
        }
        out[out_prefix + last_line_len] = '\n' as u8;
    }
    Ok(out)
}

pub fn decode(data: &[u8]) -> Result<Vec<u8>> {
    Ok(BASE64.decode(data)?)
}

pub fn digest(digest_type: MessageDigest, data: &[u8]) -> Result<Vec<u8>> {
    let mut hasher = Hasher::new(digest_type)?;
    hasher.update(data)?;
    Ok(hasher.finish()?.to_vec())
}

pub mod hex {
    pub fn encode_low(data: &[u8]) -> Vec<u8> {
        hex::encode(data).into_bytes()
    }

    pub fn encode_up(data: &[u8]) -> Vec<u8> {
        hex::encode_upper(data).into_bytes()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encrypt_decrypt_aes128_cbc_roundtrip() {
        let key = b"0123456789abcdef"; // 16 bytes
        let iv = b"abcdef0123456789"; // 16 bytes
        let data = b"Hello, World! This is a test.";

        let encrypted = encrypt(Cipher::aes_128_cbc(), key, iv, data).unwrap();
        assert_ne!(encrypted, data);

        let decrypted = decrypt(Cipher::aes_128_cbc(), key, iv, &encrypted).unwrap();
        assert_eq!(decrypted, data);
    }

    #[test]
    fn test_encrypt_decrypt_aes128_ecb_roundtrip() {
        let key = b"0123456789abcdef";
        let iv = b""; // ECB doesn't use IV but openssl requires it
        let data = b"Test data for ECB mode!!"; // 24 bytes

        let encrypted = encrypt(Cipher::aes_128_ecb(), key, iv, data).unwrap();
        let decrypted = decrypt(Cipher::aes_128_ecb(), key, iv, &encrypted).unwrap();
        assert_eq!(decrypted, data);
    }

    #[test]
    fn test_base64_encode_block() {
        assert_eq!(encode_block(b"Hello"), "SGVsbG8=");
        assert_eq!(encode_block(b""), "");
        assert_eq!(encode_block(b"a"), "YQ==");
    }

    #[test]
    fn test_base64_decode() {
        let decoded = decode(b"SGVsbG8=").unwrap();
        assert_eq!(decoded, b"Hello");
    }

    #[test]
    fn test_base64_encode_decode_roundtrip() {
        let data = b"Round-trip test data with special chars: \x00\x01\xff";
        let encoded = encode_block(data);
        let decoded = decode(encoded.as_bytes()).unwrap();
        assert_eq!(decoded, data);
    }

    #[test]
    fn test_encode_with_newlines_short() {
        // Data shorter than 64 base64 chars
        let data = b"Hello";
        let result = encode(data).unwrap();
        let result_str = String::from_utf8(result).unwrap();
        assert!(result_str.ends_with('\n'));
        assert_eq!(result_str.trim(), "SGVsbG8=");
    }

    #[test]
    fn test_encode_with_newlines_long() {
        // Data that produces > 64 base64 chars
        let data = vec![0u8; 100]; // 100 bytes -> 136 base64 chars -> 2+ lines
        let result = encode(&data).unwrap();
        let result_str = String::from_utf8(result).unwrap();
        let lines: Vec<&str> = result_str.split('\n').filter(|l| !l.is_empty()).collect();
        assert!(lines.len() >= 2);
        assert_eq!(lines[0].len(), 64);
    }

    #[test]
    fn test_digest_md5() {
        let data = b"hello";
        let hash = digest(MessageDigest::md5(), data).unwrap();
        assert_eq!(hash.len(), 16);
        let hex_str = ::hex::encode(&hash);
        assert_eq!(hex_str, "5d41402abc4b2a76b9719d911017c592");
    }

    #[test]
    fn test_digest_sha256() {
        let data = b"hello";
        let hash = digest(MessageDigest::sha256(), data).unwrap();
        assert_eq!(hash.len(), 32);
        let hex_str = ::hex::encode(&hash);
        assert_eq!(
            hex_str,
            "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"
        );
    }

    #[test]
    fn test_digest_empty_input() {
        let hash = digest(MessageDigest::md5(), b"").unwrap();
        let hex_str = ::hex::encode(&hash);
        assert_eq!(hex_str, "d41d8cd98f00b204e9800998ecf8427e");
    }

    #[test]
    fn test_hex_encode_low() {
        let data = &[0xde, 0xad, 0xbe, 0xef];
        assert_eq!(hex::encode_low(data), b"deadbeef");
    }

    #[test]
    fn test_hex_encode_up() {
        let data = &[0xde, 0xad, 0xbe, 0xef];
        assert_eq!(hex::encode_up(data), b"DEADBEEF");
    }

    #[test]
    fn test_hex_encode_empty() {
        assert_eq!(hex::encode_low(&[]), b"");
        assert_eq!(hex::encode_up(&[]), b"");
    }
}

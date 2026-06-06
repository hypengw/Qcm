use unicode_normalization::UnicodeNormalization;

#[derive(Debug, PartialEq, Clone)]
pub enum Token {
    Alphabetic(String, usize),
    Numeric(String, usize),
    NGram(String, usize),
}

#[derive(Debug, PartialEq)]
enum CharType {
    Alphabetic,
    Numeric,
    Other,
    Separator,
}

#[derive(Debug)]
pub struct FtsTokenizer {}

impl FtsTokenizer {
    pub fn new() -> Self {
        FtsTokenizer {}
    }

    fn get_char_type(c: char) -> CharType {
        if c.is_ascii_alphabetic() {
            CharType::Alphabetic
        } else if c.is_ascii_digit() {
            CharType::Numeric
        } else if (c as u32) < 0x80 {
            CharType::Separator
        } else {
            CharType::Other
        }
    }

    fn process_token(token: &str, char_type: CharType, start_pos: usize) -> Vec<Token> {
        if token.is_empty() {
            return vec![];
        }

        match char_type {
            CharType::Numeric => vec![Token::Numeric(token.to_string(), start_pos)],
            CharType::Alphabetic => vec![Token::Alphabetic(token.to_lowercase(), start_pos)],
            CharType::Other => {
                let chars: Vec<char> = token.chars().collect();
                if chars.len() < 2 {
                    vec![Token::NGram(token.to_string(), start_pos)]
                } else {
                    let mut pos = start_pos;
                    chars
                        .windows(2)
                        .map(|window| {
                            let token = Token::NGram(window.iter().collect::<String>(), pos);
                            pos += 1;
                            token
                        })
                        .collect()
                }
            }
            CharType::Separator => vec![],
        }
    }

    pub fn tokenize(&self, input: &str) -> Vec<Token> {
        let normalized = input.nfkc().collect::<String>();
        let mut result = Vec::new();
        let mut current_token = String::new();
        let mut current_type = CharType::Separator;
        let mut current_start = 0;
        let mut pos = 0;

        for c in normalized.chars() {
            let char_type = Self::get_char_type(c);

            if char_type != current_type && !current_token.is_empty() {
                result.extend(Self::process_token(
                    &current_token,
                    current_type,
                    current_start,
                ));
                current_token.clear();
                current_start = pos;
            }

            if char_type != CharType::Separator {
                if current_token.is_empty() {
                    current_start = pos;
                }
                current_token.push(c);
            }
            current_type = char_type;
            pos += 1;
        }

        if !current_token.is_empty() {
            result.extend(Self::process_token(
                &current_token,
                current_type,
                current_start,
            ));
        }

        result
    }

    pub fn tokenize_query(&self, query: &str) -> String {
        let tokens = self.tokenize(query);
        let mut fts_conditions = Vec::new();

        for token in tokens {
            let term = match token {
                Token::Alphabetic(s, _) => format!("{}*", s), // Match word start
                Token::Numeric(s, _) => s,                        // Exact match for numbers
                Token::NGram(s, _) => format!("{}*", s),        // Partial match for n-grams
            };
            fts_conditions.push(term);
        }

        // Combine all conditions with AND
        fts_conditions.join(" AND ")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_tokenizer() {
        let tokenizer = FtsTokenizer::new();

        // Test alphabetic (lowercase)
        assert_eq!(
            tokenizer.tokenize("Hello"),
            vec![Token::Alphabetic("hello".to_string(), 0)]
        );

        // Test numeric
        assert_eq!(
            tokenizer.tokenize("12345"),
            vec![Token::Numeric("12345".to_string(), 0)]
        );

        // Test mixed with separators
        assert_eq!(
            tokenizer.tokenize("Hello-World"),
            vec![
                Token::Alphabetic("hello".to_string(), 0),
                Token::Alphabetic("world".to_string(), 6)
            ]
        );

        // Test Chinese characters (ngram)
        assert_eq!(
            tokenizer.tokenize("中国"),
            vec![Token::NGram("中国".to_string(), 0)]
        );
        assert_eq!(
            tokenizer.tokenize("中国人"),
            vec![
                Token::NGram("中国".to_string(), 0),
                Token::NGram("国人".to_string(), 1)
            ]
        );

        // Test mixed content
        assert_eq!(
            tokenizer.tokenize("Hello世界123"),
            vec![
                Token::Alphabetic("hello".to_string(), 0),
                Token::NGram("世界".to_string(), 5),
                Token::Numeric("123".to_string(), 7)
            ]
        );

        // Test mixed without separators
        assert_eq!(
            tokenizer.tokenize("abc123世界def"),
            vec![
                Token::Alphabetic("abc".to_string(), 0),
                Token::Numeric("123".to_string(), 3),
                Token::NGram("世界".to_string(), 6),
                Token::Alphabetic("def".to_string(), 8)
            ]
        );

        // Test mixed with Chinese
        assert_eq!(
            tokenizer.tokenize("我abc123"),
            vec![
                Token::NGram("我".to_string(), 0),
                Token::Alphabetic("abc".to_string(), 1),
                Token::Numeric("123".to_string(), 4)
            ]
        );

        // Test mixed numbers and letters
        assert_eq!(
            tokenizer.tokenize("abc123def456"),
            vec![
                Token::Alphabetic("abc".to_string(), 0),
                Token::Numeric("123".to_string(), 3),
                Token::Alphabetic("def".to_string(), 6),
                Token::Numeric("456".to_string(), 9)
            ]
        );
    }

    #[test]
    fn test_query_tokenization() {
        let tokenizer = FtsTokenizer::new();
        
        // Test alphabetic query
        assert_eq!(tokenizer.tokenize_query("Hello"), "hello*");
        
        // Test numeric query
        assert_eq!(tokenizer.tokenize_query("123"), "123");
        
        // Test mixed query
        assert_eq!(
            tokenizer.tokenize_query("Hello世界123"),
            "hello* AND 世界* AND 123"
        );
        
        // Test Chinese query
        assert_eq!(
            tokenizer.tokenize_query("中国人"),
            "中国* AND 国人*"
        );
    }

    #[test]
    fn test_complex_query_tokenization() {
        let tokenizer = FtsTokenizer::new();
        
        // Test mixed separators
        assert_eq!(
            tokenizer.tokenize_query("Hello-World 123"),
            "hello* AND world* AND 123"
        );
        
        // Test mixed content with multiple tokens
        assert_eq!(
            tokenizer.tokenize_query("abc123世界def456"),
            "abc* AND 123 AND 世界* AND def* AND 456"
        );
        
        // Test mixed with Chinese characters
        assert_eq!(
            tokenizer.tokenize_query("中国abc123"),
            "中国* AND abc* AND 123"
        );
    }

    // --- Edge case tests ---

    #[test]
    fn test_empty_input() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(tokenizer.tokenize(""), Vec::<Token>::new());
        assert_eq!(tokenizer.tokenize_query(""), "");
    }

    #[test]
    fn test_whitespace_only() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(tokenizer.tokenize("   "), Vec::<Token>::new());
        assert_eq!(tokenizer.tokenize("  \t "), Vec::<Token>::new());
    }

    #[test]
    fn test_pure_ascii_alphabetic() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(
            tokenizer.tokenize("abcdef"),
            vec![Token::Alphabetic("abcdef".to_string(), 0)]
        );
    }

    #[test]
    fn test_pure_numeric() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(
            tokenizer.tokenize("9876543210"),
            vec![Token::Numeric("9876543210".to_string(), 0)]
        );
    }

    #[test]
    fn test_pure_cjk() {
        let tokenizer = FtsTokenizer::new();
        let tokens = tokenizer.tokenize("你好世界测试");
        // 6 chars -> 5 bigrams: 你好, 好世, 世界, 界测, 测试
        assert_eq!(tokens.len(), 5);
        assert_eq!(tokens[0], Token::NGram("你好".to_string(), 0));
        assert_eq!(tokens[1], Token::NGram("好世".to_string(), 1));
        assert_eq!(tokens[4], Token::NGram("测试".to_string(), 4));
    }

    #[test]
    fn test_single_cjk_char() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(
            tokenizer.tokenize("中"),
            vec![Token::NGram("中".to_string(), 0)]
        );
    }

    #[test]
    fn test_special_ascii_separators() {
        let tokenizer = FtsTokenizer::new();
        // Punctuation, brackets, etc. are separators
        assert_eq!(
            tokenizer.tokenize("a.b,c!d"),
            vec![
                Token::Alphabetic("a".to_string(), 0),
                Token::Alphabetic("b".to_string(), 2),
                Token::Alphabetic("c".to_string(), 4),
                Token::Alphabetic("d".to_string(), 6),
            ]
        );
    }

    #[test]
    fn test_nfkc_fullwidth_to_halfwidth() {
        let tokenizer = FtsTokenizer::new();
        // Fullwidth 'Ａ' (U+FF21) normalizes to 'A' via NFKC
        let tokens = tokenizer.tokenize("\u{FF21}\u{FF22}\u{FF23}");
        assert_eq!(
            tokens,
            vec![Token::Alphabetic("abc".to_string(), 0)]
        );
    }

    #[test]
    fn test_nfkc_fullwidth_digits() {
        let tokenizer = FtsTokenizer::new();
        // Fullwidth '１２３' normalizes to '123'
        let tokens = tokenizer.tokenize("\u{FF11}\u{FF12}\u{FF13}");
        assert_eq!(
            tokens,
            vec![Token::Numeric("123".to_string(), 0)]
        );
    }

    #[test]
    fn test_japanese_hiragana() {
        let tokenizer = FtsTokenizer::new();
        let tokens = tokenizer.tokenize("あいう");
        assert_eq!(tokens.len(), 2); // 3 chars -> 2 bigrams
        assert_eq!(tokens[0], Token::NGram("あい".to_string(), 0));
        assert_eq!(tokens[1], Token::NGram("いう".to_string(), 1));
    }

    #[test]
    fn test_query_single_char() {
        let tokenizer = FtsTokenizer::new();
        assert_eq!(tokenizer.tokenize_query("a"), "a*");
        assert_eq!(tokenizer.tokenize_query("1"), "1");
        assert_eq!(tokenizer.tokenize_query("中"), "中*");
    }

    #[test]
    fn test_long_input() {
        let tokenizer = FtsTokenizer::new();
        let long_str = "a".repeat(10000);
        let tokens = tokenizer.tokenize(&long_str);
        assert_eq!(tokens.len(), 1);
        assert_eq!(tokens[0], Token::Alphabetic(long_str, 0));
    }
}

use nom::error::ErrorKind as NomErrorKind;
use nom::error::ParseError;

#[derive(Clone, Debug, PartialEq)]
pub enum ErrorKind {
    InvalidTimestamp,
    InvalidOffset,
    Incomplete,
    Nom(NomErrorKind),
}

#[derive(Clone, Debug, PartialEq)]
pub struct Error<I> {
    /// position of the error in the input data
    pub input: I,
    /// nom error code
    pub code: ErrorKind,
}

impl<I> Error<I> {
    pub fn new(input: I, code: ErrorKind) -> Self {
        Error { input, code }
    }

    pub fn nom(input: I, code: ErrorKind) -> nom::Err<Self> {
        nom::Err::Error(Error { input, code })
    }
}

impl<I> ParseError<I> for Error<I> {
    fn from_error_kind(input: I, kind: NomErrorKind) -> Self {
        Error {
            input,
            code: ErrorKind::Nom(kind),
        }
    }

    fn append(_: I, _: NomErrorKind, other: Self) -> Self {
        other
    }
}

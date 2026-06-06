use num_enum::{IntoPrimitive, TryFromPrimitive};
use sea_orm::entity::prelude::*;
use serde::{Deserialize, Serialize};
use strum_macros::{Display, EnumString};

#[derive(
    Copy,
    Debug,
    Clone,
    PartialEq,
    Eq,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    DeriveActiveEnum,
    TryFromPrimitive,
)]
#[sea_orm(rs_type = "i32", db_type = "Integer")]
#[repr(i32)]
pub enum CacheType {
    Image = 0,
    Audio = 1,
    M3u = 2,
    AudioTs = 3,
}

#[derive(
    Copy,
    Debug,
    Clone,
    Display,
    PartialEq,
    Eq,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    TryFromPrimitive,
    DeriveActiveEnum,
)]
#[sea_orm(rs_type = "i32", db_type = "Integer")]
#[strum(ascii_case_insensitive)]
#[repr(i32)]
pub enum ImageType {
    #[strum(to_string = "Primary")]
    Primary = 0,
    #[strum(to_string = "Backdrop")]
    Backdrop = 1,
    #[strum(to_string = "Banner")]
    Banner = 2,
    #[strum(to_string = "Thumb")]
    Thumb = 3,
    #[strum(to_string = "Logo")]
    Logo = 4,
}

#[derive(
    Copy,
    Debug,
    Clone,
    Default,
    PartialEq,
    Display,
    Eq,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    DeriveActiveEnum,
    TryFromPrimitive,
    IntoPrimitive,
)]
#[sea_orm(rs_type = "i32", db_type = "Integer")]
#[strum(ascii_case_insensitive)]
#[repr(i32)]
pub enum ItemType {
    #[default]
    UnSpecified = 0,
    Provider = 1,
    Library = 2,

    Album = 51,
    AlbumArtist = 52,
    Artist = 53,
    Mix = 54,
    Radio = 55,

    Song = 101,
    Program = 102,
}

#[derive(
    Copy,
    Debug,
    Clone,
    PartialEq,
    Eq,
    Display,
    Default,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    DeriveActiveEnum,
    TryFromPrimitive,
    IntoPrimitive,
)]
#[sea_orm(rs_type = "i32", db_type = "Integer")]
#[repr(i32)]
pub enum AlbumType {
    Unknown = 0,
    #[default]
    Album = 1,
    EP = 2,
    Single = 3,
    Soundtrack = 4,
    Compilation = 5,
    Live = 6,
}

#[derive(
    Copy,
    Debug,
    Clone,
    PartialEq,
    Eq,
    Default,
    Serialize,
    Deserialize,
    EnumIter,
    EnumString,
    DeriveActiveEnum,
    TryFromPrimitive,
)]
#[sea_orm(rs_type = "i32", db_type = "Integer")]
#[repr(i32)]
pub enum MixType {
    #[default]
    Normal = 0,
    Link = 1,
    Cache = 2,
}

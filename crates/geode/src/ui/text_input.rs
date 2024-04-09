use proc_macros::*;
use rcocos2d_sys::cocos2d_CCSprite as CCSprite;

type Void = !;

#[derive(Debug)]
pub enum CommonFilter {
    Uint,
    Int,
    Float,
    ID,
    Name,
    Any,
    Hex,
    Base64Normal,
    Base64URL,
}

impl CommonFilter {
    pub fn get_common_filter_allowed_chars(filter: CommonFilter) -> CommonFilter {
        filter
    }
}

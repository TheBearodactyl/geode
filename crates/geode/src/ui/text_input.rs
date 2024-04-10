
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

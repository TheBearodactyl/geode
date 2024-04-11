pub trait SettingNode {}
pub trait SettingValue {}

pub struct JsonMaybeObject;
pub struct JsonMaybeValue;

pub struct BoolSetting {
    name: Option<String>,
    description: Option<String>,
    default_value: bool,
}

pub struct IntSetting {
    name: Option<String>,
    description: Option<String>,
    min: i64,
    max: i64,
    controls: IntSettingControls,
}

pub struct IntSettingControls {
    arrows: bool,
    big_arrows: bool,
    arrow_step: usize,
    big_arrow_step: usize,
    slider: bool,
    slider_step: Option<u64>,
    input: bool,
}

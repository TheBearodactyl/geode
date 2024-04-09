use rcocos2d_sys::cocos2d_CCArray as CCArray;
use rcocos2d_sys::cocos2d_CCLabelBMFont as CCLabelBMFont;
use rcocos2d_sys::cocos2d_CCSprite as CCSprite;

pub const NOTIFICATION_DEFAULT_TIME: f64 = 1.0;
pub const NOTIFICATION_LONG_TIME: f64 = 4.0;

pub enum NotificationIcon {
    None,
    Loading,
    Success,
    Warning,
    Error,
}

pub struct Notification<'a> {
    pub s_queue: &'a CCArray,
    pub m_bg: CCSprite,
    pub m_label: CCLabelBMFont,
    pub m_icon: CCSprite,
    pub m_time: f64,
    pub m_showing: bool,
}

pub trait PrivNotification {
    fn init(text: String, icon: CCSprite, time: f64) -> bool;
    fn create_icon(icon: NotificationIcon) -> CCSprite;

    fn update_layout();
    fn animate_in();
    fn animate_out();
    fn show_next_notification();
    fn wait();
}

pub trait PubNotification<'a> {
    fn create(text: String, icon: Option<NotificationIcon>, time: f64) -> Notification<'a>;
    fn create_with_sprite(text: String, icon: CCSprite, time: f64) -> Notification<'a>;

    fn set_string(text: String);
    fn set_icon(icon: Option<NotificationIcon>);
    fn set_icon_with_sprite(icon: CCSprite);
    fn set_time(time: f64);
    fn wait_and_hide();
    fn show();
    fn hide();
}

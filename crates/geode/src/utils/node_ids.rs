use rcocos2d_sys::cocos2d_CCNode as CCNode;
use rcocos2d_sys::cocos2d_CCSize as CCSize;
use rcocos2d_sys::cocos2d_CCSizeZero as CCSizeZero;

pub static GEODE_ID_PRIORITY: i32 = 0x100000;

#[inline]
pub unsafe fn get_size_safe(node: Option<CCNode>) -> CCSize {
    if node.is_some() {
        node.unwrap().m_obContentSize
    } else {
        CCSizeZero
    }
}

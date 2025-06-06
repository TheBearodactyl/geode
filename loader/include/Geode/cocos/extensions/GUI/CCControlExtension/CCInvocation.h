/*
 * Copyright (c) 2012 cocos2d-x.org
 * http://www.cocos2d-x.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * Converted to c++ / cocos2d-x by Angus C
 */
/*
 *
 * Helper class to store targets and selectors (and eventually, params?) in the same CCMutableArray. Basically a very crude form of a NSInvocation
 */
#ifndef __CCINVOCATION_H__
#define __CCINVOCATION_H__

#include "../../../cocoa/CCObject.h"
#include "../../ExtensionMacros.h"

NS_CC_EXT_BEGIN

/**
 * @addtogroup GUI
 * @{
 * @addtogroup control_extension
 * @{
 */

typedef unsigned int CCControlEvent;

typedef void (CCObject::*SEL_CCControlHandler)(CCObject*, CCControlEvent);

#define cccontrol_selector(_SELECTOR) (SEL_CCControlHandler)(&_SELECTOR)

/**
 * @js NA
 * @lua NA
 */
class CC_DLL CCInvocation : public CCObject
{
    CC_SYNTHESIZE_READONLY(SEL_CCControlHandler, m_action, Action);
    CC_SYNTHESIZE_READONLY(CCObject*, m_target, Target);
    CC_SYNTHESIZE_READONLY(CCControlEvent, m_controlEvent, ControlEvent);

public:
    static CCInvocation* create(CCObject* target, SEL_CCControlHandler action, CCControlEvent controlEvent);
    CCInvocation(CCObject* target, SEL_CCControlHandler action, CCControlEvent controlEvent);

    void invoke(CCObject* sender);
};

// end of GUI group
/// @}
/// @}

NS_CC_EXT_END

#endif
/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#ifndef __CC_TOUCH_H__
#define __CC_TOUCH_H__

#include "../cocoa/CCObject.h"
#include "../cocoa/CCGeometry.h"

NS_CC_BEGIN

/**
 * @addtogroup input
 * @{
 */

class CC_DLL CCTouch : public CCObject
{
    GEODE_FRIEND_MODIFY
public:
    /**
     * @js ctor
     */
    CCTouch()
        : m_nId(0),
        m_startPointCaptured(false)
    {}

    /** returns the current touch location in OpenGL coordinates */
    CCPoint getLocation() const;
    /** returns the previous touch location in OpenGL coordinates */
    CCPoint getPreviousLocation() const;
    /** returns the start touch location in OpenGL coordinates */
    CCPoint getStartLocation() const;
    /** returns the delta of 2 current touches locations in screen coordinates */
    CCPoint getDelta() const;
    /** returns the current touch location in screen coordinates */
    CCPoint getLocationInView() const;
    /** returns the previous touch location in screen coordinates */
    CCPoint getPreviousLocationInView() const;
    /** returns the start touch location in screen coordinates */
    CCPoint getStartLocationInView() const;

    void setTouchInfo(int id, float x, float y)
    {
        m_nId = id;
        m_prevPoint = m_point;
        m_point.x   = x;
        m_point.y   = y;
        if (!m_startPointCaptured)
        {
            m_startPoint = m_point;
            m_startPointCaptured = true;
        }
    }
    /**
     *  @js getId
     */
    int getID() const
    {
        return m_nId;
    }

public:
    int m_nId;
    bool m_startPointCaptured;
    CCPoint m_startPoint;
    CCPoint m_point;
    CCPoint m_prevPoint;
};

class CC_DLL CCEvent : public CCObject
{
    GEODE_FRIEND_MODIFY
};

// end of input group
/// @}

NS_CC_END

#endif  // __PLATFORM_TOUCH_H__

/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2009 Lam Pham
Copyright (c) 2012 Ricardo Quesada

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

#ifndef __CCTRANSITIONPROGRESS_H__
#define __CCTRANSITIONPROGRESS_H__

#include "CCTransition.h"

NS_CC_BEGIN

class CCProgressTimer;
class CCRenderTexture;

/**
 * @addtogroup transition
 * @{
 */

class CC_DLL CCTransitionProgress : public CCTransitionScene
{
    GEODE_FRIEND_MODIFY
public:
    static CCTransitionProgress* create(float t, CCScene* scene);

    GEODE_CUSTOM_CONSTRUCTOR_COCOS(CCTransitionProgress, CCTransitionScene)
    /**
     *  @js ctor
     */
    CCTransitionProgress();
    /**
     *  @js NA
     *  @lua NA
     */
    virtual void onEnter();
    /**
     *  @js NA
     *  @lua NA
     */
    virtual void onExit();
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);
    virtual void setupTransition();
    virtual void sceneOrder();
public:
    float m_fTo;
    float m_fFrom;
    CCScene* m_pSceneToBeModified;
};


/** CCTransitionRadialCCW transition.
 A counter clock-wise radial transition to the next scene
 */
class CC_DLL CCTransitionProgressRadialCCW : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:
    static CCTransitionProgressRadialCCW* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);

};


/** CCTransitionRadialCW transition.
 A counter clock-wise radial transition to the next scene
*/
class CC_DLL CCTransitionProgressRadialCW : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:
    static CCTransitionProgressRadialCW* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);

};

/** CCTransitionProgressHorizontal transition.
 A  clock-wise radial transition to the next scene
 */
class CC_DLL CCTransitionProgressHorizontal : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:

    static CCTransitionProgressHorizontal* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);

};

class CC_DLL CCTransitionProgressVertical : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:

    static CCTransitionProgressVertical* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);

};

class CC_DLL CCTransitionProgressInOut : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:

    static CCTransitionProgressInOut* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);
    virtual void sceneOrder();
    virtual void setupTransition();
};

class CC_DLL CCTransitionProgressOutIn : public CCTransitionProgress
{
    GEODE_FRIEND_MODIFY
public:

    static CCTransitionProgressOutIn* create(float t, CCScene* scene);
protected:
    virtual CCProgressTimer* progressTimerNodeWithRenderTexture(CCRenderTexture* texture);

};

// end of transition group
/// @}

NS_CC_END

#endif /* __CCTRANSITIONPROGRESS_H__ */


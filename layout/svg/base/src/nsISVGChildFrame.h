/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is
 * Crocodile Clips Ltd..
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __NS_ISVGCHILDFRAME_H__
#define __NS_ISVGCHILDFRAME_H__

#include "gfxRect.h"
#include "nsQueryFrame.h"
#include "nsRect.h"

class nsIFrame;
class nsRenderingContext;

struct gfxMatrix;
struct nsPoint;
class SVGBBox;

namespace mozilla {
class SVGAnimatedLengthList;
class SVGAnimatedNumberList;
class SVGLengthList;
class SVGNumberList;
class SVGUserUnitList;
}

/**
 * This class is not particularly well named. It is inherited by some, but
 * not all SVG frame classes that can be descendants of an
 * nsSVGOuterSVGFrame in the frame tree. Note specifically that SVG container
 * frames that do not inherit nsSVGDisplayContainerFrame do not inherit this
 * class (so that's classes that only inherit nsSVGContainerFrame).
 */
class nsISVGChildFrame : public nsQueryFrame
{
public:
  typedef mozilla::SVGAnimatedNumberList SVGAnimatedNumberList;
  typedef mozilla::SVGNumberList SVGNumberList;
  typedef mozilla::SVGAnimatedLengthList SVGAnimatedLengthList;
  typedef mozilla::SVGLengthList SVGLengthList;
  typedef mozilla::SVGUserUnitList SVGUserUnitList;

  NS_DECL_QUERYFRAME_TARGET(nsISVGChildFrame)

  // Paint this frame - aDirtyRect is the area being redrawn, in frame
  // offset pixel coordinates
  NS_IMETHOD PaintSVG(nsRenderingContext* aContext,
                      const nsIntRect *aDirtyRect)=0;

  // Check if this frame or children contain the given point,
  // specified in app units relative to the origin of the outer
  // svg frame (origin ill-defined in the case of borders - bug
  // 290770).  See bug 290852 for foreignObject complications.
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint)=0;

  // Get bounds in our gfxContext's coordinates space (in app units)
  NS_IMETHOD_(nsRect) GetCoveredRegion()=0;

  // Called on SVG child frames (except NS_STATE_SVG_NONDISPLAY_CHILD frames)
  // to update and then invalidate their cached bounds. This method is not
  // called until after the nsSVGOuterSVGFrame has had its initial reflow
  // (i.e. once the SVG viewport dimensions are known). It should also only
  // be called by nsSVGOuterSVGFrame during its reflow.
  virtual void UpdateBounds()=0;

  // Flags to pass to NotifySVGChange:
  //
  // DO_NOT_NOTIFY_RENDERING_OBSERVERS - this should only be used when
  //                           updating the descendant frames of a clipPath,
  //                           mask, pattern or marker frame (or other similar
  //                           NS_STATE_SVG_NONDISPLAY_CHILD frame) immediately
  //                           prior to painting that frame's descendants.
  // TRANSFORM_CHANGED     - the current transform matrix for this frame has changed
  // COORD_CONTEXT_CHANGED - the dimensions of this frame's coordinate context has
  //                           changed (percentage lengths must be reevaluated)
  enum SVGChangedFlags {
    DO_NOT_NOTIFY_RENDERING_OBSERVERS = 0x01,
    TRANSFORM_CHANGED     = 0x02,
    COORD_CONTEXT_CHANGED = 0x04,
    FULL_ZOOM_CHANGED     = 0x08
  };
  virtual void NotifySVGChanged(PRUint32 aFlags)=0;

  /**
   * Get this frame's contribution to the rect returned by a GetBBox() call
   * that occurred either on this element, or on one of its ancestors.
   *
   * SVG defines an element's bbox to be the element's fill bounds in the
   * userspace established by that element. By allowing callers to pass in the
   * transform from the userspace established by this element to the userspace
   * established by an an ancestor, this method allows callers to obtain this
   * element's fill bounds in the userspace established by that ancestor
   * instead. In that case, since we return the bounds in a different userspace
   * (the ancestor's), the bounds we return are not this element's bbox, but
   * rather this element's contribution to the bbox of the ancestor.
   *
   * @param aToBBoxUserspace The transform from the userspace established by
   *   this element to the userspace established by the ancestor on which
   *   getBBox was called. This will be the identity matrix if we are the
   *   element on which getBBox was called.
   *
   * @param aFlags Flags indicating whether, stroke, for example, should be
   *   included in the bbox calculation.
   */
  virtual SVGBBox GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
                                      PRUint32 aFlags) = 0;

  // Are we a container frame?
  NS_IMETHOD_(bool) IsDisplayContainer()=0;
};

#endif // __NS_ISVGCHILDFRAME_H__


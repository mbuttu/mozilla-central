/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * interface that provides scroll APIs implemented by scrollable frames
 */

#ifndef nsIScrollFrame_h___
#define nsIScrollFrame_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsPresContext.h"

#define NS_DEFAULT_VERTICAL_SCROLL_DISTANCE 3

class nsBoxLayoutState;
class nsIScrollPositionListener;
class nsIFrame;

/**
 * Interface for frames that are scrollable. This interface exposes
 * APIs for examining scroll state, observing changes to scroll state,
 * and triggering scrolling.
 */
class nsIScrollableFrame : public nsQueryFrame {
public:

  NS_DECL_QUERYFRAME_TARGET(nsIScrollableFrame)

  /**
   * Get the frame for the content that we are scrolling within
   * this scrollable frame.
   */
  virtual nsIFrame* GetScrolledFrame() const = 0;

  typedef nsPresContext::ScrollbarStyles ScrollbarStyles;
  /**
   * Get the styles (NS_STYLE_OVERFLOW_SCROLL, NS_STYLE_OVERFLOW_HIDDEN,
   * or NS_STYLE_OVERFLOW_AUTO) governing the horizontal and vertical
   * scrollbars for this frame.
   */
  virtual ScrollbarStyles GetScrollbarStyles() const = 0;

  enum { HORIZONTAL = 0x01, VERTICAL = 0x02 };
  /**
   * Return the scrollbars which are visible. It's OK to call this during reflow
   * of the scrolled contents, in which case it will reflect the current
   * assumptions about scrollbar visibility.
   */
  virtual PRUint32 GetScrollbarVisibility() const = 0;
  /**
   * Return the actual sizes of all possible scrollbars. Returns 0 for scrollbar
   * positions that don't have a scrollbar or where the scrollbar is not visible.
   * Do not call this while this frame's descendants are being reflowed, it won't be
   * accurate.
   */
  virtual nsMargin GetActualScrollbarSizes() const = 0;
  /**
   * Return the sizes of all scrollbars assuming that any scrollbars that could
   * be visible due to overflowing content, are. This can be called during reflow
   * of the scrolled contents.
   */
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) = 0;
  /**
   * Return the sizes of all scrollbars assuming that any scrollbars that could
   * be visible due to overflowing content, are. This can be called during reflow
   * of the scrolled contents.
   */
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
                                            nsRenderingContext* aRC) = 0;

  /**
   * Get the area of the scrollport relative to the origin of this frame's
   * border-box.
   * This is the area of this frame minus border and scrollbars.
   */
  virtual nsRect GetScrollPortRect() const = 0;
  /**
   * Get the offset of the scrollport origin relative to the scrolled
   * frame origin. Typically the position will be non-negative.
   * This will always be a multiple of device pixels.
   */
  virtual nsPoint GetScrollPosition() const = 0;
  /**
   * Get the area that must contain the scroll position. Typically
   * (but not always, e.g. for RTL content) x and y will be 0, and
   * width or height will be nonzero if the content can be scrolled in
   * that direction. Since scroll positions must be a multiple of
   * device pixels, the range extrema will also be a multiple of
   * device pixels.
   */
  virtual nsRect GetScrollRange() const = 0;
  /**
   * Get the size of the scroll port to use when clamping the scroll
   * position.
   */
  virtual nsSize GetScrollPositionClampingScrollPortSize() const = 0;

  /**
   * Return how much we would try to scroll by in each direction if
   * asked to scroll by one "line" vertically and horizontally.
   */
  virtual nsSize GetLineScrollAmount() const = 0;
  /**
   * Return how much we would try to scroll by in each direction if
   * asked to scroll by one "page" vertically and horizontally.
   */
  virtual nsSize GetPageScrollAmount() const = 0;

  /**
   * When a scroll operation is requested, we ask for instant, smooth or normal
   * scrolling. SMOOTH will only be smooth if smooth scrolling is actually
   * enabled. INSTANT is always synchronous, NORMAL can be asynchronous.
   * If an INSTANT request happens while a smooth or async scroll is already in
   * progress, the async scroll is interrupted and we instantly scroll to the
   * destination.
   */
  enum ScrollMode { INSTANT, SMOOTH, NORMAL };
  /**
   * Clamps aScrollPosition to GetScrollRange and sets the scroll position
   * to that value.
   * @param aRange If non-null, specifies area which contains aScrollPosition
   * and can be used for choosing a performance-optimized scroll position.
   * Any point within this area can be chosen.
   * The choosen point will be as close as possible to aScrollPosition.
   */
  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode,
                        const nsRect* aRange = nullptr) = 0;
  /**
   * Scrolls to a particular position in integer CSS pixels.
   * Keeps the exact current horizontal or vertical position if the current
   * position, rounded to CSS pixels, matches aScrollPosition. If
   * aScrollPosition.x/y is different from the current CSS pixel position,
   * makes sure we only move in the direction given by the difference.
   * The scroll mode is INSTANT.
   */
  virtual void ScrollToCSSPixels(nsIntPoint aScrollPosition) = 0;
  /**
   * When scrolling by a relative amount, we can choose various units.
   */
  enum ScrollUnit { DEVICE_PIXELS, LINES, PAGES, WHOLE };
  /**
   * Modifies the current scroll position by aDelta units given by aUnit,
   * clamping it to GetScrollRange. If WHOLE is specified as the unit,
   * content is scrolled all the way in the direction(s) given by aDelta.
   * @param aOverflow if non-null, returns the amount that scrolling
   * was clamped by in each direction (how far we moved the scroll position
   * to bring it back into the legal range). This is never negative. The
   * values are in device pixels.
   */
  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow = nullptr, nsIAtom *aOrigin = nullptr) = 0;
  /**
   * This tells the scroll frame to try scrolling to the scroll
   * position that was restored from the history. This must be called
   * at least once after state has been restored. It is called by the
   * scrolled frame itself during reflow, but sometimes state can be
   * restored after reflows are done...
   * XXX should we take an aMode parameter here? Currently it's instant.
   */
  virtual void ScrollToRestoredPosition() = 0;

  /**
   * Add a scroll position listener. This listener must be removed
   * before it is destroyed.
   */
  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) = 0;
  /**
   * Remove a scroll position listener.
   */
  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) = 0;

  /**
   * Obtain the XUL box for the horizontal or vertical scrollbar, or null
   * if there is no such box. Avoid using this, but may be useful for
   * setting up a scrollbar mediator if you want to redirect scrollbar
   * input.
   */
  virtual nsIFrame* GetScrollbarBox(bool aVertical) = 0;

  /**
   * Internal method used by scrollbars to notify their scrolling
   * container of changes.
   */
  virtual void CurPosAttributeChanged(nsIContent* aChild) = 0;

  /**
   * Allows the docshell to request that the scroll frame post an event
   * after being restored from history.
   */
  NS_IMETHOD PostScrolledAreaEventForCurrentArea() = 0;

  /**
   * Returns true if this scrollframe is being "actively scrolled".
   * This basically means that we should allocate resources in the
   * expectation that scrolling is going to happen.
   */
  virtual bool IsScrollingActive() = 0;
};

#endif

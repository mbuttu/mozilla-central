/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;

import java.util.HashMap;

public class DoorHangerPopup extends PopupWindow implements GeckoEventListener {
    private static final String LOGTAG = "GeckoDoorHangerPopup";

    private GeckoApp mActivity;
    private View mAnchor;
    private LinearLayout mContent;

    private boolean mInflated; 
    private ImageView mArrow;
    private int mArrowWidth;

    DoorHangerPopup(GeckoApp aActivity, View aAnchor) {
        super(aActivity);
        mActivity = aActivity;
        mAnchor = aAnchor;

        mInflated = false;
        mArrowWidth = aActivity.getResources().getDimensionPixelSize(R.dimen.doorhanger_arrow_width);

        GeckoAppShell.registerGeckoEventListener("Doorhanger:Add", this);
        GeckoAppShell.registerGeckoEventListener("Doorhanger:Remove", this);
    }

    void destroy() {
        GeckoAppShell.unregisterGeckoEventListener("Doorhanger:Add", this);
        GeckoAppShell.unregisterGeckoEventListener("Doorhanger:Remove", this);
    }

    public void handleMessage(String event, JSONObject geckoObject) {
        try {
            if (event.equals("Doorhanger:Add")) {
                final String message = geckoObject.getString("message");
                final String value = geckoObject.getString("value");
                final JSONArray buttons = geckoObject.getJSONArray("buttons");
                final int tabId = geckoObject.getInt("tabID");
                final JSONObject options = geckoObject.getJSONObject("options");

                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        Tab tab = Tabs.getInstance().getTab(tabId);
                        if (tab != null)
                            addDoorHanger(message, value, buttons, tab, options);
                    }
                });
            } else if (event.equals("Doorhanger:Remove")) {
                final String value = geckoObject.getString("value");
                final int tabId = geckoObject.getInt("tabID");

                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        Tab tab = Tabs.getInstance().getTab(tabId);
                        if (tab == null)
                            return;
                        tab.removeDoorHanger(value);
                        updatePopup();
                    }
                });
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    private void init() {
        setBackgroundDrawable(new BitmapDrawable());
        setOutsideTouchable(true);
        setFocusable(true);
        setWindowLayoutMode(mActivity.isTablet() ? ViewGroup.LayoutParams.WRAP_CONTENT : ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        LayoutInflater inflater = LayoutInflater.from(mActivity);
        RelativeLayout layout = (RelativeLayout) inflater.inflate(R.layout.doorhangerpopup, null);
        mArrow = (ImageView) layout.findViewById(R.id.doorhanger_arrow);
        mContent = (LinearLayout) layout.findViewById(R.id.doorhanger_container);
        
        setContentView(layout);
        mInflated = true;
    }

    private void addDoorHanger(String message, String value, JSONArray buttons,
                               Tab tab, JSONObject options) {
        if (!mInflated)
            init();

        // Replace the doorhanger if it already exists
        DoorHanger dh = tab.getDoorHanger(value);
        if (dh != null) {
            tab.removeDoorHanger(value);
        }
        dh = new DoorHanger(mContent.getContext(), value);
 
        // Set the doorhanger text and buttons
        dh.setText(message);
        for (int i = 0; i < buttons.length(); i++) {
            try {
                JSONObject buttonObject = buttons.getJSONObject(i);
                String label = buttonObject.getString("label");
                int callBackId = buttonObject.getInt("callback");
                dh.addButton(label, callBackId);
            } catch (JSONException e) {
                Log.i(LOGTAG, "JSON throws", e);
            }
         }
        dh.setOptions(options);

        dh.setTab(tab);
        tab.addDoorHanger(value, dh);
        mContent.addView(dh);

        // Only update the popup if we're adding a notifcation to the selected tab
        if (tab.equals(Tabs.getInstance().getSelectedTab()))
            updatePopup();
    }

    public void updatePopup() {
        Tab tab = Tabs.getInstance().getSelectedTab();
        if (tab == null) {
            dismiss();
            return;
        }
        
        Log.i(LOGTAG, "Showing all doorhangers for tab: " + tab.getId());
 
        HashMap<String, DoorHanger> doorHangers = tab.getDoorHangers();
        // Hide the popup if there aren't any doorhangers to show
        if (doorHangers == null || doorHangers.size() == 0) {
            dismiss();
            return;
        }

        if (!mInflated)
            init();

        // Hide old doorhangers
        for (int i = 0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            dh.hide();
        }

        // Show the doorhangers for the tab
        for (DoorHanger dh : doorHangers.values()) {
            dh.show();
        }


        fixBackgroundForFirst();
        if (isShowing()) {
            update();
            return;
        }

        // If there's no anchor, just show the popup at the top of the gecko app view.
        if (mAnchor == null) {
            showAtLocation(mActivity.getView(), Gravity.TOP, 0, 0);
            return;
        }

        // On tablets, we need to position the popup so that the center of the arrow points to the
        // center of the anchor view. On phones the popup stretches across the entire screen, so the
        // arrow position is determined by its left margin.
        int offset = mActivity.isTablet() ? mAnchor.getWidth()/2 - mArrowWidth/2 -
                     ((RelativeLayout.LayoutParams) mArrow.getLayoutParams()).leftMargin : 0;
        showAsDropDown(mAnchor, offset, 0);
    }

    private void fixBackgroundForFirst() {
        for (int i=0; i < mContent.getChildCount(); i++) {
            DoorHanger dh = (DoorHanger) mContent.getChildAt(i);
            if (dh.isVisible()) {
                dh.setBackgroundResource(R.drawable.doorhanger_bg);
                break;
            }
        }
    }
}

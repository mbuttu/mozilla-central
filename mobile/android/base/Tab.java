/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.mozglue.DirectBufferAllocator;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.ContentResolver;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.util.Log;
import android.view.View;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class Tab {
    private static final String LOGTAG = "GeckoTab";

    private static Pattern sColorPattern;
    private int mId;
    private String mUrl;
    private String mTitle;
    private Drawable mFavicon;
    private String mFaviconUrl;
    private int mFaviconSize;
    private JSONObject mIdentityData;
    private boolean mReaderEnabled;
    private BitmapDrawable mThumbnail;
    private int mHistoryIndex;
    private int mHistorySize;
    private int mParentId;
    private boolean mExternal;
    private boolean mBookmark;
    private boolean mReadingListItem;
    private long mFaviconLoadId;
    private String mDocumentURI;
    private String mContentType;
    private boolean mHasTouchListeners;
    private boolean mAllowZoom;
    private float mDefaultZoom;
    private float mMinZoom;
    private float mMaxZoom;
    private ArrayList<View> mPluginViews;
    private HashMap<Object, Layer> mPluginLayers;
    private ContentResolver mContentResolver;
    private ContentObserver mContentObserver;
    private int mCheckerboardColor = Color.WHITE;
    private int mState;
    private ByteBuffer mThumbnailBuffer;
    private Bitmap mThumbnailBitmap;
    private boolean mDesktopMode;

    public static final int STATE_DELAYED = 0;
    public static final int STATE_LOADING = 1;
    public static final int STATE_SUCCESS = 2;
    public static final int STATE_ERROR = 3;

    public Tab(int id, String url, boolean external, int parentId, String title) {
        mId = id;
        mUrl = url;
        mExternal = external;
        mParentId = parentId;
        mTitle = title;
        mFavicon = null;
        mFaviconUrl = null;
        mFaviconSize = 0;
        mIdentityData = null;
        mReaderEnabled = false;
        mThumbnail = null;
        mHistoryIndex = -1;
        mHistorySize = 0;
        mBookmark = false;
        mReadingListItem = false;
        mFaviconLoadId = 0;
        mDocumentURI = "";
        mContentType = "";
        mPluginViews = new ArrayList<View>();
        mPluginLayers = new HashMap<Object, Layer>();
        mState = "about:home".equals(url) ? STATE_SUCCESS : STATE_LOADING;
        mContentResolver = Tabs.getInstance().getContentResolver();
        mContentObserver = new ContentObserver(GeckoAppShell.getHandler()) {
            public void onChange(boolean selfChange) {
                updateBookmark();
            }
        };
        BrowserDB.registerBookmarkObserver(mContentResolver, mContentObserver);
    }

    public void onDestroy() {
        BrowserDB.unregisterContentObserver(mContentResolver, mContentObserver);
    }

    public int getId() {
        return mId;
    }

    public int getParentId() {
        return mParentId;
    }

    // may be null if user-entered query hasn't yet been resolved to a URI
    public String getURL() {
        return mUrl;
    }

    public String getDisplayTitle() {
        if (mTitle != null && mTitle.length() > 0) {
            return mTitle;
        }

        return mUrl;
    }

    public Drawable getFavicon() {
        return mFavicon;
    }

    public Drawable getThumbnail() {
        return mThumbnail;
    }

    synchronized public ByteBuffer getThumbnailBuffer() {
        int capacity = getThumbnailWidth() * getThumbnailHeight() * 2 /* 16 bpp */;
        if (mThumbnailBuffer != null && mThumbnailBuffer.capacity() == capacity)
            return mThumbnailBuffer;
        freeBuffer();
        mThumbnailBuffer = DirectBufferAllocator.allocate(capacity);
        return mThumbnailBuffer;
    }

    public Bitmap getThumbnailBitmap() {
        if (mThumbnailBitmap != null)
            return mThumbnailBitmap;
        return mThumbnailBitmap = Bitmap.createBitmap(getThumbnailWidth(), getThumbnailHeight(), Bitmap.Config.RGB_565);
    }

    public void finalize() {
        freeBuffer();
    }

    synchronized void freeBuffer() {
        DirectBufferAllocator.free(mThumbnailBuffer);
        mThumbnailBuffer = null;
    }

    int getThumbnailWidth() {
        return (int) (GeckoApp.mAppContext.getResources().getDimension(R.dimen.tab_thumbnail_width));
    }

    int getThumbnailHeight() {
        return (int) (GeckoApp.mAppContext.getResources().getDimension(R.dimen.tab_thumbnail_height));
    }

    public void updateThumbnail(final Bitmap b) {
        final Tab tab = this;
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                if (b != null) {
                    try {
                        mThumbnail = new BitmapDrawable(b);
                        if (mState == Tab.STATE_SUCCESS)
                            saveThumbnailToDB();
                    } catch (OutOfMemoryError oom) {
                        Log.e(LOGTAG, "Unable to create/scale bitmap", oom);
                        mThumbnail = null;
                    }
                } else {
                    mThumbnail = null;
                }
                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.THUMBNAIL);
                    }
                });
            }
        });
    }

    public String getFaviconURL() {
        return mFaviconUrl;
    }

    public String getSecurityMode() {
        try {
            return mIdentityData.getString("mode");
        } catch (Exception e) {
            // If mIdentityData is null, or we get a JSONException
            return SiteIdentityPopup.UNKNOWN;
        }
    }

    public JSONObject getIdentityData() {
        return mIdentityData;
    }

    public boolean getReaderEnabled() {
        return mReaderEnabled;
    }

    public boolean isBookmark() {
        return mBookmark;
    }

    public boolean isReadingListItem() {
        return mReadingListItem;
    }

    public boolean isExternal() {
        return mExternal;
    }

    public void updateURL(String url) {
        if (url != null && url.length() > 0) {
            mUrl = url;
            Log.i(LOGTAG, "Updated url: " + url + " for tab with id: " + mId);
            updateBookmark();
            updateHistory(mUrl, mTitle);
        }
    }

    public void setDocumentURI(String documentURI) {
        mDocumentURI = documentURI;
    }

    public String getDocumentURI() {
        return mDocumentURI;
    }

    public void setContentType(String contentType) {
        mContentType = contentType;
    }

    public String getContentType() {
        return mContentType;
    }

    public void updateTitle(String title) {
        mTitle = (title == null ? "" : title);

        Log.i(LOGTAG, "Updated title: " + mTitle + " for tab with id: " + mId);
        updateHistory(mUrl, mTitle);
        final Tab tab = this;

        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(tab, Tabs.TabEvents.TITLE);
            }
        });
    }

    private void updateHistory(final String uri, final String title) {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                GlobalHistory.getInstance().update(uri, title);
            }
        });
    }

    public void setState(int state) {
        mState = state;
    }

    public int getState() {
        return mState;
    }

    public void setAllowZoom(boolean aValue) {
        mAllowZoom = aValue;
    }

    public boolean getAllowZoom() {
        return mAllowZoom;
    }

    public void setDefaultZoom(float aValue) {
        mDefaultZoom = aValue;
    }

    public float getDefaultZoom() {
        return mDefaultZoom;
    }

    public void setMinZoom(float aValue) {
        mMinZoom = aValue;
    }

    public float getMinZoom() {
        return mMinZoom;
    }

    public void setMaxZoom(float aValue) {
        mMaxZoom = aValue;
    }

    public float getMaxZoom() {
        return mMaxZoom;
    }

    public void setHasTouchListeners(boolean aValue) {
        mHasTouchListeners = aValue;
    }

    public boolean getHasTouchListeners() {
        return mHasTouchListeners;
    }

    public void setFaviconLoadId(long faviconLoadId) {
        mFaviconLoadId = faviconLoadId;
    }

    public long getFaviconLoadId() {
        return mFaviconLoadId;
    }

    public void updateFavicon(Drawable favicon) {
        mFavicon = favicon;
        Log.i(LOGTAG, "Updated favicon for tab with id: " + mId);
    }

    public void updateFaviconURL(String faviconUrl, int size) {
        // If we already have an "any" sized icon, don't update the icon.
        if (mFaviconSize == -1)
            return;

        // Only update the favicon if it's bigger than the current favicon.
        // We use -1 to represent icons with sizes="any".
        if (size == -1 || size >= mFaviconSize) {
            mFaviconUrl = faviconUrl;
            mFaviconSize = size;
            Log.i(LOGTAG, "Updated favicon URL for tab with id: " + mId);
        }
    }

    public void clearFavicon() {
        mFavicon = null;
        mFaviconUrl = null;
        mFaviconSize = 0;
    }


    public void updateIdentityData(JSONObject identityData) {
        mIdentityData = identityData;
    }

    public void setReaderEnabled(boolean readerEnabled) {
        mReaderEnabled = readerEnabled;
        GeckoAppShell.getMainHandler().post(new Runnable() {
            public void run() {
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.MENU_UPDATED);
            }
        });
    }

    private void updateBookmark() {
        final String url = getURL();
        if (url == null)
            return;

        (new GeckoAsyncTask<Void, Void, Void>() {
            @Override
            public Void doInBackground(Void... params) {
                if (url.equals(getURL())) {
                    mBookmark = BrowserDB.isBookmark(mContentResolver, url);
                    mReadingListItem = BrowserDB.isReadingListItem(mContentResolver, url);
                }
                return null;
            }

            @Override
            public void onPostExecute(Void result) {
                Tabs.getInstance().notifyListeners(Tab.this, Tabs.TabEvents.MENU_UPDATED);
            }
        }).execute();
    }

    public void addBookmark() {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                String url = getURL();
                if (url == null)
                    return;

                BrowserDB.addBookmark(mContentResolver, mTitle, url);
            }
        });
    }

    public void removeBookmark() {
        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                String url = getURL();
                if (url == null)
                    return;

                BrowserDB.removeBookmarksWithURL(mContentResolver, url);
            }
        });
    }

    public void addToReadingList() {
        if (!mReaderEnabled)
            return;

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Reader:Add", String.valueOf(getId()));
        GeckoAppShell.sendEventToGecko(e);
    }

    public void removeFromReadingList() {
        if (!mReaderEnabled)
            return;

        GeckoAppShell.getHandler().post(new Runnable() {
            public void run() {
                String url = getURL();
                if (url == null)
                    return;

                BrowserDB.removeReadingListItemWithURL(mContentResolver, url);

                GeckoApp.mAppContext.mMainHandler.post(new Runnable() {
                    public void run() {
                        GeckoEvent e = GeckoEvent.createBroadcastEvent("Reader:Remove", getURL());
                        GeckoAppShell.sendEventToGecko(e);
                    }
                });
            }
        });
    }

    public void readerMode() {
        if (!mReaderEnabled)
            return;

        GeckoApp.mAppContext.loadUrl("about:reader?url=" + Uri.encode(getURL()));
    }

    public void doReload() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Reload", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    // Our version of nsSHistory::GetCanGoBack
    public boolean canDoBack() {
        return mHistoryIndex > 0;
    }

    public boolean doBack() {
        if (!canDoBack())
            return false;

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Back", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    public void doStop() {
        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Stop", "");
        GeckoAppShell.sendEventToGecko(e);
    }

    // Our version of nsSHistory::GetCanGoForward
    public boolean canDoForward() {
        return mHistoryIndex < mHistorySize - 1;
    }

    public boolean doForward() {
        if (!canDoForward())
            return false;

        GeckoEvent e = GeckoEvent.createBroadcastEvent("Session:Forward", "");
        GeckoAppShell.sendEventToGecko(e);
        return true;
    }

    void handleSessionHistoryMessage(String event, JSONObject message) throws JSONException {
        if (event.equals("New")) {
            final String url = message.getString("url");
            mHistoryIndex++;
            mHistorySize = mHistoryIndex + 1;
            GeckoAppShell.getHandler().post(new Runnable() {
                public void run() {
                    GlobalHistory.getInstance().add(url);
                }
            });
        } else if (event.equals("Back")) {
            if (!canDoBack()) {
                Log.e(LOGTAG, "Received unexpected back notification");
                return;
            }
            mHistoryIndex--;
        } else if (event.equals("Forward")) {
            if (!canDoForward()) {
                Log.e(LOGTAG, "Received unexpected forward notification");
                return;
            }
            mHistoryIndex++;
        } else if (event.equals("Goto")) {
            int index = message.getInt("index");
            if (index < 0 || index >= mHistorySize) {
                Log.e(LOGTAG, "Received unexpected history-goto notification");
                return;
            }
            mHistoryIndex = index;
        } else if (event.equals("Purge")) {
            int numEntries = message.getInt("numEntries");
            if (numEntries > mHistorySize) {
                Log.e(LOGTAG, "Received unexpectedly large number of history entries to purge");
                mHistoryIndex = -1;
                mHistorySize = 0;
                return;
            }

            mHistorySize -= numEntries;
            mHistoryIndex -= numEntries;

            // If we weren't at the last history entry, mHistoryIndex may have become too small
            if (mHistoryIndex < -1)
                 mHistoryIndex = -1;
        }
    }

    private void saveThumbnailToDB() {
        try {
            String url = getURL();
            if (url == null)
                return;

            BrowserDB.updateThumbnailForUrl(mContentResolver, url, mThumbnail);
        } catch (Exception e) {
            // ignore
        }
    }

    public void addPluginView(View view) {
        mPluginViews.add(view);
    }

    public void removePluginView(View view) {
        mPluginViews.remove(view);
    }

    public View[] getPluginViews() {
        return mPluginViews.toArray(new View[mPluginViews.size()]);
    }

    public void addPluginLayer(Object surfaceOrView, Layer layer) {
        mPluginLayers.put(surfaceOrView, layer);
    }

    public Layer getPluginLayer(Object surfaceOrView) {
        return mPluginLayers.get(surfaceOrView);
    }

    public Collection<Layer> getPluginLayers() {
        return mPluginLayers.values();
    }

    public Layer removePluginLayer(Object surfaceOrView) {
        return mPluginLayers.remove(surfaceOrView);
    }

    public int getCheckerboardColor() {
        return mCheckerboardColor;
    }

    /** Sets a new color for the checkerboard. */
    public void setCheckerboardColor(int color) {
        mCheckerboardColor = color;
    }

    /** Parses and sets a new color for the checkerboard. */
    public void setCheckerboardColor(String newColor) {
        setCheckerboardColor(parseColorFromGecko(newColor));
    }

    // Parses a color from an RGB triple of the form "rgb([0-9]+, [0-9]+, [0-9]+)". If the color
    // cannot be parsed, returns white.
    private static int parseColorFromGecko(String string) {
        if (sColorPattern == null) {
            sColorPattern = Pattern.compile("rgb\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)");
        }

        Matcher matcher = sColorPattern.matcher(string);
        if (!matcher.matches()) {
            return Color.WHITE;
        }

        int r = Integer.parseInt(matcher.group(1));
        int g = Integer.parseInt(matcher.group(2));
        int b = Integer.parseInt(matcher.group(3));
        return Color.rgb(r, g, b);
    }

    public void setDesktopMode(boolean enabled) {
        mDesktopMode = enabled;
    }

    public boolean getDesktopMode() {
        return mDesktopMode;
    }
}

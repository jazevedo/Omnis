package com.example.respirator;

import android.view.View;

public class InputCluster {
    View[] mViews;

    public InputCluster(View[] views) {
        mViews = views;
    }

    public void toggleVisibility(boolean[] visible) {
        if (visible.length == mViews.length) {
            for (int i = 0; i < mViews.length; i++) {
                mViews[i].setVisibility(visible[i] ? View.VISIBLE : View.GONE);
            }
        }
    }
}

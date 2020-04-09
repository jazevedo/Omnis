package com.example.respirator;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class UsbDisconnectBroadcastReceiver extends BroadcastReceiver {

    private MainActivity mActivity;

    public UsbDisconnectBroadcastReceiver(MainActivity activity) {
        mActivity = activity;
    }

    @Override
    public void onReceive(Context context, Intent intent) {

        mActivity.usbDisonnected();

    }
}

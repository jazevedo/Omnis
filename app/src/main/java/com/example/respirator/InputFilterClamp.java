package com.example.respirator;

import android.text.InputFilter;
import android.text.Spanned;

public class InputFilterClamp implements InputFilter {

    double min, max;
    int minInt, maxInt;

    boolean mIsInt;

    public InputFilterClamp(double min, double max) {
        this.min = min;
        this.max = max;
    }

    public InputFilterClamp(int min, int max) {
        this.minInt = min;
        this.maxInt = max;
        mIsInt = true;
    }

    @Override
    public CharSequence filter(CharSequence source, int start, int end, Spanned dest, int dstart, int dend) {
        String newVal = dest.toString().substring(0, dstart) + dest.toString().substring(dend);
        newVal = newVal.substring(0, dstart) + source.toString() + newVal.substring(dstart);

        try {
            if (mIsInt) {
                int value = Integer.parseInt(newVal);
                if (value >= minInt && value <= maxInt)
                    return null;
            }
            else {
                double value = Double.parseDouble(newVal);
                if (value >= min && value <= max)
                    return null;
            }
        }
        catch (NumberFormatException ex) {
        }
        return "";
    }
}

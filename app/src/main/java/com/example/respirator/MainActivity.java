package com.example.respirator;

import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;

public class MainActivity extends AppCompatActivity {

    private Spinner mModeSpinner;
    private Button mToggleTreatment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mModeSpinner = findViewById(R.id.mode_spinner);
        mToggleTreatment = findViewById(R.id.toggle_treatment);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    boolean mIsTreating = false;

    public void toggleTreatment(View view) {

        int colorRef;
        int textRef;
        if (!mIsTreating) {
            colorRef = R.color.colorAccent;
            textRef = R.string.begin_treatment;
        }
        else {
            colorRef = R.color.colorAccentInverse;
            textRef = R.string.stop_treatment;
        }

        /*
        int color = ResourcesCompat.getColor(getResources(), colorRef, null);
        String buttonText = getResources().getString(textRef);
        mToggleTreatment.setBackgroundColor(color);
        */
        mToggleTreatment.setBackgroundResource(colorRef);
        mToggleTreatment.setText(textRef);
        mIsTreating = !mIsTreating;
    }

}

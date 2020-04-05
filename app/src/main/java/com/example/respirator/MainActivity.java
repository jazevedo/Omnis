package com.example.respirator;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.Group;


public class MainActivity extends AppCompatActivity {

    private Spinner mModeSpinner;
    private Button mToggleTreatment;
    private Group mGroupIns;
    private Group mGroupOuts;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mModeSpinner = findViewById(R.id.mode_spinner);
        mToggleTreatment = findViewById(R.id.toggle_treatment);
        mGroupIns = findViewById(R.id.group_ins);
        mGroupOuts = findViewById(R.id.group_outs);
        mGroupOuts.setVisibility(View.GONE);
        findViewById(R.id.divider).setVisibility(View.GONE);


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
            mGroupIns.setVisibility(View.GONE);
            mGroupOuts.setVisibility(View.VISIBLE);
        }
        else {
            colorRef = R.color.colorAccentInverse;
            textRef = R.string.stop_treatment;
            mGroupIns.setVisibility(View.VISIBLE);
            mGroupOuts.setVisibility(View.GONE);
        }

        mToggleTreatment.setBackgroundResource(colorRef);
        mToggleTreatment.setText(textRef);
        mIsTreating = !mIsTreating;
    }

}

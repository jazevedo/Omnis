package com.example.respirator;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.Spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.Group;


public class MainActivity extends AppCompatActivity
    implements AdapterView.OnItemSelectedListener {

    public static final String tag = "Omnis";

    private Spinner mModeSpinner;
    private Button mToggleTreatment;
    private Group mGroupOuts;

    private Modes mCurrentMode;
    private InputCluster mCluster;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mModeSpinner = findViewById(R.id.mode_spinner);
        mToggleTreatment = findViewById(R.id.toggle_treatment);
        mGroupOuts = findViewById(R.id.group_outs);
        mGroupOuts.setVisibility(View.GONE);
        findViewById(R.id.divider).setVisibility(View.GONE);

        mCluster = new InputCluster(new View[] {
            findViewById(R.id.group_tidal_volume),
            findViewById(R.id.group_respiratory_rate),
            findViewById(R.id.group_insp_exp_ratio),
            findViewById(R.id.group_peep),
            findViewById(R.id.group_pressure_support)
        });

        mModeSpinner.setSelection(0);
        mModeSpinner.setOnItemSelectedListener(this);
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
        if (mIsTreating) {
            colorRef = R.color.colorAccent;
            textRef = R.string.begin_treatment;
            mModeSpinner.setEnabled(true);
            mModeSpinner.setClickable(true);
            toggleInputs();
            mGroupOuts.setVisibility(View.GONE);
        }
        else {
            colorRef = R.color.colorAccentInverse;
            textRef = R.string.stop_treatment;
            mCluster.toggleVisibility(new boolean[] { false, false, false, false, false });
            mGroupOuts.setVisibility(View.VISIBLE);
            mModeSpinner.setEnabled(false);
            mModeSpinner.setClickable(false);
        }

        mToggleTreatment.setBackgroundResource(colorRef);
        mToggleTreatment.setText(textRef);
        mIsTreating = !mIsTreating;
    }

    @Override
    public void onItemSelected(AdapterView<?> adapterView, View view, int position, long id) {

        Modes[] values = Modes.values();
        if (position >= 0 && position < values.length) {
            Modes mode = values[position];
            setMode(mode);
        }
    }

    void setMode(Modes mode) {
        mCurrentMode = mode;
        toggleInputs();
    }

    void toggleInputs() {
        switch (mCurrentMode) {
            case AssistControlPressureVentilation:
                mCluster.toggleVisibility(new boolean[] { false, true, true, true, true });
                break;
            case AssistControlVolumeVentilation:
                mCluster.toggleVisibility(new boolean[] { true, true, true, true, false });
                break;
            case ContinuousPositiveAirwayPressure:
                mCluster.toggleVisibility(new boolean[] { false, false, false, false, true });
                break;
        }
    }

    public void onNothingSelected(AdapterView<?> adapterView) {

    }
}

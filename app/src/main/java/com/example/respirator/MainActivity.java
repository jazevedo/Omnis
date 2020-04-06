package com.example.respirator;

import android.os.Bundle;
import android.text.InputFilter;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.Group;


public class MainActivity extends AppCompatActivity
        implements AdapterView.OnItemSelectedListener {

    public static final String tag = "Omnis";

    private final double TidalVolumeMin = 6.0;
    private final double TidalVolumeMax = 8.0;
    private final double PressureSupportMin = 8.0;
    private final double PressureSupportMax = 20.0;
    private final double PeepMin = 3.0;
    private final double PeepMax = 10.0;
    private final int RespiratoryRateMin = 10;
    private final int RespiratoryRateMax = 20;


    private Spinner mModeSpinner;
    private Button mToggleTreatment;
    private Group mGroupOuts;

    private Modes mCurrentMode;
    private InputCluster mCluster;
    private View mVolumeIn;
    private View mVolumeOut;
    private View mFlowIn;
    private View mFlowOut;
    private View mPressure;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mModeSpinner = findViewById(R.id.mode_spinner);
        mToggleTreatment = findViewById(R.id.toggle_treatment);
        mGroupOuts = findViewById(R.id.group_outs);
        mGroupOuts.setVisibility(View.GONE);
        findViewById(R.id.divider).setVisibility(View.GONE);

        EditText tidalVolume = findViewById(R.id.tidal_volume_text);
        tidalVolume.setText(Double.toString(TidalVolumeMin));
        tidalVolume.setFilters(new InputFilter[]{new InputFilterClamp(TidalVolumeMin, TidalVolumeMax)});

        EditText pressureSupport = findViewById(R.id.pressure_support_text);
        pressureSupport.setText(Double.toString(PressureSupportMin));
        pressureSupport.setFilters(new InputFilter[]{new InputFilterClamp(PressureSupportMin, PressureSupportMax)});

        EditText peep = findViewById(R.id.peep_text);
        peep.setText(Double.toString(PeepMin));
        peep.setFilters(new InputFilter[]{new InputFilterClamp(PeepMin, PeepMax)});

        EditText respiratoryRate = findViewById(R.id.respiratory_rate_text);
        respiratoryRate.setText(Integer.toString(RespiratoryRateMin));
        respiratoryRate.setFilters(new InputFilter[]{new InputFilterClamp(RespiratoryRateMin, RespiratoryRateMax)});

        mCluster = new InputCluster(new View[]{
                findViewById(R.id.group_tidal_volume),
                findViewById(R.id.group_respiratory_rate),
                findViewById(R.id.group_insp_exp_ratio),
                findViewById(R.id.group_peep),
                findViewById(R.id.group_pressure_support)
        });

        mModeSpinner.setSelection(0);
        mModeSpinner.setOnItemSelectedListener(this);

        mVolumeIn = findViewById(R.id.volume_in_text);
        mVolumeOut = findViewById(R.id.volume_out_text);
        mFlowIn = findViewById(R.id.flow_in_text);
        mFlowOut = findViewById(R.id.flow_out_text);
        mPressure = findViewById(R.id.pressure_text);
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
            mCluster.toggleVisibility(new boolean[]{false, false, false, false, false});
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
                mCluster.toggleVisibility(new boolean[]{false, true, true, true, true});
                break;
            case AssistControlVolumeVentilation:
                mCluster.toggleVisibility(new boolean[]{true, true, true, true, false});
                break;
            case ContinuousPositiveAirwayPressure:
                mCluster.toggleVisibility(new boolean[]{false, false, false, false, true});
                break;
        }
    }

    public void onNothingSelected(AdapterView<?> adapterView) {
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.tidal_volume_text:
                clamp((EditText) view, TidalVolumeMin, TidalVolumeMax);
                break;
        }
    }

    void clamp(EditText editText, double start, double stop) {
        String text = editText.getText().toString();
        double value = Double.parseDouble(text);
        if (value < start)
            editText.setText(Double.toString(start));
        else if (value > stop) {
            editText.setText(Double.toString(stop));
        }
    }
}

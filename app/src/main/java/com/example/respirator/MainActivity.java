package com.example.respirator;

import android.content.Context;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.text.InputFilter;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;
import androidx.constraintlayout.widget.Group;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.Executors;


public class MainActivity extends AppCompatActivity
        implements AdapterView.OnItemSelectedListener,
         SerialInputOutputManager.Listener {

    public static final String tag = "Omnis";

    private final double TidalVolumeMin = 1.0;
    private final double TidalVolumeMax = 1000.0;
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
    private TextView mVolumeIn;
    private TextView mVolumeOut;
    private TextView mFlowIn;
    private TextView mFlowOut;
    private TextView mPressure;
    private Button mToggleView;
    private TextView mConnectionIndicator;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.activity_main);

        mConnectionIndicator = findViewById(R.id.connectionIndicator);
        mModeSpinner = findViewById(R.id.mode_spinner);
        mToggleTreatment = findViewById(R.id.toggle_treatment);
        mToggleView = findViewById(R.id.toggle_view);
        mGroupOuts = findViewById(R.id.group_outs);
        mGroupOuts.setVisibility(View.GONE);
        findViewById(R.id.divider).setVisibility(View.GONE);

        EditText tidalVolume = findViewById(R.id.tidal_volume_text);
        tidalVolume.setText(Double.toString(TidalVolumeMin));
        //tidalVolume.setFilters(new InputFilter[]{new InputFilterClamp(TidalVolumeMin, TidalVolumeMax)});

        EditText pressureSupport = findViewById(R.id.pressure_support_text);
        pressureSupport.setText(Double.toString(PressureSupportMin));
        //pressureSupport.setFilters(new InputFilter[]{new InputFilterClamp(PressureSupportMin, PressureSupportMax)});

        EditText peep = findViewById(R.id.peep_text);
        peep.setText(Double.toString(PeepMin));
        //peep.setFilters(new InputFilter[]{new InputFilterClamp(PeepMin, PeepMax)});

        EditText respiratoryRate = findViewById(R.id.respiratory_rate_text);
        respiratoryRate.setText(Integer.toString(RespiratoryRateMin));
        //respiratoryRate.setFilters(new InputFilter[]{new InputFilterClamp(RespiratoryRateMin, RespiratoryRateMax)});

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

        try {
            onConnect();
        }
        catch (Exception ex) {
            ex.printStackTrace();
            mPort = null;
        }

        //setOutputs("10.0588,20.0400,30.0417,40.0769,50.3333");
    }

    @Override
    protected void onStop() {
        super.onStop();
        mIsTreating = false;
    }

    boolean mIsTreating = false;
    boolean mSettingsView = true;

    public void toggleTreatment(View view) {
        _toggleTreatment(true);
    }

    void _toggleTreatment(boolean commandArduino) {
        int colorRef;
        int textRef;
        if (mIsTreating) {
            passStandby();
            colorRef = R.color.colorAccent;
            textRef = R.string.begin_treatment;
            mToggleView.setVisibility(View.GONE);
            changeSettingsMode();
        }
        else {
            if (commandArduino) {
                passInputsToArduino();
            }
            colorRef = R.color.colorAccent;
            textRef = R.string.stop_treatment;
            mToggleView.setVisibility(View.VISIBLE);
            changeMonitorMode();
        }

        mToggleTreatment.setBackgroundResource(colorRef);
        mToggleTreatment.setText(textRef);
        mIsTreating = !mIsTreating;
    }

    void changeMonitorMode() {
        mCluster.toggleVisibility(new boolean[]{false, false, false, false, false});
        mGroupOuts.setVisibility(View.VISIBLE);
        mModeSpinner.setEnabled(false);
        mModeSpinner.setClickable(false);
        mToggleView.setText(R.string.view_settings);
        mSettingsView = false;
    }

    void changeSettingsMode() {
        mModeSpinner.setEnabled(true);
        mModeSpinner.setClickable(true);
        toggleInputs();
        mGroupOuts.setVisibility(View.GONE);
        mToggleView.setText(R.string.view_monitor);
        mSettingsView = true;
    }

    public void toggleView(View view) {
        if (mSettingsView) {
            changeMonitorMode();
        }
        else {
            changeSettingsMode();
        }
    }

    void passInputsToArduino() {
        EditText tidalVolumeText = findViewById(R.id.tidal_volume_text);
        EditText pressureSupportText = findViewById(R.id.pressure_support_text);
        EditText respiratoryRateText = findViewById(R.id.respiratory_rate_text);
        Spinner inspExpText = findViewById(R.id.inspiratory_expiratory_ratio_spinner);
        EditText peepText = findViewById(R.id.peep_text);

        double tidalVolume = Double.parseDouble(tidalVolumeText.getText().toString());
        double pressureSupport = Double.parseDouble(pressureSupportText.getText().toString());
        double respiratoryRate = Double.parseDouble(respiratoryRateText.getText().toString());
        String inspExp = inspExpText.getSelectedItem().toString();
        double peep = Double.parseDouble(peepText.getText().toString());

        char command = 0;
        switch (mCurrentMode) {
            case AssistControlPressureVentilation:
                command = 'p';
                break;
            case AssistControlVolumeVentilation:
                command = 'v';
                break;
            case ContinuousPositiveAirwayPressure:
                command = 'a';
                break;
        }

        String text = String.format("%c%f,%f,%f,%s,%f;",
                command,
                tidalVolume,
                pressureSupport,
                respiratoryRate,
                inspExp,
                peep);
        //Log.d("omnis", text);

        sendArduino(text);

    }

    void passStandby() {
        sendArduino("s");
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


    private UsbSerialPort mPort;

    public void onConnect() {
        mConnectionIndicator.setText("Connecting...");

        UsbManager manager = (UsbManager)getSystemService(Context.USB_SERVICE);
        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        if (availableDrivers.isEmpty()) {
            return;
        }

        // Open a connection to the first available driver.
        UsbSerialDriver driver = availableDrivers.get(0);
        UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
        if (connection == null) {
            // add UsbManager.requestPermission(driver.getDevice(), ..) handling here
            return;
        }

        connection.

        // Most devices have just one port (port 0)
        mPort = driver.getPorts().get(0);
        try {
            mPort.open(connection);

            mPort.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
            mConnectionIndicator.setText("Connected.");
        }
        catch (IOException ex) {
            ex.printStackTrace();
            mConnectionIndicator.setText(String.format("Failed: %s", ex.getMessage()));
        }

        SerialInputOutputManager usbIoManager = new SerialInputOutputManager(mPort, this);
        Executors.newSingleThreadExecutor().submit(usbIoManager);
    }


    public void sendArduino(String text) {
        if (mPort == null)
            return;

        try {
            mPort.write(text.getBytes(), 20);
        }
        catch (IOException ex) {
            ex.printStackTrace();
            mConnectionIndicator.setText(String.format("Error writing: %s", ex.getMessage()));
        }

    }

    String mBufferedRead = "";

    @Override
    public void onNewData(byte[] data) {
        String response = new String(data);
        mBufferedRead += response;

        if (mBufferedRead.contains("\r\n")) {

            runOnUiThread(() -> {
                Log.d(tag, "RECEIVED " + mBufferedRead);
                setOutputs(mBufferedRead);
                mBufferedRead = "";
                //mSerialText.append(response);
            });
        }
    }

    @Override
    public void onRunError(Exception e) {
        e.printStackTrace();
    }

    void setOutputs(String received) {

        String[] elts = received.split(",");

        if (elts.length != 5) {
            Log.d(tag, "setOutputs error, received " + Integer.toString(elts.length) + " elements from serial");
        }
        else {
            try {
                double volumeIn = Double.parseDouble(elts[0]);
                double volumeOut = Double.parseDouble(elts[1]);
                double flowIn = Double.parseDouble(elts[2]);
                double flowOut = Double.parseDouble(elts[3]);
                double pressure = Double.parseDouble(elts[4]);

                mVolumeIn.setText(Double.toString(volumeIn));
                mVolumeOut.setText(Double.toString(volumeOut));
                mFlowIn.setText(Double.toString(flowIn));
                mFlowOut.setText(Double.toString(flowOut));
                mPressure.setText(Double.toString(pressure));


                if (!mIsTreating) {
                    _toggleTreatment(false);
                }
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }

    }
}

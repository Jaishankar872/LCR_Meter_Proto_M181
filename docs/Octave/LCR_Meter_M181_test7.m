pkg load instrument-control  % Load serial package (install if needed)
pkg load signal              % For filter application

% Clear previous serial connections and variables
clc;
clear;
clear s;
close all;  % Close any open figure

%%----------------------------------------------------------------------------------%%
%%--UART Data Configuration--%%
%%----------------------------------------------------------------------------------%%
port = "COM11";           % Manual Update Before Running the Code
baudrate = 115200;
s = serialport(port, baudrate);
configureTerminator(s, "LF");  % Use Line Feed as end-of-line
flush(s); % Clear old data
pause(2);  % Allow time for connection to stabilize

%%----------------------------------------------------------------------------------%%
%%--MCU ADC Data Parameter--%%
%%----------------------------------------------------------------------------------%%
LCR_measure_freq = 1000;   % Excitation frequency in Hz
LCR_ADC_sample_rate = 64;  % Number of ADC samples per period
signal_sample_count_n = 128; % Number of samples to store per update
ADC_Ref = 3.3;             % Reference voltage for ADC
ADC_Bit_size = 12;         % ADC resolution in bits

% Calculate ADC resolution and sample timing
ADC_Resolution = 2^ADC_Bit_size;
per_sample_time = 1 / LCR_measure_freq;  % One period in seconds
per_sample_time = per_sample_time / LCR_ADC_sample_rate;
sampling_freq = 1 / per_sample_time;       % Hz

%%----------------------------------------------------------------------------------%%
%% Step 2: Prepare Data Buffer and Time Scale for Plotting
%%----------------------------------------------------------------------------------%%
time_window = (signal_sample_count_n - 1) * per_sample_time;
raw_data = zeros(signal_sample_count_n, 8);
% Columns: 1: Voltage ADC, 2: Voltage AFC, 3: Current ADC, 4: Current AFC,
%          5-8: Additional channels (e.g., zero crossing signals)

time_scale = linspace(0, time_window, signal_sample_count_n);

%%----------------------------------------------------------------------------------%%
%% Step 2.5: Generate Square Wave Reference Signals (0° and 90°)
%%----------------------------------------------------------------------------------%%
ref0 = sign(sin(2*pi*LCR_measure_freq*time_scale));    % 0° reference square wave (+1 / -1)
ref90 = sign(cos(2*pi*LCR_measure_freq*time_scale));     % 90° reference square wave

%%----------------------------------------------------------------------------------%%
%% Step 3: Create Figure for Time-Domain Display and Demodulation Results
%%----------------------------------------------------------------------------------%%
fig1 = figure("Name", "ADC Data & Synchronous Demodulation");

% Subplot for Voltage Data
subplot(2,2,1);
hold on;
hVolt = plot(time_scale, raw_data(:,1), "r-");
hVoltAFC = plot(time_scale, raw_data(:,2), "g-");
hold off;
title("Voltage Data ADC & AFC");
xlabel("Time (s)");
ylabel("Voltage (V)");
grid on;
legend("ADC", "AFC");

% Subplot for Current Data
subplot(2,2,2);
hold on;
hCurr = plot(time_scale, raw_data(:,3), "b-");
hCurrAFC = plot(time_scale, raw_data(:,4), "m-");
hold off;
title("Current Data ADC & AFC");
xlabel("Time (s)");
ylabel("Current (mA)");
grid on;
legend("ADC", "AFC");

% Subplot for Demodulated Phase (Voltage)
subplot(2,2,3);
phase_text_volt = text(0.1, 0.5, "Voltage Phase: Waiting...", 'FontSize', 12);
title("Synchronous Demodulation (Voltage)");
xlabel("Time (s)");
ylabel("Phase (deg)");
axis off;

% Subplot for Demodulated Phase (Current)
subplot(2,2,4);
phase_text_curr = text(0.1, 0.5, "Current Phase: Waiting...", 'FontSize', 12);
phase_text_total = text(0.1, 0.8, "Current Phase: Waiting...", 'FontSize', 12);
title("Synchronous Demodulation (Current)");
xlabel("Time (s)");
ylabel("Phase (deg)");
axis off;

%%----------------------------------------------------------------------------------%%
%% Step 4: Serial Data Acquisition, Processing, and Synchronous Demodulation
%%----------------------------------------------------------------------------------%%
% Preallocate arrays for multiplication products for four channels
I_product = zeros(signal_sample_count_n, 4);  % For in-phase (0°)
Q_product = zeros(signal_sample_count_n, 4);  % For quadrature (90°)

while true
    try
        % Read one line of data from serial port
        line = readline(s);
        values = str2double(strsplit(line, ","));  % Expect CSV string

        % Check if data format is correct (expecting 9 values: index + 8 ADC channels)
        if length(values) == 9
            % Remove the first element (index) and update raw_data buffer (FIFO)
            raw_data = [raw_data(2:end, :); values(2:9)];
            % Convert ADC counts to voltage
            data = (raw_data * ADC_Ref) / ADC_Resolution;

            % Remove DC offset for AC coupling (for each channel)
            AC_data = zeros(size(data));
            for ch = 1:8
                AC_data(:,ch) = data(:,ch) - mean(data(:,ch));
            end
            % Apply Low-pass filter to remove DC offset and noise
##            cutoff_freq = LCR_measure_freq*1.2;  % Adjust cutoff frequency as needed
##            [b, a] = butter(2, cutoff_freq / (sampling_freq / 2), 'low');
##            filtered_data = filtfilt(b, a, AC_data);

            % Apply the Low-Pass Filter
            alpha = 0.15;  % Smoothing factor (adjust between 0 and 1)
            for ch = 1:8
              filtered_data(1,ch) = AC_data(1,ch);  % Initialize first value
              for i = 2:signal_sample_count_n
                filtered_data(i,ch) = alpha * AC_data(i,ch) + (1 - alpha) * filtered_data(i - 1, ch);
              end
            end

##            filtered_data = sgolayfilt(AC_data, 3, 11);

            % Optionally, you can AC coupling the data if needed (disabled here)
            filtered_data = AC_data;

            Volt_Gain_B = 0;
            Amp_Gain_B = 1;
            % Update time-domain plots
            if Volt_Gain_B == 0
                set(hVolt, "YData", filtered_data(:,1));
                set(hVoltAFC, "YData", filtered_data(:,2));
            else
                set(hVolt, "YData", filtered_data(:,5));
                set(hVoltAFC, "YData", filtered_data(:,6));
            end

            if Amp_Gain_B == 0
                set(hCurr, "YData", filtered_data(:,3));
                set(hCurrAFC, "YData", filtered_data(:,4));
            else
                set(hCurr, "YData", filtered_data(:,7));
                set(hCurrAFC, "YData", filtered_data(:,8));
            end

            %% Synchronous Demodulation using Square-Wave Multiplication
            % For each channel (1: Voltage ADC, 2: Voltage AFC, 3: Current ADC, 4: Current AFC)
            phase_est = zeros(1,4);
            for ch = 1:8
                % Multiply by square wave references
                I_product(:,ch) = filtered_data(:,ch) .* ref0(:);
                Q_product(:,ch) = filtered_data(:,ch) .* ref90(:);
                % Average over the sample period
                I_avg = mean(I_product(:,ch));
                Q_avg = mean(Q_product(:,ch));
                % Compensate scaling: for square-wave multiplication, the DC term is (2/pi)*A*sin() or cos()
                % Multiply by (pi/2) to recover the amplitude scale.
                I_demod = I_avg * (pi/2);
                Q_demod = Q_avg * (pi/2);
                % Calculate phase (in degrees)
                phase_est(ch) = rad2deg(atan2(Q_demod, I_demod));
            end

            if Volt_Gain_B == 0
                voltage_phase = phase_est(2) - phase_est(1);
            else
                voltage_phase = phase_est(6) - phase_est(5);
            end
            voltage_phase = mod(voltage_phase + 180, 360) - 180;  % Wrap voltage phase

            if Amp_Gain_B == 0
                current_phase = phase_est(4) - phase_est(3);
            else
                current_phase = phase_est(8) - phase_est(7);
            end
            current_phase = current_phase - 180;
            current_phase = mod(current_phase + 180, 360) - 180;  % Wrap current phase

            % Calculate the difference between current and voltage phases
            VI_phase = abs(current_phase) - abs(voltage_phase);
            VI_phase = mod(VI_phase + 180, 360) - 180;  % Wrap overall phase difference

            % Update the demodulation text displays (for Voltage ADC and Current ADC)
            set(phase_text_volt, "String", sprintf("Voltage Phase: %.1f° || Eq= %.1f° - %.1f° (AFC - V)", voltage_phase, phase_est(2), phase_est(1)));
            set(phase_text_curr, "String", sprintf("Current Phase: %.1f° || Eq= |%.1f° - %.1f°| - 180° (AFC - I)", current_phase, phase_est(4),phase_est(3)));
            set(phase_text_total, "String", sprintf("VI Phase: %.1f°     || Eq= |%.1f°| - |%.1f°| (I - V)", VI_phase, current_phase,voltage_phase));

            drawnow;  % Refresh plots
        end
    catch err
        disp("Error reading data: " + err.message);
        break;
    end
end


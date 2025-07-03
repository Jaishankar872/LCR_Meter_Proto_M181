pkg load instrument-control  % Load serial package (install if needed)
pkg load signal              % For filter application

clc; clear; clear s; close all;

%% UART Data Configuration
port = "COM12";
baudrate = 115200;
s = serialport(port, baudrate);
configureTerminator(s, "CR/LF");
flush(s);
pause(2);

%% MCU ADC Data Parameter
LCR_measure_freq   = 1000;
LCR_ADC_sample_rate= 64;
signal_sample_count_n = 128;
ADC_Ref            = 3.3;
ADC_Bit_size       = 12;

ADC_Resolution = 2^ADC_Bit_size;
per_sample_time = 1 / (LCR_measure_freq * LCR_ADC_sample_rate);
sampling_freq   = 1 / per_sample_time;

%% Prepare buffers and time axis
time_window = (signal_sample_count_n - 1) * per_sample_time;
raw_data    = zeros(signal_sample_count_n, 8);
time_scale  = linspace(0, time_window, signal_sample_count_n);

%% Square‐wave references
ref0  = sign(sin(2*pi*LCR_measure_freq*time_scale));
ref90 = sign(cos(2*pi*LCR_measure_freq*time_scale));

%% Create Figure: 2 rows × 4 columns
fig1 = figure("Name","ADC Data & Synchronous Demodulation");

% Top row: Gain A Voltage
subplot(2,4,1); hold on;
hVA_A = plot(time_scale, raw_data(:,1), "r-");  % Volt ADC A
hVA_F = plot(time_scale, raw_data(:,2), "g-");  % Volt AFC A
hold off; grid on;
title("Volt (Gain A)");
xlabel("Time (s)"); ylabel("V"); legend("ADC","AFC");

% Top row: Gain A Current
subplot(2,4,2); hold on;
hCA_A = plot(time_scale, raw_data(:,3), "b-");  % Curr ADC A
hCA_F = plot(time_scale, raw_data(:,4), "m-");  % Curr AFC A
hold off; grid on;
title("Curr (Gain A)");
xlabel("Time (s)"); ylabel("mA"); legend("ADC","AFC");

% Top row: Gain B Voltage
subplot(2,4,3); hold on;
hVB_A = plot(time_scale, raw_data(:,5), "r-");  % Volt ADC B
hVB_F = plot(time_scale, raw_data(:,6), "g-");  % Volt AFC B
hold off; grid on;
title("Volt (Gain B)");
xlabel("Time (s)"); ylabel("V"); legend("ADC","AFC");

% Top row: Gain B Current
subplot(2,4,4); hold on;
hCB_A = plot(time_scale, raw_data(:,7), "b-");  % Curr ADC B
hCB_F = plot(time_scale, raw_data(:,8), "m-");  % Curr AFC B
hold off; grid on;
title("Curr (Gain B)");
xlabel("Time (s)"); ylabel("mA"); legend("ADC","AFC");

% Bottom row: Demodulation text panels
subplot(2,4,5); axis off;
txtVA = text(0.1,0.5,"Volt Phase A: Waiting...", "FontSize",12);
title("Sync Demod (Volt A)");

subplot(2,4,6); axis off;
txtCA = text(0.1,0.5,"Curr Phase A: Waiting...", "FontSize",12);
title("Sync Demod (Curr A)");

subplot(2,4,7); axis off;
txtVB = text(0.1,0.5,"Volt Phase B: Waiting...", "FontSize",12);
title("Sync Demod (Volt B)");

subplot(2,4,8); axis off;
txtCB = text(0.1,0.5,"Curr Phase B: Waiting...", "FontSize",12);
title("Sync Demod (Curr B)");

drawnow;

%% Main loop: read, filter, plot & demodulate
I_product = zeros(signal_sample_count_n, 8);
Q_product = zeros(signal_sample_count_n, 8);

alpha = 0.15;  % filter smoothing factor

while true
    try
        line   = readline(s);
        values = str2double(strsplit(strtrim(line),","));
        if numel(values)==9
            raw_data = [raw_data(2:end,:); values(2:9)];

            data     = (raw_data * ADC_Ref) / ADC_Resolution;

            % AC‐couple
            AC_data = data - mean(data);

            % Apply Low-pass filter to remove DC offset and noise
            cutoff_freq = LCR_measure_freq*1.2;  % Adjust cutoff frequency as needed
            [b, a] = butter(2, cutoff_freq / (sampling_freq / 2), 'low');
            filtered = filtfilt(b, a, AC_data);

##            % Simple LPF
##            filtered = zeros(size(AC_data));
##            filtered(1,:) = AC_data(1,:);
##            for ch=1:8
##                for i=2:signal_sample_count_n
##                    filtered(i,ch) = alpha*AC_data(i,ch) + (1-alpha)*filtered(i-1,ch);
##                end
##            end

            % Optionally, you can AC coupling the data if needed (disabled here)
            filtered = AC_data;

            % Update time‐plots
            set(hVA_A, "YData", filtered(:,1));
            set(hVA_F, "YData", filtered(:,2));
            set(hCA_A, "YData", filtered(:,3));
            set(hCA_F, "YData", filtered(:,4));

            set(hVB_A, "YData", filtered(:,5));
            set(hVB_F, "YData", filtered(:,6));
            set(hCB_A, "YData", filtered(:,7));
            set(hCB_F, "YData", filtered(:,8));

            % Multiply & average for I/Q
            for ch=1:8
                I_product(:,ch) = filtered(:,ch) .* ref0(:);
                Q_product(:,ch) = filtered(:,ch) .* ref90(:);
            end
            I_avg = mean(I_product);
            Q_avg = mean(Q_product);
            I_demod = I_avg * (pi/2);
            Q_demod = Q_avg * (pi/2);
            phase_est = rad2deg(atan2(Q_demod, I_demod));

            % Compute wrapped phase differences
            wrap = @(x) mod(x+180,360)-180;
            voltA = wrap(phase_est(2)-phase_est(1));
            currA = wrap(phase_est(4)-phase_est(3));
            voltB = wrap(phase_est(6)-phase_est(5));
            currB = wrap(phase_est(8)-phase_est(7));

            % Update text panels
            set(txtVA, "String", sprintf("Volt Phase A: %.1f° (%.1f°–%.1f°)", ...
                voltA, phase_est(2), phase_est(1)));
            set(txtCA, "String", sprintf("Curr Phase A: %.1f° (%.1f°–%.1f°)", ...
                currA, phase_est(4), phase_est(3)));
            set(txtVB, "String", sprintf("Volt Phase B: %.1f° (%.1f°–%.1f°)", ...
                voltB, phase_est(6), phase_est(5)));
            set(txtCB, "String", sprintf("Curr Phase B: %.1f° (%.1f°–%.1f°)", ...
                currB, phase_est(8), phase_est(7)));

            drawnow;
        end
    catch err
        disp("Error: " + err.message);
        break;
    end
end


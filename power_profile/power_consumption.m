% Calculating total power of one node
% Startup Current - 20mV/10
% Radio ON Peak - 26.5mV/10
% Radio in CCA/NOT off - 20mV/10
demoMAC()

% function TSCHMAC()
%     clear all;
% 
%     global P_total timex P_mode P_needed;
%     define_constants();
%     update_power(0,0,0.1);
%     update_power(turn_on_node(),0,0.340); % Turn on Node
%     update_power(idle_state(1),0,0.2); % CCA
%     update_power(switch_tx_rx(0),0,0.192);
%     
% end

function demoMAC()
    clear all;

    global P_total timex P_mode P_needed;
    define_constants();
    update_power(0,0,0.1);
    update_power(turn_on_node(),0,0.340);
    update_power(idle_state(1),0,0.2);
    update_power(switch_tx_rx(0),0,0.192);
    update_power(sense_send_message(),0,0.5);
    update_power(switch_tx_rx(1),0,0.192);
    update_power(idle_state(1),0,0.3);
    update_power(sleep_node(),0,0.5);
    update_power(0,0,0.5);
    
    figure
%     subplot(3,1,1)
%     plot(timex,P_total);
%     xlabel('time');
%     ylabel('Power changes');
    subplot(2,1,1)
    stairs(timex,P_mode);
    xlabel('time');
    ylabel('Mode');
    subplot(2,1,2)
    plot(timex,P_needed,'o-');
    xlabel('time (ms)');
    ylabel('Power (mW)');
    axis([0 10 0 500]);
    P_needed
    timex
%     trapz(timex,P_needed); % Energy needed!
end

function define_constants()
    disp('Defining Constants');
    VDD = 3;
    global P_off_on_mcu P_off_on_sensor P_rx_tx_radio P_tx_rx_radio;
    P_off_on_mcu = 140 * VDD; % Node in RX State
    P_off_on_sensor = 0;
    P_rx_tx_radio = 36 * VDD; % RX to TX
    P_tx_rx_radio = 36 * VDD; % TX to RX
    
    global P_idle_mcu P_rx_radio P_idle_radio P_idle_sensor;
    P_idle_mcu = 13 * VDD; % 32-MHz XOSC running. No radio or peripherals active. CPU running at 32-MHz with flash access
    P_rx_radio = 20 * VDD; % 32-MHz XOSC running, radio in RX mode, –50-dBm input power, no peripherals active, CPU idle
    P_idle_radio = 0;
    P_idle_sensor = 0;

    global P_on_sleep_mcu P_sleep_mcu P_sleep_on_mcu P_on_sleep_radio P_sleep_radio P_sleep_on_radio;
    P_on_sleep_mcu = 31.07 * VDD; % Mode 2
    P_sleep_mcu = 0.4 * 10^(-3) * VDD;
    P_sleep_on_mcu = 140 * VDD;
    P_on_sleep_radio = 0;
    P_sleep_radio = 0;
    P_sleep_on_radio = 0;

    global P_process_packet P_send_packet P_read_sensor;
    P_process_packet = 1 * VDD;
    P_send_packet = 26 * VDD;
    P_read_sensor = 55; 
    
    global P_total timex P_mode;
    P_total(1) = 100;
    timex(1) = 0;
    P_mode(1) = 0;
    
    global ON IDLE RECIEVE SLEEP OFF SENSE_SEND TX;
    ON = 6;
    IDLE = 3;
    RECIEVE = 4;
    TX = 5;
    SLEEP = 2;
    OFF = 0;
    SENSE_SEND = 7;
end

function p_on_node = turn_on_node()
    global P_off_on_mcu P_mode ON;
    P_mode(length(P_mode) + 1) = ON;
    
    p_on_node = P_off_on_mcu;
end

function p_idle_state = idle_state(RX) % RX = 1(Radio in receive state), RX = 0(Radio in idle)
    global P_idle_mcu P_rx_radio P_idle_radio P_mode IDLE RECIEVE;
    if RX == 1
        P_mode(length(P_mode) + 1) = RECIEVE;
    else
        P_mode(length(P_mode) + 1) = IDLE;
    end
    P_mcu_state = P_idle_mcu; % Remain in idle state
    P_radio_state = RX * P_rx_radio + (1 - RX) * P_idle_radio; % RX Power in Idle state
                                                               % Radio in Idle mode 
                                                               
    p_idle_state = P_mcu_state + P_radio_state;
end

function p_sleep = sleep_node()
    global P_on_sleep_mcu P_on_sleep_radio P_sleep_radio P_mode SLEEP;
    
    P_mode(length(P_mode) + 1) = SLEEP;
    P_mcu_state = P_on_sleep_mcu; % MCU to sleep state
    P_radio_state =  P_on_sleep_radio; % Radio to sleep state
    P_sensor_state = 0; % Cutoff power completely?
    
    p_sleep = P_mcu_state + P_radio_state + P_sensor_state;
end
 
function p_wakeup = wakeup_node()
    global P_sleep_on_mcu P_sleep_on_radio P_mode ON;
    
    P_mode(length(P_mode) + 1) = ON;
    P_mcu_state = P_sleep_on_mcu; % MCU from sleep to on
    P_radio_state =  P_sleep_on_radio; % Radio from sleep to on
    P_sensor_state = 0; % Turn on sensor
    
    p_wakeup = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_message = sense_send_message()
    
    global P_process_packet P_send_packet P_read_sensor P_mode P_off_on_sensor SENSE_SEND;
    P_mode(length(P_mode) + 1) = SENSE_SEND;
    P_mcu_state = P_process_packet;
    P_radio_state = P_send_packet;
    P_sensor_state = P_off_on_sensor + P_read_sensor;
    
    p_message = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_switch = switch_tx_rx(MODE) % MODE = 0(TX),1(RX)
    global P_tx_rx_radio P_rx_tx_radio P_mode RECIEVE TX;
    
    if MODE == 1
        p_switch = P_tx_rx_radio;
        P_mode(length(P_mode) + 1) = RECIEVE;
    elseif MODE == 0
        p_switch = P_rx_tx_radio;
        P_mode(length(P_mode) + 1) = TX;
    end
end

% Inputs -  power_sub - Power to be subtracted
%           power_add - Power to be added
%           time_elapsed - time since last change was called
function update_power(power_sub,power_add,time_elapsed)
    global P_total timex P_needed P_mode;
    P_total(length(P_total) + 1) = P_total(length(P_total)) - power_sub + power_add;
    P_needed(length(P_total)) = power_sub;
    timex(length(P_total)) = timex(length(P_total)-1) + time_elapsed;
    if power_sub == 0 && power_add == 0
        P_mode(length(P_total)) = P_mode(length(P_total) - 1);
    end
end



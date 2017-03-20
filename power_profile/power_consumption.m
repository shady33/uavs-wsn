% Calculating total power of every node

clear all;

global P_total timex P_mode;
define_constants();
update_power(turn_on_node(),0,1);
update_power(sense_send_message(),0,2);
update_power(sleep_node(),0,2);
update_power(wakeup_node(),0,4);
update_power(sense_send_message(),0,9);
update_power(idle_state(1),0,4);
update_power(idle_state(1),20,2);
plot(timex,P_total);
xlabel('time');
ylabel('Power changes');

stairs(timex,P_mode);
xlabel('time');
ylabel('Mode');

function define_constants()
    disp('Defining Constants');
    VDD = 3;
    global P_off_on_mcu P_off_on_sensor P_off_on_radio;
    P_off_on_mcu = 95 * 0.001 * VDD; % Node in RX State
    P_off_on_sensor = 0;
    P_off_on_radio = 12.85 * 0.001 *VDD; % RX to TX
    
    global P_idle_mcu P_rx_radio P_idle_radio P_idle_sensor
    P_idle_mcu = 13 * 0.001 * VDD; % 32-MHz XOSC running. No radio or peripherals active. CPU running at 32-MHz with flash access
    P_rx_radio = 20 * 0.001 * VDD; % 32-MHz XOSC running, radio in RX mode, –50-dBm input power, no peripherals active, CPU idle
    P_idle_radio = 0;
    P_idle_sensor = 0;

    global P_on_sleep_mcu P_sleep_mcu P_sleep_on_mcu P_on_sleep_radio P_sleep_radio P_sleep_on_radio;
    P_on_sleep_mcu = 1;
    P_sleep_mcu = 2;
    P_sleep_on_mcu = 3;
    P_on_sleep_radio = 1;
    P_sleep_radio = 1;
    P_sleep_on_radio = 1;

    global P_process_packet P_send_packet P_read_sensor;
    P_process_packet = 2;
    P_send_packet = 9;
    P_read_sensor = 0;
    
    global P_total timex P_mode;
    P_total(1) = 100;
    timex(1) = 0;
    P_mode(1) = 0;
    
    global ON IDLE RECIEVE SLEEP OFF SENSE_SEND;
    ON = 5;
    IDLE = 3;
    RECIEVE = 4;
    SLEEP = 2;
    OFF = 0;
    SENSE_SEND = 7;
end

function p_on_node = turn_on_node()

    global P_off_on_mcu P_off_on_sensor P_off_on_radio P_mode ON;
    
    P_mode(length(P_mode) + 1) = ON;
    P_off_on_mcu; % Turn on the MCU
    P_off_on_sensor; % Turn on the Sensor
    P_off_on_radio; % Turn on the Radio
    
    p_on_node = P_off_on_mcu + P_off_on_sensor + P_off_on_radio;
end

function p_idle_state = idle_state(RX) % RX = 1(Radio in receive state), RX = 0(Radio in idle)

    global P_idle_mcu P_rx_radio P_idle_radio P_idle_sensor P_mode IDLE RECIEVE;
    if RX == 1
        P_mode(length(P_mode) + 1) = RECIEVE;
    else
        P_mode(length(P_mode) + 1) = IDLE;
    end
    P_mcu_state = P_idle_mcu; % Remain in idle state
    P_radio_state = RX * P_rx_radio + (1 - RX) * P_idle_radio; % RX Power in Idle state
                                                               % Radio in Idle mode
    P_sensor_state = P_idle_sensor; % Sensor in idle mode
    
    p_idle_state = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_sleep = sleep_node()

    global P_on_sleep_mcu P_sleep_mcu P_on_sleep_radio P_sleep_radio P_mode SLEEP;
    
    P_mode(length(P_mode) + 1) = SLEEP;
    P_mcu_state = P_on_sleep_mcu + P_sleep_mcu; % MCU to sleep and power in that state
    P_radio_state =  P_on_sleep_radio + P_sleep_radio; % Radio to sleep and power in that state
    P_sensor_state = 0; % Cutoff power completely?
    
    p_sleep = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_wakeup = wakeup_node()
    
    global P_sleep_on_mcu P_sleep_on_radio P_off_on_sensor P_mode ON;
    P_mode(length(P_mode) + 1) = ON;
    P_mcu_state = P_sleep_on_mcu; % MCU from sleep to on
    P_radio_state =  P_sleep_on_radio; % Radio from sleep to on
    P_sensor_state = P_off_on_sensor; % Turn on sensor
    
    p_wakeup = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_message = sense_send_message()
    
    global P_process_packet P_send_packet P_read_sensor P_mode SENSE_SEND;
    P_mode(length(P_mode) + 1) = SENSE_SEND;
    P_mcu_state = P_process_packet;
    P_radio_state = P_send_packet;
    P_sensor_state = P_read_sensor;
    
    p_message = P_mcu_state + P_radio_state + P_sensor_state;
end

% Inputs -  power_sub - Power to be subtracted
%           power_add - Power to be added
%           time_elapsed - time since last change was called

function update_power(power_sub,power_add,time_elapsed)
    global P_total timex;
    P_total(length(P_total) + 1) = P_total(length(P_total)) - power_sub + power_add;
    timex(length(P_total)) = timex(length(P_total)-1) + time_elapsed;
end



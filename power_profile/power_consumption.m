% Calculating total power of every node

global P_total_power timex;
define_constants();
update_power(turn_on_node(),0,1);
update_power(sense_send_message(),0,3);
update_power(sleep_node(),0,5);
update_power(wakeup_node(),0,12);
update_power(sense_send_message(),0,15);
update_power(idle_state(1),0,18);

plot(timex,P_total_power);

function define_constants()
    clear all;
    disp('Defining Constants');
    global P_off_on_mcu P_off_on_sensor P_off_on_radio;
    P_off_on_mcu = 2;
    P_off_on_sensor = 2;
    P_off_on_radio = 2;
    
    global P_idle_mcu P_rx_radio P_idle_radio P_idle_sensor
    P_idle_mcu = 1;
    P_rx_radio = 2;
    P_idle_radio = 1;
    P_idle_sensor = 1;

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
    P_read_sensor = 4;
    
    global P_total_power timex;
    P_total_power(1) = 100;
    timex(1) = 0;
end

function p_on_node = turn_on_node()

    global P_off_on_mcu P_off_on_sensor P_off_on_radio;
    P_off_on_mcu; % Turn on the MCU
    P_off_on_sensor; % Turn on the Sensor
    P_off_on_radio; % Turn on the Radio
    
    p_on_node = P_off_on_mcu + P_off_on_sensor + P_off_on_radio;
end

function p_idle_state = idle_state(RX) % RX = 1(Radio in receive state), RX = 0(Radio in idle)

    global P_idle_mcu P_rx_radio P_idle_radio P_idle_sensor
    P_mcu_state = P_idle_mcu; % Remain in idle state
    P_radio_state = RX * P_rx_radio + (1 - RX) * P_idle_radio; % RX Power in Idle state
                                                               % Radio in Idle mode
    P_sensor_state = P_idle_sensor; % Sensor in idle mode
    
    p_idle_state = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_sleep = sleep_node()

    global P_on_sleep_mcu P_sleep_mcu P_on_sleep_radio P_sleep_radio;
    P_mcu_state = P_on_sleep_mcu + P_sleep_mcu; % MCU to sleep and power in that state
    P_radio_state =  P_on_sleep_radio + P_sleep_radio; % Radio to sleep and power in that state
    P_sensor_state = 0; % Cutoff power completely?
    
    p_sleep = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_wakeup = wakeup_node()

    global P_sleep_on_mcu P_sleep_on_radio P_off_on_sensor;
    P_mcu_state = P_sleep_on_mcu; % MCU from sleep to on
    P_radio_state =  P_sleep_on_radio; % Radio from sleep to on
    P_sensor_state = P_off_on_sensor; % Turn on sensor
    
    p_wakeup = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_message = sense_send_message()
    
    global P_process_packet P_send_packet P_read_sensor;
    P_mcu_state = P_process_packet;
    P_radio_state = P_send_packet;
    P_sensor_state = P_read_sensor;
    
    p_message = P_mcu_state + P_radio_state + P_sensor_state;
end

function update_power(power_sub,power_add,time_x)
    global P_total_power timex;
    P_total_power(length(P_total_power) + 1) = P_total_power(length(P_total_power)) - power_sub + power_add;
    timex(length(P_total_power)) = time_x;

end



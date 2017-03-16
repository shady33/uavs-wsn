% Calculating total power of every node

function define_constants()
    global P_off_on_mcu P_off_on_sensor P_off_on_radio;
    P_off_on_mcu = 0;
    P_off_on_sensor = 0;
    P_off_on_radio = 0;
    
    global P_idle_state_mcu P_rx_state p_idle_radio P_idle_sensor
    P_idle_state_mcu = 0;
    P_rx_state = 0;
    p_idle_radio = 0;
    P_idle_sensor = 0;

    global P_on_sleep_mcu P_sleep_mcu P_sleep_on_mcu P_sleep_on_radio;
    P_on_sleep_mcu = 0;
    P_sleep_mcu = 0;
    P_sleep_on_mcu = 0;
    P_sleep_on_radio = 0;

    global P_process_packet P_send_packet P_read_sensor;
    P_process_packet = 0;
    P_send_packet = 0;
    P_read_sensor = 0;
end



function p_total_node = turn_on_node()
    P_off_on_mcu; % Turn on the MCU
    P_off_on_sensor; % Turn on the Sensor
    P_off_on_radio; % Turn on the Radio
    
    p_total_node = P_off_on_mcu + P_off_on_sensor + P_off_on_radio;
end

function p_idle_state = idle_state(RX) % RX = 1(Radio in receive state), RX = 0(Radio in idle)
    P_mcu_state = P_idle_mcu; % Remain in idle state
    P_radio_state = RX * P_rx_radio + (1 - RX) * P_idle_radio; % RX Power in Idle state
                                                               % Radio in Idle mode
    P_sensor_state = P_idle_sensor; % Sensor in idle mode
    
    p_idle_state = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_sleep = sleep_node()
    P_mcu_state = P_on_sleep_mcu + P_sleep_mcu; % MCU to sleep and power in that state
    P_radio_state =  P_on_sleep_radio + P_sleep_radio; % Radio to sleep and power in that state
    P_sensor_state = 0; % Cutoff power completely?
    
    p_sleep = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_wakeup = wakeup_node()
    P_mcu_state = P_sleep_on_mcu; % MCU from sleep to on
    P_radio_state =  P_sleep_on_radio; % Radio from sleep to on
    P_sensor_state = P_off_on_sensor; % Turn on sensor
    
    p_wakeup = P_mcu_state + P_radio_state + P_sensor_state;
end

function p_message = sense_send_message()
    P_mcu_state = P_process_packet;
    P_radio_state = P_send_packet;
    P_sensor_state = P_read_sensor;
    
    p_message = P_mcu_state + P_radio_state + P_sensor_state;
end



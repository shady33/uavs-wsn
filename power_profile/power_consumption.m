% Calculating total power of one node
% Current values are in mA, Voltage is in Volts and Power is in mVA, time
% is in Seconds

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

function val = toState(mode)
    global ON IDLE RECIEVE SLEEP OFF SENSE_SEND TX;
    switch mode
        case ON
            val = 'On';
        case IDLE
            val = 'Idle';
        case RECIEVE
            val = 'RX';
        case SLEEP
            val = 'Sleep';
        case OFF
            val = 'Off';
        case SENSE_SEND
            val = 'SenseSend';
        case TX
            val = 'TX';
    end
end
function demoMAC()
    clear all;

    global P_total timex P_mode P_needed VDD;
    define_constants();
%     update_power(0,0,0.4);
%     update_power(turn_on_node(),0,0.340);
%     update_power(turn_on_radio(),0,0.5);
%     update_power(idle_state(1),0,0.2);
%     update_power(switch_tx_rx(0),0,0.192);
%     update_power(sense_send_message(),0,0.5);
%     update_power(switch_tx_rx(1),0,0.192);
%     update_power(idle_state(1),0,0.3);
%     update_power(sleep_node(),0,0.5);
%     update_power(0,0,0.5);

    update_power(0,0,4);
    update_power(turn_on_node(),0,0.340);
    update_power(turn_on_radio(),0,0.5);
    update_power(idle_state(1),0,5);
    update_power(switch_tx_rx(0),0,0.192);
    update_power(sense_send_message(),0,5);
    update_power(switch_tx_rx(1),0,0.192);
    update_power(idle_state(0),0,5);
%     update_power(sleep_node(),0,5);
    update_power(0,0,0.5);
    
    figure
    data_csv = csvread('PowerProfile.csv');
    plot(data_csv(:,1),data_csv(:,3),'o-')
%     subplot(3,1,1)
%     plot(timex,P_total);
%     xlabel('time');
%     ylabel('Power changes');
%     subplot(2,1,1)
%     stairs(timex,P_mode);
%     xlabel('time');
%     ylabel('Mode');
%     subplot(2,1,2)
     hold on;
%      stairs(timex,P_needed,'o-');
    stairs(timex,(P_needed/VDD),'r-');
    hold off;
    xlabel('time(s)');
    ylabel('Current(mA)');
    
    for n=1:((size(timex,2)/2) - 1)
        text(timex(n*2),P_needed(n*2)/VDD,toState(P_mode(n)),'FontSize', 20)
    end
%     trapz(timex,P_needed); % Energy needed!
end

function define_constants()
    disp('Defining Constants');
    global VDD;
    VDD = 3;
    global P_off_on_mcu P_off_on_sensor P_rx_tx_radio P_tx_rx_radio P_off_on_radio;
    P_off_on_mcu = 15 * VDD; % Node in RX State
    P_off_on_sensor = 0;
    P_rx_tx_radio = 1 * VDD; % RX to TX
    P_tx_rx_radio = 1 * VDD; % TX to RX
    P_off_on_radio = P_off_on_mcu + 15 * VDD;
    
    global P_idle_mcu P_rx_radio P_idle_sensor P_idle_radio;
    P_idle_mcu = 0.6 * VDD; % 32-MHz XOSC running. No radio or peripherals active. CPU running at 32-MHz with flash access
    P_rx_radio = 20 * VDD; % 32-MHz XOSC running, radio in RX mode, –50-dBm input power, no peripherals active, CPU idle
    P_idle_sensor = 0;
    P_idle_radio = 0;

    global P_on_sleep_mcu P_sleep_on_mcu P_on_sleep_radio P_sleep_radio P_sleep_on_radio;
    P_on_sleep_mcu = 1 * VDD; 
%     P_sleep_mcu = 1.3 * 10^(-3) * VDD; % Power Mode 2
    P_sleep_on_mcu = 15 * VDD;
    P_on_sleep_radio = 0;
    P_sleep_radio = 0;
    P_sleep_on_radio = 0;

    global P_process_packet P_send_packet P_read_sensor;
%     P_process_packet = 1 * VDD;
    P_process_packet = 0;
    P_send_packet = 24 * VDD;
    P_read_sensor = 0; 
%     P_read_sensor = 55; 
    
    global P_total timex P_mode P_needed;
    P_total(1) = 100;
    timex(1) = 0;
    P_mode(1) = 0;
    P_needed(1) = 0;
    
    global ON IDLE RECIEVE SLEEP OFF SENSE_SEND TX;
    ON = 6;
    IDLE = 3;
    RECIEVE = 4;
    TX = 5;
    SLEEP = 2;
    OFF = 0;
    SENSE_SEND = 7;
end

function p_on_node = turn_on_radio()
    global P_off_on_radio P_mode ON;
    P_mode(length(P_mode) + 1) = ON;
    
    p_on_node = P_off_on_radio;
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
                                                               % Radio in TX mode 
                                                               
    p_idle_state = P_mcu_state + P_radio_state;
end

function p_sleep = sleep_node()
    global P_on_sleep_mcu P_on_sleep_radio P_mode SLEEP;
    
    P_mode(length(P_mode) + 1) = SLEEP;
    P_mcu_state =  P_on_sleep_mcu; % MCU to sleep state
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
    global P_rx_radio P_tx_rx_radio P_rx_tx_radio P_mode RECIEVE TX;
    
    if MODE == 1
        p_switch = P_rx_radio + P_tx_rx_radio;
        P_mode(length(P_mode) + 1) = RECIEVE;
    elseif MODE == 0
        p_switch = P_rx_radio + P_rx_tx_radio;
        P_mode(length(P_mode) + 1) = TX;
    end
end

% Inputs -  power_sub - Power to be subtracted
%           power_add - Power to be added
%           time_elapsed - time since last change was called
function update_power(power_sub,power_add,time_elapsed)
    global P_total timex P_needed P_mode;
    
    P_needed(length(P_needed) + 1) = power_sub;
    timex(length(P_needed)) = timex(length(P_needed)-1);
    
    P_needed(length(P_needed) + 1) = power_sub;
    timex(length(P_needed)) = timex(length(P_needed)-1) + time_elapsed;
    
%     P_total(length(P_total) + 1) = P_total(length(P_total)) - power_sub + power_add;
%     
%     P_needed(length(P_needed) + 1) = power_sub;
%     timex(length(P_needed)) = timex(length(P_needed)-1);
%     
%     P_needed(length(P_needed) + 1) = power_sub;
% %     timex(length(P_needed)) = timex(length(P_needed)-1) + time_elapsed;
%       if power_sub == 0 && power_add == 0
%         P_mode(length(P_needed)) = P_mode(length(P_needed));
%       end
end



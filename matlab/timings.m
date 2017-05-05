% Modelling of broadcast duration, speed of drone, range of
% communication, number of packets, time for sending stuff
hit_location = 70;
desired_speed = 5;
disp(strcat('Possible or not with changing distance:',num2str(changing_distance(hit_location))));
disp(strcat('Possible or not with changing speed:',num2str(changing_speed(hit_location,desired_speed))));

function possible = changing_distance(hit_distance)
    broadcast = 1;             %seconds
    speed = 10;                 %m/s
    range = 10;                %meters
    num_packets = 15;   
    time_per_packet = 0.06;    %seconds 
    %3.47s for 10seconds timer,2s backoff
    %1s for 15 packets of size 49bytes each
    total_time_packets = num_packets * time_per_packet;
    check_packets = 2;          %seconds
    if(check_packets < total_time_packets)
        disp('Check packets smaller than total time required to send');
        possible = false;
        return;
    end
    
    total_time = check_packets;
    
	%dropped packets
    r = floor((4)*rand(1));
	if r(1) ~= 0
        str = strcat('Dropped packets:',num2str(r(1)));
        disp(str);
        total_time = total_time + (r(1) + 1) * time_per_packet;
    end 
    total_time = total_time + 1 * time_per_packet; %sending backoff packet
    
    no_of_broadcasts = range / (broadcast * speed);
    str = strcat('No of packets:' , num2str(no_of_broadcasts));
    disp(str);
    
    output1 = zeros(100,1);
    j = 0;
    %changing range, assuming hit
    for i = linspace(1,range,100)
        j = j + 1;
        output1(j) = ((range - i)/speed) - total_time;
    end
    plot(output1);
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    xlabel('Range of communication');
    ylabel('Free time/Possible backoff times');
    
    if output1(hit_distance) > 0
        possible = true;
    else
        possible = false;
    end 
    backoff = 2;                %seconds
end

function possible = changing_speed(hit_distance,desired_speed)
    broadcast = 1;             %seconds
    max_speed = 10;                 %m/s
    range = 100;                %meters
    num_packets = 15;   
    time_per_packet = 0.06;    %seconds 
    %3.47s for 10seconds timer,2s backoff
    %1s for 15 packets of size 49bytes each
    total_time_packets = num_packets * time_per_packet;
    check_packets = 2;          %seconds
    if(check_packets < total_time_packets)
        disp('Check packets smaller than total time required to send');
        possible = false;
        return;
    end
    
    total_time = check_packets;
    
	%dropped packets
    r = floor((4)*rand(1));
	if r(1) ~= 0
        str = strcat('Dropped packets:',num2str(r(1)));
        disp(str);
        total_time = total_time + (r(1) + 1) * time_per_packet;
    end 
    total_time = total_time + 1 * time_per_packet; %sending backoff packet
        
    output1 = zeros(100,1);
    no_of_broadcasts = zeros(100,1);
    j = 0;
    for i = linspace(1,max_speed,100)
        j = j + 1;
        output1(j) = ((range - hit_distance)/i) - total_time;
        no_of_broadcasts(j) = range / (broadcast * i);
    end
    
    plot(linspace(1,max_speed,100),output1);
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    xlabel('Speed');
    ylabel('Free time/Possible backoff times');
    
    disp(no_of_broadcasts);
    if output1(desired_speed * 10) > 0
        possible = true;
    else
        possible = false;
    end
    backoff = 2;                %seconds
end
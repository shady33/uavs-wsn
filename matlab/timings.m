% Modelling of broadcast duration, speed of drone, range of
% communication, number of packets, time for sending stuff

hit_location = 50;          %distance
desired_speed = 5;          %m/s
broadcast = 10;             %seconds
range = 100;                %meters

disp(strcat('Possible or not with changing hit distance:',num2str(changing_distance(hit_location,desired_speed,broadcast,range))));
disp(strcat('Possible or not with changing speed:',num2str(changing_speed(hit_location,desired_speed,broadcast,range))));
disp(strcat('Possible or not with changing range:',num2str(changing_range(hit_location,desired_speed,broadcast,range))));

%changes hit location i.e when the packet is received
function possible = changing_distance(hit_distance,speed,broadcast,range)
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
    figure;
    hold on;
    plot(linspace(1,range,100),output1);
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    xlabel('Hit distance');
    ylabel('Free time/Possible backoff times');
    title('Plot that shows the possible backoff times by varying the distance at which the packet is received');
    [a index] = min(abs(linspace(1,range,100) - hit_distance));
    plot(hit_distance,output1(index),'r*');
    dim = [0.2 0.5 0.3 0.3];
    str = {'Fixed Parameters',strcat('Speed: ',num2str(speed)),strcat('Range: ',num2str(range))};
    annotation('textbox',dim,'String',str,'FitBoxToText','on');

    if output1(hit_distance) > 0
        possible = true;
    else
        possible = false;
    end 
    backoff = 2;                %seconds
end

% changes range of communication
function possible = changing_range(hit_distance,speed,broadcast,range)
    if hit_distance > range
        disp('Hit distance cannot be further from range');
        possible = false;
        return;
    end
    max_range = 100;
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
    for i = linspace(1,max_range,100)
        j = j + 1;
        output1(j) = ((i - hit_distance)/speed) - total_time;
        no_of_broadcasts(j) = i / (broadcast * speed);
    end
    
    figure;
    hold on;
    plot(linspace(1,max_range,100),output1);
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    xlabel('Range(TX)');
    ylabel('Free time/Possible backoff times');
    title('Plot that shows the possible backoff times by varying the tx range of the nodes');
    plot(range,output1(range),'r*');
    
    dim = [0.2 0.5 0.3 0.3];
    str = {'Fixed Parameters',strcat('Speed: ',num2str(speed)),strcat('Hit distance: ',num2str(hit_distance))};
    annotation('textbox',dim,'String',str,'FitBoxToText','on');
    
%     disp(no_of_broadcasts);
    if output1(range) > 0
        possible = true;
    else
        possible = false;
    end
    backoff = 2;                %seconds
end

% changes speed of drone
function possible = changing_speed(hit_distance,desired_speed,broadcast,range)
    max_speed = 10;                 %m/s
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
    
    figure;
    hold on;
    plot(linspace(1,max_speed,100),output1);
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    xlabel('Speed');
    ylabel('Free time/Possible backoff times');
    title('Plot that shows the possible backoff times by varying the speed of the drone');
    plot(desired_speed,output1(desired_speed*10),'r*');
    dim = [0.2 0.5 0.3 0.3];
    str = {'Fixed Parameters',strcat('Hit Distance: ',num2str(hit_distance)),strcat('Range: ',num2str(range))};
    annotation('textbox',dim,'String',str,'FitBoxToText','on');
    
%     disp(no_of_broadcasts);
    if output1(desired_speed * 10) > 0
        possible = true;
    else
        possible = false;
    end
    backoff = 2;                %seconds
end
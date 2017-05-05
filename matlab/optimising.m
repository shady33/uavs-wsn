hit_location = 1;          %distance
desired_speed = 10;          %m/s
broadcast = 10;             %seconds
range = 100;                %meters

optimise(hit_location,desired_speed,broadcast,range);

function optimise(hit_location,desired_speed,broadcast,range)
    max_range = 100;
    max_speed = 10;
    num_packets = 15;   
    time_per_packet = 0.06;    %seconds 
    %3.47s for 10seconds timer,2s backoff
    %1s for 15 packets of size 49bytes each
    total_time_packets = num_packets * time_per_packet;
    check_packets = 2;          %seconds
    if(check_packets < total_time_packets)
        disp('Check packets smaller than total time required to send');
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
    
    no_of_broadcasts = range / (broadcast * desired_speed);
    str = strcat('No of packets:' , num2str(no_of_broadcasts));
    disp(str);
    
    output1 = zeros(100,1);
    j = 0;
    %changing hit distance
    for i = linspace(1,range,100)
        j = j + 1;
        output1(j) = ((range - i)/desired_speed) - total_time;
    end
    
    output2 = zeros(100,1);
    no_of_broadcasts_2 = zeros(100,1);
    j = 0;
    for i = linspace(1,max_range,100)
        j = j + 1;
        output2(j) = ((i - hit_location)/desired_speed) - total_time;
        no_of_broadcasts_2(j) = i / (broadcast * desired_speed);
    end
    
    output3 = zeros(100,1);
    no_of_broadcasts_3 = zeros(100,1);
    j = 0;
    for i = linspace(1,max_speed,100)
        j = j + 1;
        output3(j) = ((range - hit_location)/i) - total_time;
        no_of_broadcasts_3(j) = range / (broadcast * i);
    end
    figure;
    hold on;
    plot(output1);
    plot(output2);
    plot(output3);
    
	plot(hit_location,output1(hit_location),'r*');
    plot(range,output2(range),'b*');
    plot(desired_speed*10,output3(desired_speed*10),'g*');
    
    ax = gca;                        % gets the current axes
    ax.XAxisLocation = 'origin';     % sets them to zero
    ax.YAxisLocation = 'origin';     % sets them to zero
    legend('Varying Hit Distance','Varying TX Range','Varying Speed of drone','Location','northeast')
    title('Trying to optimise by plotting in same figure');
    xlabel('All quantities divided into 100 equal parts');
    ylabel('Free time/Possible backofftimes');
    dim = [0.2 0.5 0.3 0.3];
    str = {strcat('Hit Locations: ',num2str(hit_location)),strcat('Range: ',num2str(range)),strcat('Speed: ',num2str(desired_speed))};
    annotation('textbox',dim,'String',str,'FitBoxToText','on');
    
end
% fspl - db
% d - m
% f - Hz
% Gx - dB
% Px - dBm
% maxrange - m
% alt - m

set(0,'defaultAxesFontName','Calibri');
set(0,'defaultAxesFontSize',25);
set(0,'defaultlinelinewidth',5);

f = 2.4 * (10 ^ 9);
maxrange = 500;
maxalt = 50;
Pt = 7;
Gt = 0;
Gr = 0;
receivedpower(f,maxrange,maxalt,Pt,Gt,Gr)
% angle(f,maxrange,maxalt,Pt,Gt,Gr);
function receivedpower(f,maxrange,maxalt,Pt,Gt,Gr)
d = linspace(1,maxrange,maxrange);
j = 0:10:maxalt;
n = 2.7;
hold on
for alt = j
	l = sqrt(power(d,2)+alt^2);
	Pr = Pt + Gt + Gr + n * ((10 * log10(physconst('LightSpeed') / ( 4 * pi * f))) - (10 * log10(l)));
	n = 2.3;
    plot(d,Pr);
end
% Pr = Pt + Gt + Gr + (20 * log10(physconst('LightSpeed') / ( 4 * pi * f))) - (20 * log10(l));
xlabel('Distance along x (m)');
ylabel('Received power (dBm)');
legendCell = cellstr(num2str(j', 'Alt=%-dm'));
legend(legendCell);
plot([0 maxrange],[-99 -99]);
hold off;
end
% fspl = (20 * log10(d)) + (20 * log10(f)) + (20 * log10((4 * pi)/physconst('LightSpeed')));
% plot(fspl);

function angle(f,maxrange,maxalt,Pt,Gt,Gr)
d = linspace(1,maxrange,maxrange);
j = 0:10:maxalt;
hold on;
for alt=j
    val = rad2deg(atan(d./alt));
    plot(d,val);
end
xlabel('Distance along x (m)');
ylabel('Angle in degrees');
legendCell = cellstr(num2str(j', 'Alt=%-dm'));
legend(legendCell);
plot([0 maxrange],[55 55]);
hold off;
end

% d = (Pt + Gt + Gr + (20 * log10(physconst('LightSpeed') / ( 4 * pi * f))) - Pr)/20;
% d